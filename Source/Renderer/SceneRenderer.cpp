#include "SceneRenderer.h"
#include "VulkanRenderer.h"
#include "../Application.h"
#include <iostream>
#include "../PhysicsSystem.h"
#include "../Scene.h"
#include "../Components.h"
#include "../LineMeshComponent.h"
#include "ModelManager.h"
#include "../Utils.h"
#include <future>

namespace FLOOF {
    SceneRenderer::SceneRenderer() {
        CreateTextureRenderer();

        m_Skybox = std::make_unique<Skybox>("Assets/Skybox/dikhololo_night_4k.hdr");
        m_IrradienceMap = m_Skybox->m_Cubemap.GetIrradienceMap();
        m_PrefilterMap = m_Skybox->m_Cubemap.GetPrefilterMap();
        m_BRDFLut = TextureManager::GetBRDFLut();

        auto* renderer = VulkanRenderer::Get();

        VkSampler sampler = renderer->GetTextureSampler();

        std::vector<VkDescriptorImageInfo> imageInfos{
            { sampler, m_IrradienceMap.ImageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL },
            { sampler, m_PrefilterMap.ImageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL },
        };

        m_IrradiencePrefilterDescriptorSet = renderer->AllocatePBRMapsDescriptorSet(renderer->GetDescriptorSetLayout(RenderSetLayouts::PBRMaps));

        std::array<VkWriteDescriptorSet, 2> writeDescriptorSets{};
        for (uint32_t i = 0; i < imageInfos.size(); i++) {
            writeDescriptorSets[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            writeDescriptorSets[i].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            writeDescriptorSets[i].descriptorCount = 1;
            writeDescriptorSets[i].dstSet = m_IrradiencePrefilterDescriptorSet;
            writeDescriptorSets[i].dstBinding = i;
            writeDescriptorSets[i].pImageInfo = &imageInfos[i];
        }

        vkUpdateDescriptorSets(renderer->GetDevice(), static_cast<uint32_t>(writeDescriptorSets.size()), writeDescriptorSets.data(), 0, nullptr);
    }

    SceneRenderer::~SceneRenderer() {
        DestroyTextureRenderer();
    }

    SceneRenderFinishedData SceneRenderer::RenderToTexture(Scene* scene, glm::vec2 extent, CameraComponent* camera,
        RenderPipelineKeys drawMode, PhysicsDebugDraw* physicDrawer, VkSemaphore waitSemaphore, bool drawDebugLines) {
        if (extent == glm::vec2(0.f))
            return SceneRenderFinishedData();
        if (extent != m_Extent)
            ResizeBuffers(extent);

        m_DrawMode = drawMode;
        m_DrawDebugLines = drawDebugLines;

        // Camera setup
        glm::mat4 cameraProjection = camera->GetPerspective(glm::radians(70.f), m_Extent.x / m_Extent.y, m_NearClip, m_FarClip);
        glm::mat4 cameraView = camera->GetView();
        glm::mat4 vp = cameraProjection * cameraView;

        m_SceneFrameData.View = cameraView;
        m_SceneFrameData.CameraPos = glm::vec4(camera->Position, 1.f);
        m_SceneFrameData.VP = vp;

        if (m_DrawDebugCameraLines && m_DrawDebugLines) {
            auto activeCamera = scene->GetActiveCamera();
            DrawDebugCameraLines(activeCamera);
            m_DrawDebugCameraLines = false;
        }

        auto* renderer = VulkanRenderer::Get();
        auto* window = renderer->GetVulkanWindow();
        auto frameIndex = window->FrameIndex;

        std::thread t(&SceneRenderer::SortLightsInTiles, this, scene, camera);

        std::vector<DrawData> drawData;
        {
            auto view = scene->m_Registry.view<TransformComponent, StaticMeshComponent>();
            for (auto [entity, transform, staticMesh] : view.each()) {
                auto t = transform.GetTransform();
                auto invT = glm::inverse(t);
                drawData.emplace_back(DrawData{ &staticMesh, t, invT });
            }
        }

        waitSemaphore = ShadowPass(waitSemaphore, drawData, t, scene, camera);

        waitSemaphore = MainPass(waitSemaphore, drawData, scene, camera, cameraProjection, cameraView, physicDrawer);

        waitSemaphore = AlphaClearPass(waitSemaphore);

        SceneRenderFinishedData renderFinishedData{};
        renderFinishedData.Texture = m_TextureFrameBuffers[frameIndex].VKTexture.DesctriptorSet;
        renderFinishedData.SceneRenderFinishedSemaphore = waitSemaphore;

        return renderFinishedData;
    }

    VkSemaphore SceneRenderer::ShadowPass(VkSemaphore waitSemaphore, const std::vector<DrawData>& drawData, std::thread& lightSortThread, Scene* scene, CameraComponent* camera)
    {
        uint32_t SHADOW_MAP_CASCADE_COUNT = m_SceneFrameData.CascadeCount;
        static constexpr float cascadeSplitLambda = 0.95f;

        std::vector<float> cascadeSplits(SHADOW_MAP_CASCADE_COUNT);

        float nearClip = camera->Near;
        float farClip = m_ShadowFarClip;
        float clipRange = farClip - nearClip;

        float minZ = nearClip;
        float maxZ = nearClip + clipRange;

        float range = maxZ - minZ;
        float ratio = maxZ / minZ;

        // Calculate split depths based on view camera frustum
        // Based on method presented in https://developer.nvidia.com/gpugems/GPUGems3/gpugems3_ch10.html
        for (uint32_t i = 0; i < SHADOW_MAP_CASCADE_COUNT; i++) {
            float p = (i + 1) / static_cast<float>(SHADOW_MAP_CASCADE_COUNT);
            float log = minZ * std::pow(ratio, p);
            float uniform = minZ + range * p;
            float d = cascadeSplitLambda * (log - uniform) + uniform;
            cascadeSplits[i] = (d - nearClip) / clipRange;
        }

        // Calculate orthographic projection matrix for each cascade
        float lastSplitDist = 0.0;
        glm::mat4 VP = glm::perspective(camera->FOV, camera->Aspect, camera->Near, farClip) * camera->GetView();
        glm::mat4 invCam = glm::inverse(VP);
        for (uint32_t i = 0; i < SHADOW_MAP_CASCADE_COUNT; i++) {
            float splitDist = cascadeSplits[i];

            glm::vec3 frustumCorners[8] = {
                glm::vec3(-1.0f,  1.0f, -1.0f),
                glm::vec3(1.0f,  1.0f, -1.0f),
                glm::vec3(1.0f, -1.0f, -1.0f),
                glm::vec3(-1.0f, -1.0f, -1.0f),
                glm::vec3(-1.0f,  1.0f,  1.0f),
                glm::vec3(1.0f,  1.0f,  1.0f),
                glm::vec3(1.0f, -1.0f,  1.0f),
                glm::vec3(-1.0f, -1.0f,  1.0f),
            };

            // Project frustum corners into world space
            for (uint32_t i = 0; i < 8; i++) {
                glm::vec4 invCorner = invCam * glm::vec4(frustumCorners[i], 1.0f);
                frustumCorners[i] = invCorner / invCorner.w;
            }

            for (uint32_t i = 0; i < 4; i++) {
                glm::vec3 dist = frustumCorners[i + 4] - frustumCorners[i];
                frustumCorners[i + 4] = frustumCorners[i] + (dist * splitDist);
                frustumCorners[i] = frustumCorners[i] + (dist * lastSplitDist);
            }

            // Get frustum center
            glm::vec3 frustumCenter = glm::vec3(0.0f);
            for (uint32_t i = 0; i < 8; i++) {
                frustumCenter += frustumCorners[i];
            }
            frustumCenter /= 8.0f;

            float radius = 0.0f;
            for (uint32_t i = 0; i < 8; i++) {
                float distance = glm::length(frustumCorners[i] - frustumCenter);
                radius = glm::max(radius, distance);
            }
            radius = std::ceil(radius * 16.0f) / 16.0f;

            glm::vec3 maxExtents = glm::vec3(radius);
            glm::vec3 minExtents = -maxExtents;

            glm::vec3 lightDir = glm::normalize(-m_SceneFrameData.SunPosition);
            glm::vec3 eye = frustumCenter - (lightDir * -minExtents.z);
            glm::mat4 lightViewMatrix = glm::lookAt(eye, frustumCenter, glm::vec3(0.0f, 1.0f, 0.0f));
            glm::mat4 lightOrthoMatrix = glm::ortho(minExtents.x, maxExtents.x, minExtents.y, maxExtents.y, -m_ShadowZExtentOffset, maxExtents.z - minExtents.z);

            // Store split distance and matrix in cascade
            m_SceneFrameData.SplitDists[i] = (camera->Near + splitDist * clipRange) * -1.f;
            m_SceneFrameData.LightSpaceMatrix[i] = lightOrthoMatrix * lightViewMatrix;

            lastSplitDist = cascadeSplits[i];
        }

        auto* renderer = VulkanRenderer::Get();
        auto* vulkanWindow = renderer->GetVulkanWindow();
        int frameIndex = vulkanWindow->FrameIndex;
        auto renderPass = m_ShadowDepthBuffers[frameIndex]->GetRenderPass();
        auto extent = m_ShadowDepthBuffers[frameIndex]->GetExtent();

        std::vector<VkCommandBuffer> commandBuffers(FLOOF_CASCADE_COUNT);
        for (auto& commandBuffer : commandBuffers) {
            commandBuffer = renderer->AllocateCommandBuffer();
            renderer->BeginSingleUseCommandBuffer(commandBuffer);
        }

        VkClearValue clearValue{};
        clearValue.depthStencil = { 1.f, 0 };

        m_SceneDataUBO[frameIndex].Update(m_SceneFrameData);

        for (uint32_t i = 0; i < m_SceneFrameData.CascadeCount; i++) {
            auto framebuffer = m_ShadowDepthBuffers[frameIndex]->GetFramebuffer(i);
            auto commandBuffer = commandBuffers[i];

            VkRenderPassBeginInfo renderPassInfo = { VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };
            renderPassInfo.renderPass = renderPass;
            renderPassInfo.framebuffer = framebuffer;
            renderPassInfo.renderArea.extent = extent;
            renderPassInfo.clearValueCount = 1;
            renderPassInfo.pClearValues = &clearValue;

            renderer->StartRenderPass(commandBuffer, &renderPassInfo);

            //vkCmdSetDepthBias(commandBuffer, depthBiasConstant, 0.0f, depthBiasSlope);

            auto pipelineLayout = renderer->BindGraphicsPipeline(commandBuffer, RenderPipelineKeys::ShadowPass);

            auto sceneDescriptor = m_SceneDataUBO[frameIndex].GetDescriptorSet();

            vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1,
                &sceneDescriptor, 0, nullptr);
            {

                for (auto& [staticMesh, transform, invTransform] : drawData) {
                    DepthPushConstants constants;
                    constants.Model = transform;
                    constants.InvModelMat = invTransform;
                    constants.cascadeIndex = i;
                    vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT,
                        0, sizeof(DepthPushConstants), &constants);
                    for (auto& mesh : staticMesh->meshes) {
                        VkDeviceSize offset{ 0 };
                        if (mesh.MeshMaterial.DescriptorSet != VK_NULL_HANDLE) {
                            vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout,
                                1, 1, &mesh.MeshMaterial.DescriptorSet, 0, nullptr);
                        }
                        vkCmdBindVertexBuffers(commandBuffer, 0, 1, &mesh.VertexBuffer.Buffer, &offset);
                        if (mesh.IndexBuffer.Buffer != VK_NULL_HANDLE) {
                            vkCmdBindIndexBuffer(commandBuffer, mesh.IndexBuffer.Buffer, 0, VK_INDEX_TYPE_UINT32);
                            vkCmdDrawIndexed(commandBuffer, mesh.IndexCount,
                                1, 0, 0, 0);
                        } else {
                            vkCmdDraw(commandBuffer, mesh.VertexCount, 1, 0, 0);
                        }
                    }
                }
            }
            {
                auto view = scene->m_Registry.view<TransformComponent, LandscapeComponent>();
                for (auto [entity, transform, landscape] : view.each()) {
                    if (landscape.TestMaterial.DescriptorSet == VK_NULL_HANDLE) {
                        landscape.TestMaterial.UpdateDescriptorSet();
                    }
                    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout,
                        1, 1, &landscape.TestMaterial.DescriptorSet, 0, nullptr);
                    DepthPushConstants constants;
                    glm::mat4 modelMat = transform.GetTransform();
                    constants.Model = modelMat;
                    constants.InvModelMat = glm::inverse(modelMat);
                    constants.cascadeIndex = i;
                    vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT,
                        0, sizeof(DepthPushConstants), &constants);
                    VkDeviceSize offset{ 0 };
                    vkCmdBindVertexBuffers(commandBuffer, 0, 1, &landscape.meshData.VertexBuffer.Buffer, &offset);
                    vkCmdBindIndexBuffer(commandBuffer, landscape.meshData.IndexBuffer.Buffer, 0, VK_INDEX_TYPE_UINT32);
                    vkCmdDrawIndexed(commandBuffer, landscape.meshData.IndexCount, 1, 0, 0, 0);
                }
            }
            vkCmdEndRenderPass(commandBuffer);
            VkResult endResult = vkEndCommandBuffer(commandBuffer);
            ASSERT(endResult == VK_SUCCESS);
        }

        VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = &waitSemaphore;
        submitInfo.pWaitDstStageMask = waitStages;
        submitInfo.commandBufferCount = commandBuffers.size();
        submitInfo.pCommandBuffers = commandBuffers.data();
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = &m_ShadowPassSignalSemaphores[frameIndex];

        // need to sync sorting thread here since mapped gpu-cpu buffers gets synced on queue submit.
        lightSortThread.join();

        renderer->QueueSubmitGraphics(1, &submitInfo);

        return m_ShadowPassSignalSemaphores[frameIndex];
    }

    struct AlphaDrawData {
        MeshData* mesh; 
        glm::mat4 transform;
        glm::mat4 invTransform;
        float distanceToCamera2;
    };

    VkSemaphore SceneRenderer::MainPass(VkSemaphore waitSemaphore, const std::vector<DrawData>& drawData, Scene* scene, CameraComponent* camera, const glm::mat4& cameraProjection, const glm::mat4& cameraView, PhysicsDebugDraw* physicDrawer)
    {
        auto* renderer = VulkanRenderer::Get();
        auto* window = renderer->GetVulkanWindow();
        int frameIndex = window->FrameIndex;
        auto& frameData = window->Frames[frameIndex];
        auto signalSemaphore = m_MainPassSignalSemaphores[frameIndex];
        auto commandBuffer = renderer->AllocateCommandBuffer();
        renderer->BeginSingleUseCommandBuffer(commandBuffer);

        VkExtent2D vkExtent = { static_cast<uint32_t>(m_Extent.x), static_cast<uint32_t>(m_Extent.y) };

        VkRenderPassBeginInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = m_RenderPass;
        renderPassInfo.framebuffer = m_OffscreenFrameBuffer.FrameBuffer;
        renderPassInfo.renderArea.offset = { 0, 0 };
        renderPassInfo.renderArea.extent = vkExtent;

        VkClearValue clearColors[2]{};
        clearColors[0].color = {};
        clearColors[0].color.float32[0] = 0.f;
        clearColors[0].color.float32[1] = 0.14f;
        clearColors[0].color.float32[2] = 0.28;
        clearColors[0].color.float32[3] = 1.f;
        clearColors[1].depthStencil = { 1.0f, 0 };

        renderPassInfo.clearValueCount = 2;
        renderPassInfo.pClearValues = clearColors;

        renderer->StartRenderPass(commandBuffer, &renderPassInfo);

        if (m_Skybox) {
            auto pipelineLayout = renderer->BindGraphicsPipeline(commandBuffer, RenderPipelineKeys::Skybox);

            SkyPushConstants constants;
            glm::mat4 modelMat = glm::mat4(1.f);
            constants.VP = cameraProjection * glm::mat4(glm::mat3(cameraView));
            constants.Model = modelMat;
            constants.InvModelMat = glm::inverse(modelMat);
            vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT,
                0, sizeof(SkyPushConstants), &constants);

            m_Skybox->Draw(commandBuffer, pipelineLayout);
        }

        std::vector<AlphaDrawData> transparentGeometry;

        // Draw models
        {
            auto pipelineLayout = renderer->BindGraphicsPipeline(commandBuffer, m_DrawMode);
            auto lightDescriptor = m_LightSSBO[frameIndex].GetDescriptorSet();
            if (m_DrawMode == RenderPipelineKeys::PBR) {
                vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 2, 1,
                    &lightDescriptor, 0, nullptr);

                auto sceneDescriptor = m_SceneDataUBO[frameIndex].GetDescriptorSet();

                vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 1, 1,
                    &sceneDescriptor, 0, nullptr);

                vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 3, 1,
                    &m_IrradiencePrefilterDescriptorSet, 0, nullptr);

                vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 4, 1,
                    &m_BRDFLut.DesctriptorSet, 0, nullptr);

                auto shadowDescriptor = m_ShadowDepthBuffers[frameIndex]->GetDescriptorSet();
                vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 5, 1,
                    &shadowDescriptor, 0, nullptr);

                auto lightCountDescriptor = m_LightCountsSSBO[frameIndex].GetDescriptorSet();
                vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 6, 1,
                    &lightCountDescriptor, 0, nullptr);

                auto lightOffsetDescriptor = m_LightOffsetsSSBO[frameIndex].GetDescriptorSet();
                vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 7, 1,
                    &lightOffsetDescriptor, 0, nullptr);
            }
            if (m_DrawMode == RenderPipelineKeys::Wireframe || m_DrawMode == RenderPipelineKeys::UV) {
                auto sceneDescriptor = m_SceneDataUBO[frameIndex].GetDescriptorSet();

                vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1,
                    &sceneDescriptor, 0, nullptr);
            }
            if (m_DrawMode == RenderPipelineKeys::Normals || m_DrawMode == RenderPipelineKeys::UnLit) {
                auto sceneDescriptor = m_SceneDataUBO[frameIndex].GetDescriptorSet();

                vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 1, 1,
                    &sceneDescriptor, 0, nullptr);
            }
            {
                for (auto [staticMesh, transform, invTransform] : drawData) {
                    MeshPushConstants constants;
                    constants.Model = transform;
                    constants.InvModelMat = invTransform;
                    vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT,
                        0, sizeof(MeshPushConstants), &constants);
                    for (auto& mesh : staticMesh->meshes) {
                        if (mesh.MeshMaterial.HasOpacity) {
                            AlphaDrawData alphaDrawData;
                            alphaDrawData.mesh = &mesh;
                            alphaDrawData.transform = transform;
                            alphaDrawData.invTransform = invTransform;
                            alphaDrawData.distanceToCamera2 = glm::length2(camera->Position - glm::vec3(transform[3]));
                            transparentGeometry.push_back(alphaDrawData);
                            continue;
                        }
                        if (m_DrawMode == RenderPipelineKeys::PBR || m_DrawMode == RenderPipelineKeys::UnLit || m_DrawMode == RenderPipelineKeys::Normals) {
                            if (mesh.MeshMaterial.DescriptorSet != VK_NULL_HANDLE) {
                                vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout,
                                    0, 1, &mesh.MeshMaterial.DescriptorSet, 0, nullptr);
                            }
                        }
                        VkDeviceSize offset{ 0 };
                        vkCmdBindVertexBuffers(commandBuffer, 0, 1, &mesh.VertexBuffer.Buffer, &offset);
                        if (mesh.IndexBuffer.Buffer != VK_NULL_HANDLE) {
                            vkCmdBindIndexBuffer(commandBuffer, mesh.IndexBuffer.Buffer, 0, VK_INDEX_TYPE_UINT32);
                            vkCmdDrawIndexed(commandBuffer, mesh.IndexCount,
                                1, 0, 0, 0);
                        } else {
                            vkCmdDraw(commandBuffer, mesh.VertexCount, 1, 0, 0);
                        }
                    }
                }
            }
        }

        // Draw landscape
        {
            auto pipelineLayout = renderer->BindGraphicsPipeline(commandBuffer, RenderPipelineKeys::Landscape);
            auto lightDescriptor = m_LightSSBO[frameIndex].GetDescriptorSet();

            vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 2, 1,
                &lightDescriptor, 0, nullptr);

            auto sceneDescriptor = m_SceneDataUBO[frameIndex].GetDescriptorSet();

            vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 1, 1,
                &sceneDescriptor, 0, nullptr);

            vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 3, 1,
                &m_IrradiencePrefilterDescriptorSet, 0, nullptr);

            vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 4, 1,
                &m_BRDFLut.DesctriptorSet, 0, nullptr);

            auto shadowDescriptor = m_ShadowDepthBuffers[frameIndex]->GetDescriptorSet();

            vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 7, 1,
                &shadowDescriptor, 0, nullptr);

            auto lightCountDescriptor = m_LightCountsSSBO[frameIndex].GetDescriptorSet();
            vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 8, 1,
                &lightCountDescriptor, 0, nullptr);

            auto lightOffsetDescriptor = m_LightOffsetsSSBO[frameIndex].GetDescriptorSet();
            vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 9, 1,
                &lightOffsetDescriptor, 0, nullptr);

            auto view = scene->m_Registry.view<TransformComponent, LandscapeComponent>();
            for (auto [entity, transform, landscape] : view.each()) {
                MeshPushConstants constants;
                glm::mat4 modelMat = transform.GetTransform();
                constants.Model = modelMat;
                constants.InvModelMat = glm::inverse(modelMat);
                vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT,
                    0, sizeof(MeshPushConstants), &constants);

                if (landscape.meshData.MeshMaterial1.DescriptorSet == VK_NULL_HANDLE) {
                    landscape.meshData.MeshMaterial1.UpdateDescriptorSet();
                }
                if (landscape.meshData.MeshMaterial2.DescriptorSet == VK_NULL_HANDLE) {
                    landscape.meshData.MeshMaterial2.UpdateDescriptorSet();
                }
                if (landscape.meshData.MeshMaterial3.DescriptorSet == VK_NULL_HANDLE) {
                    landscape.meshData.MeshMaterial3.UpdateDescriptorSet();
                }

                vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout,
                    0, 1, &landscape.meshData.MeshMaterial1.DescriptorSet, 0, nullptr);
                vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout,
                    5, 1, &landscape.meshData.MeshMaterial2.DescriptorSet, 0, nullptr);
                vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout,
                    6, 1, &landscape.meshData.MeshMaterial3.DescriptorSet, 0, nullptr);

                VkDeviceSize offset{ 0 };
                vkCmdBindVertexBuffers(commandBuffer, 0, 1, &landscape.meshData.VertexBuffer.Buffer, &offset);
                vkCmdBindIndexBuffer(commandBuffer, landscape.meshData.IndexBuffer.Buffer, 0, VK_INDEX_TYPE_UINT32);
                vkCmdDrawIndexed(commandBuffer, landscape.meshData.IndexCount, 1, 0, 0, 0);
            }
        }
        {
            auto sortFunc = [](const AlphaDrawData& a, const AlphaDrawData& b) -> bool {
                return a.distanceToCamera2 > b.distanceToCamera2;
            };
            std::sort(transparentGeometry.begin(), transparentGeometry.end(), sortFunc);

            auto pipelineLayout = renderer->BindGraphicsPipeline(commandBuffer, RenderPipelineKeys::PBRAlpha);
            auto lightDescriptor = m_LightSSBO[frameIndex].GetDescriptorSet();

            vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 2, 1,
                &lightDescriptor, 0, nullptr);

            auto sceneDescriptor = m_SceneDataUBO[frameIndex].GetDescriptorSet();

            vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 1, 1,
                &sceneDescriptor, 0, nullptr);

            vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 3, 1,
                &m_IrradiencePrefilterDescriptorSet, 0, nullptr);

            vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 4, 1,
                &m_BRDFLut.DesctriptorSet, 0, nullptr);

            auto shadowDescriptor = m_ShadowDepthBuffers[frameIndex]->GetDescriptorSet();

            vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 5, 1,
                &shadowDescriptor, 0, nullptr);

            auto lightCountDescriptor = m_LightCountsSSBO[frameIndex].GetDescriptorSet();
            vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 6, 1,
                &lightCountDescriptor, 0, nullptr);

            auto lightOffsetDescriptor = m_LightOffsetsSSBO[frameIndex].GetDescriptorSet();
            vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 7, 1,
                &lightOffsetDescriptor, 0, nullptr);

            for (auto& drawData : transparentGeometry) {
                MeshPushConstants constants;
                constants.Model = drawData.transform;
                constants.InvModelMat = glm::inverse(drawData.transform);
                vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT,
                    0, sizeof(MeshPushConstants), &constants);
                if (drawData.mesh->MeshMaterial.DescriptorSet != VK_NULL_HANDLE) {
                    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout,
                        0, 1, &drawData.mesh->MeshMaterial.DescriptorSet, 0, nullptr);
                }
                VkDeviceSize offset{ 0 };
                vkCmdBindVertexBuffers(commandBuffer, 0, 1, &drawData.mesh->VertexBuffer.Buffer, &offset);
                if (drawData.mesh->IndexBuffer.Buffer != VK_NULL_HANDLE) {
                    vkCmdBindIndexBuffer(commandBuffer, drawData.mesh->IndexBuffer.Buffer, 0, VK_INDEX_TYPE_UINT32);
                    vkCmdDrawIndexed(commandBuffer, drawData.mesh->IndexCount,
                        1, 0, 0, 0);
                } else {
                    vkCmdDraw(commandBuffer, drawData.mesh->VertexCount, 1, 0, 0);
                }
            }
        }

        // Draw debug lines
        {
            auto vp = cameraProjection * cameraView;
            if (physicDrawer) {
                auto pipelineLayout = renderer->BindGraphicsPipeline(commandBuffer, RenderPipelineKeys::Line);
                auto* lineMesh = physicDrawer->GetUpdatedLineMesh();
                SkyPushConstants constants;
                constants.VP = vp;
                constants.Model = glm::mat4(1.f);
                constants.InvModelMat = glm::mat4(1.f);
                vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(SkyPushConstants),
                    &constants);
                lineMesh->Draw(commandBuffer);
            }
            {
                auto pipelineLayout = renderer->BindGraphicsPipeline(commandBuffer, RenderPipelineKeys::Line);
                auto* lineMesh = m_DebugLineMesh[frameIndex].get();
                lineMesh->UpdateBuffer(m_DebugVertexData);
                m_DebugVertexData.clear();
                SkyPushConstants constants;
                constants.VP = vp;
                constants.Model = glm::mat4(1.f);
                constants.InvModelMat = glm::mat4(1.f);
                vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(SkyPushConstants),
                    &constants);
                lineMesh->Draw(commandBuffer);
            }
        }

        // End main renderpass
        renderer->EndRenderPass(commandBuffer);

        VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = &waitSemaphore;
        submitInfo.pWaitDstStageMask = waitStages;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffer;
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = &signalSemaphore;

        renderer->QueueSubmitGraphics(1, &submitInfo);

        return signalSemaphore;
    }

    VkSemaphore SceneRenderer::AlphaClearPass(VkSemaphore waitSemaphore)
    {
        auto* renderer = VulkanRenderer::Get();
        auto* window = renderer->GetVulkanWindow();
        int frameIndex = window->FrameIndex;
        auto& frameData = window->Frames[frameIndex];
        auto signalSemaphore = m_AlphaClearPassSignalSemaphores[frameIndex];
        auto commandBuffer = renderer->AllocateCommandBuffer();
        renderer->BeginSingleUseCommandBuffer(commandBuffer);

        VkExtent2D vkExtent = { static_cast<uint32_t>(m_Extent.x), static_cast<uint32_t>(m_Extent.y) };

        VkRenderPassBeginInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = m_RenderPass;
        renderPassInfo.framebuffer = m_TextureFrameBuffers[frameIndex].FrameBuffer;
        renderPassInfo.renderArea.offset = { 0, 0 };
        renderPassInfo.renderArea.extent = vkExtent;

        // TODO: Dont need to clear. make renderpass for this.
        VkClearValue clearColors[2]{};
        clearColors[0].color = {};
        clearColors[0].color.float32[0] = 0.f;
        clearColors[0].color.float32[1] = 0.14f;
        clearColors[0].color.float32[2] = 0.28;
        clearColors[0].color.float32[3] = 1.f;
        clearColors[1].depthStencil = { 1.0f, 0 };

        renderPassInfo.clearValueCount = 2;
        renderPassInfo.pClearValues = clearColors;

        renderer->StartRenderPass(commandBuffer, &renderPassInfo);

        auto ndcRect = ModelManager::GetNDCRect();

        auto pipelineLayout = renderer->BindGraphicsPipeline(commandBuffer, RenderPipelineKeys::AlphaClear);

        VkDeviceSize offset{ 0 };
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, &ndcRect.Buffer, &offset);

        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1,
            &m_OffscreenFrameBuffer.VKTexture.DesctriptorSet, 0, nullptr);

        vkCmdDraw(commandBuffer, 4, 1, 0, 0);

        renderer->EndRenderPass(commandBuffer);

        VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = &waitSemaphore;
        submitInfo.pWaitDstStageMask = waitStages;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffer;
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = &signalSemaphore;

        renderer->QueueSubmitGraphics(1, &submitInfo);

        return signalSemaphore;
    }

    void SceneRenderer::SortLightsInTiles(Scene* scene, CameraComponent* camera)
    {
        auto invProj = glm::inverse(glm::perspective(camera->FOV, camera->Aspect, camera->Near, camera->Far) * camera->GetView());

        float tileSize = m_SceneFrameData.TileSize;

        glm::vec2 ndcTileSize = 2.f * glm::vec2(tileSize, tileSize) / m_Extent;

        std::vector<PointLightComponent::PointLight> tempPointLights;
        auto lightView = scene->m_Registry.view<TransformComponent, PointLightComponent>();
        {
            glm::vec3 frustumCorners[8] = {
                // near
                glm::vec3(-1.f, 1.f, -1.0f), // top left      0  
                glm::vec3(1.f, 1.f, -1.0f), // top right     1  
                glm::vec3(1.f, -1.f, -1.0f), // bottom right  2      
                glm::vec3(-1.f, -1.f, -1.0f), // bottom left   3     
                // far        
                glm::vec3(-1.f, 1.f, 0.99999f), // top left       4 
                glm::vec3(1.f, 1.f, 0.99999f), // top right      5  
                glm::vec3(1.f, -1.f, 0.99999f), // bottom right   6     
                glm::vec3(-1.f, -1.f, 0.99999f), // bottom left    7    
            };

            // Project frustum corners into world space
            for (uint32_t i = 0; i < 8; i++) {
                glm::vec4 invCorner = invProj * glm::vec4(frustumCorners[i], 1.0f);
                frustumCorners[i] = invCorner / invCorner.w;
            }

            std::pair<glm::vec3, glm::vec3> faces[6] = {
                    // near
                    { glm::vec3((frustumCorners[0] + frustumCorners[1] + frustumCorners[2] + frustumCorners[3]) / 4.f),
                        glm::vec3(camera->Forward) },
                    // left
                    { glm::vec3((frustumCorners[0] + frustumCorners[3] + frustumCorners[4] + frustumCorners[7]) / 4.f),
                        glm::vec3(glm::normalize(glm::cross(frustumCorners[7] - camera->Position, frustumCorners[4] - camera->Position)))  },
                    // right
                    { glm::vec3((frustumCorners[1] + frustumCorners[2] + frustumCorners[5] + frustumCorners[6]) / 4.f),
                        glm::vec3(glm::normalize(glm::cross(frustumCorners[5] - camera->Position, frustumCorners[6] - camera->Position))) },
                    // bottom
                    { glm::vec3((frustumCorners[2] + frustumCorners[2] + frustumCorners[6] + frustumCorners[7]) / 4.f),
                        glm::vec3(glm::normalize(glm::cross(frustumCorners[6] - camera->Position, frustumCorners[7] - camera->Position))) },
                    // top
                    { glm::vec3((frustumCorners[0] + frustumCorners[1] + frustumCorners[4] + frustumCorners[5]) / 4.f),
                        glm::vec3(glm::normalize(glm::cross(frustumCorners[4] - camera->Position, frustumCorners[5] - camera->Position))) },
                    // far
                    { glm::vec3((frustumCorners[4] + frustumCorners[5] + frustumCorners[6] + frustumCorners[7]) / 4.f),
                        glm::vec3(-camera->Forward) },
                };

            for (auto [entity, transform, lightComp] : lightView.each()) {
                PointLightComponent::PointLight light;
                light.position = glm::vec4(transform.GetWorldPosition(), 1.f);
                light.diffuse = lightComp.diffuse;
                light.intensity = lightComp.intensity;
                light.innerRange = lightComp.innerRange;
                light.outerRange = lightComp.outerRange;

                bool intersecting = true;
                for (auto& [pos, normal] : faces) {
                    float distance = glm::dot(glm::vec3(light.position) - pos, normal);
                    if (distance < -light.outerRange) {
                        intersecting = false;
                        break;
                    }
                }
                if (intersecting) {
                    tempPointLights.push_back(light);
                }

            }
        }

        std::vector<PointLightComponent::PointLight> pointLights;
        std::vector<int> lightCounts;
        std::vector<int> offsets;
        int offset{};

        int tileCountX = 0;
        for (float x = -1.f; x < 1.f; x += ndcTileSize.x) {
            tileCountX++;
        }
        m_SceneFrameData.TileCountX = tileCountX;

        int numTiles = 0;
        for (float y = -1.f; y < 1.f; y += ndcTileSize.y) {
            for (float x = -1.f; x < 1.f; x += ndcTileSize.x) {
                numTiles++;
            }
        }

        std::vector<std::future<void>> futures(numTiles);
        std::vector<std::vector<PointLightComponent::PointLight>> pointLightVectors(numTiles);

        int tileIndex = 0;
        for (float y = -1.f; y < 1.f; y += ndcTileSize.y) {
            for (float x = -1.f; x < 1.f; x += ndcTileSize.x) {
                futures[tileIndex] = std::async(std::launch::async, [x, y, ndcTileSize, camera, invProj](std::vector<PointLightComponent::PointLight>& pointLights,
                    std::vector<PointLightComponent::PointLight>& tempPointLights) {

                    glm::vec3 frustumCorners[8] = {
                    // near
                    glm::vec3(x,                 y + ndcTileSize.y, -1.0f), // top left      0  
                    glm::vec3(x + ndcTileSize.x, y + ndcTileSize.y, -1.0f), // top right     1  
                    glm::vec3(x + ndcTileSize.x, y,                 -1.0f), // bottom right  2      
                    glm::vec3(x,                 y,                 -1.0f), // bottom left   3     
                    // far        
                    glm::vec3(x,                 y + ndcTileSize.y, 0.99999f), // top left       4 
                    glm::vec3(x + ndcTileSize.x, y + ndcTileSize.y, 0.99999f), // top right      5  
                    glm::vec3(x + ndcTileSize.x, y,                 0.99999f), // bottom right   6     
                    glm::vec3(x,                 y,                 0.99999f), // bottom left    7    
                    };

                    // Project frustum corners into world space
                    for (uint32_t i = 0; i < 8; i++) {
                        glm::vec4 invCorner = invProj * glm::vec4(frustumCorners[i], 1.0f);
                        frustumCorners[i] = invCorner / invCorner.w;
                    }

                    // TODO: dont need to combine and divide by 4. can simply select a corner.
                    // did this just for nice debug visuals.
                    // <face position, face normal>
                    std::pair<glm::vec3, glm::vec3> faces[6] = {
                        // near
                        { glm::vec3((frustumCorners[0] + frustumCorners[1] + frustumCorners[2] + frustumCorners[3]) / 4.f),
                            glm::vec3(camera->Forward) },
                        // left
                        { glm::vec3((frustumCorners[0] + frustumCorners[3] + frustumCorners[4] + frustumCorners[7]) / 4.f),
                            glm::vec3(glm::normalize(glm::cross(frustumCorners[7] - camera->Position, frustumCorners[4] - camera->Position)))  },
                        // right
                        { glm::vec3((frustumCorners[1] + frustumCorners[2] + frustumCorners[5] + frustumCorners[6]) / 4.f),
                            glm::vec3(glm::normalize(glm::cross(frustumCorners[5] - camera->Position, frustumCorners[6] - camera->Position))) },
                        // bottom
                        { glm::vec3((frustumCorners[2] + frustumCorners[2] + frustumCorners[6] + frustumCorners[7]) / 4.f),
                            glm::vec3(glm::normalize(glm::cross(frustumCorners[6] - camera->Position, frustumCorners[7] - camera->Position))) },
                        // top
                        { glm::vec3((frustumCorners[0] + frustumCorners[1] + frustumCorners[4] + frustumCorners[5]) / 4.f),
                            glm::vec3(glm::normalize(glm::cross(frustumCorners[4] - camera->Position, frustumCorners[5] - camera->Position))) },
                        // far
                        { glm::vec3((frustumCorners[4] + frustumCorners[5] + frustumCorners[6] + frustumCorners[7]) / 4.f),
                            glm::vec3(-camera->Forward) },
                    };

                    int lightCount{};

                    for (auto& light : tempPointLights) {
                        bool intersecting = true;
                        for (auto& [pos, normal] : faces) {
                            float distance = glm::dot(glm::vec3(light.position) - pos, normal);
                            if (distance < -light.outerRange) {
                                intersecting = false;
                                break;
                            }
                        }
                        if (intersecting) {
                            lightCount++;
                            pointLights.push_back(light);
                        }
                    }

                    }, std::ref(pointLightVectors[tileIndex]), std::ref(tempPointLights));
                tileIndex++;
            }
        }

        for (auto& fut : futures)
            fut.get();

        for (auto& partialLightVec : pointLightVectors) {
            pointLights.insert(pointLights.end(), partialLightVec.begin(), partialLightVec.end());
            offsets.push_back(offset);
            auto lightCount = partialLightVec.size();
            lightCounts.push_back(lightCount);
            offset += lightCount;
        }

        // Update gpu data
        auto* renderer = VulkanRenderer::Get();
        auto* window = renderer->GetVulkanWindow();
        auto frameIndex = window->FrameIndex;

        m_LightCountsSSBO[frameIndex].Update(lightCounts);
        m_LightOffsetsSSBO[frameIndex].Update(offsets);
        m_LightSSBO[frameIndex].Update(pointLights);
    }

    void SceneRenderer::CreateTextureRenderer() {
        auto *renderer = VulkanRenderer::Get();
        m_TextureFrameBuffers.resize(VulkanGlobals::MAX_FRAMES_IN_FLIGHT);
        m_MainPassSignalSemaphores.resize(VulkanGlobals::MAX_FRAMES_IN_FLIGHT);
        m_ShadowPassSignalSemaphores.resize(VulkanGlobals::MAX_FRAMES_IN_FLIGHT);
        m_AlphaClearPassSignalSemaphores.resize(VulkanGlobals::MAX_FRAMES_IN_FLIGHT);
        m_ShadowDepthBuffers.resize(VulkanGlobals::MAX_FRAMES_IN_FLIGHT);
        m_SceneDataUBO.resize(VulkanGlobals::MAX_FRAMES_IN_FLIGHT);
        m_DebugLineMesh.resize(VulkanGlobals::MAX_FRAMES_IN_FLIGHT);
        m_LightSSBO.resize(VulkanGlobals::MAX_FRAMES_IN_FLIGHT);
        m_LightCountsSSBO.resize(VulkanGlobals::MAX_FRAMES_IN_FLIGHT);
        m_LightOffsetsSSBO.resize(VulkanGlobals::MAX_FRAMES_IN_FLIGHT);
        for (auto& shadowDB : m_ShadowDepthBuffers) {
            shadowDB = std::make_unique<DepthFramebuffer>(m_ShadowRes, m_ShadowRes, m_SceneFrameData.CascadeCount);
        }
        for (auto& lineMesh : m_DebugLineMesh) {
            std::vector<ColorVertex> vertexData(100000);
            lineMesh = std::make_unique<LineMeshComponent>(vertexData);
        }

        CreateSyncObjects();

        CreateRenderPass();
        {    // Alpha clear shader
            RenderPipelineParams params;
            params.Flags = RenderPipelineFlags::AlphaBlend;
            params.FragmentPath = "Shaders/AlphaClear.frag.spv";
            params.VertexPath = "Shaders/BRDF.vert.spv";
            params.Key = RenderPipelineKeys::AlphaClear;
            params.PolygonMode = VK_POLYGON_MODE_FILL;
            params.Topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
            params.BindingDescription = MeshVertex::GetBindingDescription();
            params.AttributeDescriptions = MeshVertex::GetAttributeDescriptions();
            params.PushConstantSize = sizeof(SkyPushConstants);
            params.DescriptorSetLayoutBindings.resize(1);
            params.DescriptorSetLayoutBindings[0] = renderer->GetDescriptorSetLayout(RenderSetLayouts::FontTexture);
            params.Renderpass = m_RenderPass;
            params.MsaaSampleCount = renderer->GetMsaaSampleCount();
            renderer->CreateGraphicsPipeline(params);
        }
        {    // Shadow shader
            RenderPipelineParams params;
            params.Flags = RenderPipelineFlags::AlphaBlend | RenderPipelineFlags::DepthPass;
            params.FragmentPath = "Shaders/Shadow.frag.spv";
            params.VertexPath = "Shaders/Shadow.vert.spv";
            params.Key = RenderPipelineKeys::ShadowPass;
            params.PolygonMode = VK_POLYGON_MODE_FILL;
            params.Topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
            params.BindingDescription = MeshVertex::GetBindingDescription();
            params.AttributeDescriptions = MeshVertex::GetAttributeDescriptions();
            params.PushConstantSize = sizeof(DepthPushConstants);
            params.Renderpass = m_ShadowDepthBuffers[0]->GetRenderPass();
            params.DescriptorSetLayoutBindings.resize(2);
            params.DescriptorSetLayoutBindings[0] = renderer->m_DescriptorSetLayouts[RenderSetLayouts::SceneFrameUBO];
            params.DescriptorSetLayoutBindings[1] = renderer->m_DescriptorSetLayouts[RenderSetLayouts::Material];
            renderer->CreateGraphicsPipeline(params);
        }
        {    // PBR shader
            RenderPipelineParams params;
            params.Flags = RenderPipelineFlags::AlphaBlend | RenderPipelineFlags::DepthPass;
            params.FragmentPath = "Shaders/PBR.frag.spv";
            params.VertexPath = "Shaders/PBR.vert.spv";
            params.Key = RenderPipelineKeys::PBR;
            params.PolygonMode = VK_POLYGON_MODE_FILL;
            params.Topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
            params.BindingDescription = MeshVertex::GetBindingDescription();
            params.AttributeDescriptions = MeshVertex::GetAttributeDescriptions();
            params.PushConstantSize = sizeof(MeshPushConstants);
            params.DescriptorSetLayoutBindings.resize(8);
            params.DescriptorSetLayoutBindings[0] = renderer->m_DescriptorSetLayouts[RenderSetLayouts::Material];
            params.DescriptorSetLayoutBindings[1] = renderer->m_DescriptorSetLayouts[RenderSetLayouts::SceneFrameUBO];
            params.DescriptorSetLayoutBindings[2] = renderer->m_DescriptorSetLayouts[RenderSetLayouts::LightSSBO];
            params.DescriptorSetLayoutBindings[3] = renderer->m_DescriptorSetLayouts[RenderSetLayouts::PBRMaps];
            params.DescriptorSetLayoutBindings[4] = renderer->m_DescriptorSetLayouts[RenderSetLayouts::DiffuseTextureClamped];
            params.DescriptorSetLayoutBindings[5] = renderer->m_DescriptorSetLayouts[RenderSetLayouts::DepthTexture];
            params.DescriptorSetLayoutBindings[6] = renderer->m_DescriptorSetLayouts[RenderSetLayouts::LightSSBO];
            params.DescriptorSetLayoutBindings[7] = renderer->m_DescriptorSetLayouts[RenderSetLayouts::LightSSBO];
            params.Renderpass = m_RenderPass;
            params.MsaaSampleCount = renderer->GetMsaaSampleCount();
            renderer->CreateGraphicsPipeline(params);
        }
        {    // PBR alpha shader
            RenderPipelineParams params;
            params.Flags = RenderPipelineFlags::AlphaBlend | RenderPipelineFlags::DepthRead;
            params.FragmentPath = "Shaders/PBR.frag.spv";
            params.VertexPath = "Shaders/PBR.vert.spv";
            params.Key = RenderPipelineKeys::PBRAlpha;
            params.PolygonMode = VK_POLYGON_MODE_FILL;
            params.Topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
            params.BindingDescription = MeshVertex::GetBindingDescription();
            params.AttributeDescriptions = MeshVertex::GetAttributeDescriptions();
            params.PushConstantSize = sizeof(MeshPushConstants);
            params.DescriptorSetLayoutBindings.resize(8);
            params.DescriptorSetLayoutBindings[0] = renderer->m_DescriptorSetLayouts[RenderSetLayouts::Material];
            params.DescriptorSetLayoutBindings[1] = renderer->m_DescriptorSetLayouts[RenderSetLayouts::SceneFrameUBO];
            params.DescriptorSetLayoutBindings[2] = renderer->m_DescriptorSetLayouts[RenderSetLayouts::LightSSBO];
            params.DescriptorSetLayoutBindings[3] = renderer->m_DescriptorSetLayouts[RenderSetLayouts::PBRMaps];
            params.DescriptorSetLayoutBindings[4] = renderer->m_DescriptorSetLayouts[RenderSetLayouts::DiffuseTextureClamped];
            params.DescriptorSetLayoutBindings[5] = renderer->m_DescriptorSetLayouts[RenderSetLayouts::DepthTexture];
            params.DescriptorSetLayoutBindings[6] = renderer->m_DescriptorSetLayouts[RenderSetLayouts::LightSSBO];
            params.DescriptorSetLayoutBindings[7] = renderer->m_DescriptorSetLayouts[RenderSetLayouts::LightSSBO];
            params.Renderpass = m_RenderPass;
            params.CullMode = VK_CULL_MODE_NONE;
            params.MsaaSampleCount = renderer->GetMsaaSampleCount();
            renderer->CreateGraphicsPipeline(params);
        }
        {    // Landscape shader
            RenderPipelineParams params;
            params.Flags = RenderPipelineFlags::AlphaBlend | RenderPipelineFlags::DepthPass;
            params.FragmentPath = "Shaders/Landscape.frag.spv";
            params.VertexPath = "Shaders/Landscape.vert.spv";
            params.Key = RenderPipelineKeys::Landscape;
            params.PolygonMode = VK_POLYGON_MODE_FILL;
            params.Topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
            params.BindingDescription = MeshVertex::GetBindingDescription();
            params.AttributeDescriptions = MeshVertex::GetAttributeDescriptions();
            params.PushConstantSize = sizeof(MeshPushConstants);
            params.DescriptorSetLayoutBindings.resize(10);
            params.DescriptorSetLayoutBindings[0] = renderer->m_DescriptorSetLayouts[RenderSetLayouts::LandscapeMaterial];
            params.DescriptorSetLayoutBindings[1] = renderer->m_DescriptorSetLayouts[RenderSetLayouts::SceneFrameUBO];
            params.DescriptorSetLayoutBindings[2] = renderer->m_DescriptorSetLayouts[RenderSetLayouts::LightSSBO];
            params.DescriptorSetLayoutBindings[3] = renderer->m_DescriptorSetLayouts[RenderSetLayouts::PBRMaps];
            params.DescriptorSetLayoutBindings[4] = renderer->m_DescriptorSetLayouts[RenderSetLayouts::DiffuseTextureClamped];
            params.DescriptorSetLayoutBindings[5] = renderer->m_DescriptorSetLayouts[RenderSetLayouts::LandscapeMaterial];
            params.DescriptorSetLayoutBindings[6] = renderer->m_DescriptorSetLayouts[RenderSetLayouts::LandscapeMaterial];
            params.DescriptorSetLayoutBindings[7] = renderer->m_DescriptorSetLayouts[RenderSetLayouts::DepthTexture];
            params.DescriptorSetLayoutBindings[8] = renderer->m_DescriptorSetLayouts[RenderSetLayouts::LightSSBO];
            params.DescriptorSetLayoutBindings[9] = renderer->m_DescriptorSetLayouts[RenderSetLayouts::LightSSBO];
            params.Renderpass = m_RenderPass;
            params.MsaaSampleCount = renderer->GetMsaaSampleCount();
            renderer->CreateGraphicsPipeline(params);
        }
        {    // Skybox shader
            RenderPipelineParams params;
            params.Flags = RenderPipelineFlags::AlphaBlend;
            params.FragmentPath = "Shaders/Skybox.frag.spv";
            params.VertexPath = "Shaders/Skybox.vert.spv";
            params.Key = RenderPipelineKeys::Skybox;
            params.PolygonMode = VK_POLYGON_MODE_FILL;
            params.Topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
            params.BindingDescription = SimpleVertex::GetBindingDescription();
            params.AttributeDescriptions = SimpleVertex::GetAttributeDescriptions();
            params.PushConstantSize = sizeof(SkyPushConstants);
            params.DescriptorSetLayoutBindings.resize(1);
            params.DescriptorSetLayoutBindings[0] = renderer->m_DescriptorSetLayouts[RenderSetLayouts::DiffuseTextureClamped];
            params.Renderpass = m_RenderPass;
            params.MsaaSampleCount = renderer->GetMsaaSampleCount();
            renderer->CreateGraphicsPipeline(params);
        }
        {    // Wireframe
            RenderPipelineParams params;
            params.Flags = RenderPipelineFlags::AlphaBlend;
            params.FragmentPath = "Shaders/Wireframe.frag.spv";
            params.VertexPath = "Shaders/Wireframe.vert.spv";
            params.Key = RenderPipelineKeys::Wireframe;
            params.PolygonMode = VK_POLYGON_MODE_LINE;
            params.Topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
            params.BindingDescription = MeshVertex::GetBindingDescription();
            params.AttributeDescriptions = MeshVertex::GetAttributeDescriptions();
            params.PushConstantSize = sizeof(MeshPushConstants);
            params.Renderpass = m_RenderPass;
            params.MsaaSampleCount = renderer->GetMsaaSampleCount();
            params.DescriptorSetLayoutBindings.resize(1);
            params.DescriptorSetLayoutBindings[0] = renderer->m_DescriptorSetLayouts[RenderSetLayouts::SceneFrameUBO];
            renderer->CreateGraphicsPipeline(params);
        }
        {    // Normals
            RenderPipelineParams params;
            params.Flags = RenderPipelineFlags::AlphaBlend | RenderPipelineFlags::DepthPass;
            params.FragmentPath = "Shaders/Normals.frag.spv";
            params.VertexPath = "Shaders/Normals.vert.spv";
            params.Key = RenderPipelineKeys::Normals;
            params.PolygonMode = VK_POLYGON_MODE_FILL;
            params.Topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
            params.BindingDescription = MeshVertex::GetBindingDescription();
            params.AttributeDescriptions = MeshVertex::GetAttributeDescriptions();
            params.PushConstantSize = sizeof(MeshPushConstants);
            params.Renderpass = m_RenderPass;
            params.DescriptorSetLayoutBindings.resize(2);
            params.DescriptorSetLayoutBindings[0] = renderer->m_DescriptorSetLayouts[RenderSetLayouts::Material];
            params.DescriptorSetLayoutBindings[1] = renderer->m_DescriptorSetLayouts[RenderSetLayouts::SceneFrameUBO];
            params.MsaaSampleCount = renderer->GetMsaaSampleCount();
            renderer->CreateGraphicsPipeline(params);
        }
        {    // UV
            RenderPipelineParams params;
            params.Flags = RenderPipelineFlags::AlphaBlend | RenderPipelineFlags::DepthPass;
            params.FragmentPath = "Shaders/UV.frag.spv";
            params.VertexPath = "Shaders/UV.vert.spv";
            params.Key = RenderPipelineKeys::UV;
            params.PolygonMode = VK_POLYGON_MODE_FILL;
            params.Topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
            params.BindingDescription = MeshVertex::GetBindingDescription();
            params.AttributeDescriptions = MeshVertex::GetAttributeDescriptions();
            params.PushConstantSize = sizeof(MeshPushConstants);
            params.Renderpass = m_RenderPass;
            params.MsaaSampleCount = renderer->GetMsaaSampleCount();
            params.DescriptorSetLayoutBindings.resize(1);
            params.DescriptorSetLayoutBindings[0] = renderer->m_DescriptorSetLayouts[RenderSetLayouts::SceneFrameUBO];
            renderer->CreateGraphicsPipeline(params);
        }
        {    // UnLit
            RenderPipelineParams params;
            params.Flags = RenderPipelineFlags::AlphaBlend | RenderPipelineFlags::DepthPass;
            params.FragmentPath = "Shaders/UnLit.frag.spv";
            params.VertexPath = "Shaders/UnLit.vert.spv";
            params.Key = RenderPipelineKeys::UnLit;
            params.PolygonMode = VK_POLYGON_MODE_FILL;
            params.Topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
            params.BindingDescription = MeshVertex::GetBindingDescription();
            params.AttributeDescriptions = MeshVertex::GetAttributeDescriptions();
            params.PushConstantSize = sizeof(MeshPushConstants);
            params.Renderpass = m_RenderPass;
            params.DescriptorSetLayoutBindings.resize(2);
            params.DescriptorSetLayoutBindings[0] = renderer->m_DescriptorSetLayouts[RenderSetLayouts::Material];
            params.DescriptorSetLayoutBindings[1] = renderer->m_DescriptorSetLayouts[RenderSetLayouts::SceneFrameUBO];
            params.MsaaSampleCount = renderer->GetMsaaSampleCount();
            renderer->CreateGraphicsPipeline(params);
        }
        {    // Line drawing shader
            RenderPipelineParams params;
            params.Flags = RenderPipelineFlags::AlphaBlend;
            params.FragmentPath = "Shaders/Color.frag.spv";
            params.VertexPath = "Shaders/Color.vert.spv";
            params.Key = RenderPipelineKeys::Line;
            params.PolygonMode = VK_POLYGON_MODE_LINE;
            params.Topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
            params.BindingDescription = ColorVertex::GetBindingDescription();
            params.AttributeDescriptions = ColorVertex::GetAttributeDescriptions();
            params.PushConstantSize = sizeof(SkyPushConstants);
            params.Renderpass = m_RenderPass;
            params.MsaaSampleCount = renderer->GetMsaaSampleCount();
            renderer->CreateGraphicsPipeline(params);
        }
    }

    void SceneRenderer::DestroyTextureRenderer() {
        auto *renderer = VulkanRenderer::Get();

        DestoryFrameBuffers();
        DestoryDepthBuffer();
        DestroyResolveBuffer();
        DestroyRenderPass();
        DestroySyncObjects();

        renderer->FreePBRMapsDescriptorSet(m_IrradiencePrefilterDescriptorSet);

        renderer->FreeTextureDescriptorSet(m_IrradienceMap.DesctriptorSet);
        vkDestroyImageView(renderer->GetDevice(), m_IrradienceMap.ImageView, nullptr);
        vmaDestroyImage(renderer->GetAllocator(), m_IrradienceMap.Image, m_IrradienceMap.Allocation);

        renderer->FreeTextureDescriptorSet(m_PrefilterMap.DesctriptorSet);
        vkDestroyImageView(renderer->GetDevice(), m_PrefilterMap.ImageView, nullptr);
        vmaDestroyImage(renderer->GetAllocator(), m_PrefilterMap.Image, m_PrefilterMap.Allocation);
    }

    void SceneRenderer::ResizeBuffers(glm::vec2 extent) {
        std::cout << "Forward renderer: Resizing texture buffers.\n";

        m_Extent = extent;

        auto *renderer = VulkanRenderer::Get();
        renderer->FinishAllFrames();

        DestoryFrameBuffers();
        DestoryDepthBuffer();
        DestroyResolveBuffer();

        CreateResolveBuffer();
        CreateDepthBuffer();
        CreateFrameBuffers();
    }

    void SceneRenderer::CreateRenderPass() {
        auto *renderer = VulkanRenderer::Get();
        auto *window = renderer->GetVulkanWindow();
        m_DepthFormat = renderer->FindDepthFormat();

        VkAttachmentDescription colorAttachments[3]{};
        colorAttachments[0].format = window->SurfaceFormat.format;;
        colorAttachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
        colorAttachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        colorAttachments[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        colorAttachments[0].finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        colorAttachments[1].format = m_DepthFormat;
        colorAttachments[1].samples = renderer->GetMsaaSampleCount();
        colorAttachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAttachments[1].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        colorAttachments[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        colorAttachments[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        colorAttachments[2].format = window->SurfaceFormat.format;
        colorAttachments[2].samples = renderer->GetMsaaSampleCount();
        colorAttachments[2].loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachments[2].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachments[2].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachments[2].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        colorAttachments[2].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        colorAttachments[2].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkAttachmentReference colorAttachmentRef{};
        colorAttachmentRef.attachment = 2;
        colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkAttachmentReference depthStencilAttachmentRef{};
        depthStencilAttachmentRef.attachment = 1;
        depthStencilAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        VkAttachmentReference resolveAttachmentRef{};
        resolveAttachmentRef.attachment = 0;
        resolveAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkSubpassDescription subpass{};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &colorAttachmentRef;
        subpass.pDepthStencilAttachment = &depthStencilAttachmentRef;
        subpass.pResolveAttachments = &resolveAttachmentRef;
        std::array<VkSubpassDependency, 2> dependencies;

        dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
        dependencies[0].dstSubpass = 0;
        dependencies[0].srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependencies[0].srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
        dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

        dependencies[1].srcSubpass = 0;
        dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
        dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependencies[1].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        dependencies[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

        VkRenderPassCreateInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassInfo.attachmentCount = std::size(colorAttachments);
        renderPassInfo.pAttachments = colorAttachments;
        renderPassInfo.subpassCount = 1;
        renderPassInfo.pSubpasses = &subpass;
        renderPassInfo.dependencyCount = dependencies.size();
        renderPassInfo.pDependencies = dependencies.data();

        VkResult result = vkCreateRenderPass(renderer->m_LogicalDevice, &renderPassInfo, nullptr, &m_RenderPass);
        ASSERT(result == VK_SUCCESS);
        LOG("Forward renderer: Render pass created.\n");
    }

    void SceneRenderer::CreateDepthBuffer() {
        auto *renderer = VulkanRenderer::Get();

        ASSERT(m_DepthFormat != VK_FORMAT_UNDEFINED);

        VkImageCreateInfo depthImageInfo = {VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
        depthImageInfo.imageType = VK_IMAGE_TYPE_2D;
        depthImageInfo.extent.width = m_Extent.x;
        depthImageInfo.extent.height = m_Extent.y;
        depthImageInfo.extent.depth = 1;
        depthImageInfo.mipLevels = 1;
        depthImageInfo.arrayLayers = 1;
        depthImageInfo.format = m_DepthFormat;
        depthImageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        depthImageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        depthImageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
        depthImageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        depthImageInfo.samples = renderer->GetMsaaSampleCount();
        depthImageInfo.flags = 0;

        VmaAllocationCreateInfo depthImageAllocCreateInfo = {};
        depthImageAllocCreateInfo.usage = VMA_MEMORY_USAGE_AUTO;
        depthImageAllocCreateInfo.flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;
        depthImageAllocCreateInfo.priority = 1.f;

        VkImageViewCreateInfo depthImageViewInfo = {VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
        depthImageViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        depthImageViewInfo.format = m_DepthFormat;
        depthImageViewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        depthImageViewInfo.subresourceRange.baseMipLevel = 0;
        depthImageViewInfo.subresourceRange.levelCount = 1;
        depthImageViewInfo.subresourceRange.baseArrayLayer = 0;
        depthImageViewInfo.subresourceRange.layerCount = 1;

        vmaCreateImage(renderer->m_Allocator, &depthImageInfo, &depthImageAllocCreateInfo,
            &m_DepthBuffer.Image,
            &m_DepthBuffer.Allocation,
            &m_DepthBuffer.AllocationInfo);

        depthImageViewInfo.image = m_DepthBuffer.Image;

        auto result = vkCreateImageView(renderer->m_LogicalDevice, &depthImageViewInfo, nullptr,
            &m_DepthBufferImageView);
        ASSERT(result == VK_SUCCESS);
    }

    void SceneRenderer::CreateResolveBuffer()
    {
        auto* renderer = VulkanRenderer::Get();
        auto* window = renderer->GetVulkanWindow();

        VkFormat format = window->SurfaceFormat.format;

        VkImageCreateInfo resolveImageInfo = { VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
        resolveImageInfo.imageType = VK_IMAGE_TYPE_2D;
        resolveImageInfo.extent.width = m_Extent.x;
        resolveImageInfo.extent.height = m_Extent.y;
        resolveImageInfo.extent.depth = 1;
        resolveImageInfo.mipLevels = 1;
        resolveImageInfo.arrayLayers = 1;
        resolveImageInfo.format = format;
        resolveImageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        resolveImageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        resolveImageInfo.usage = VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        resolveImageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        resolveImageInfo.samples = renderer->GetMsaaSampleCount();
        resolveImageInfo.flags = 0;

        VmaAllocationCreateInfo resolveImageAllocCreateInfo = {};
        resolveImageAllocCreateInfo.usage = VMA_MEMORY_USAGE_AUTO;
        resolveImageAllocCreateInfo.flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;
        resolveImageAllocCreateInfo.priority = 1.f;

        VkImageViewCreateInfo resolveImageViewInfo = { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
        resolveImageViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        resolveImageViewInfo.format = format;
        resolveImageViewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        resolveImageViewInfo.subresourceRange.baseMipLevel = 0;
        resolveImageViewInfo.subresourceRange.levelCount = 1;
        resolveImageViewInfo.subresourceRange.baseArrayLayer = 0;
        resolveImageViewInfo.subresourceRange.layerCount = 1;

        vmaCreateImage(renderer->m_Allocator, &resolveImageInfo, &resolveImageAllocCreateInfo,
            &m_ResolveImageData.Image,
            &m_ResolveImageData.Allocation,
            &m_ResolveImageData.AllocationInfo);

        resolveImageViewInfo.image = m_ResolveImageData.Image;

        auto result = vkCreateImageView(renderer->m_LogicalDevice, &resolveImageViewInfo, nullptr,
            &m_ResolveImageView);
        ASSERT(result == VK_SUCCESS);
    }

    void SceneRenderer::CreateFrameBuffers() {
        auto *renderer = VulkanRenderer::Get();

        CreateFrameBufferTextures();
        CreateOffscreenFrameBuffer();

        for (uint32_t i = 0; i < m_TextureFrameBuffers.size(); i++) {
            VkImageView attachments[] = {
                    m_TextureFrameBuffers[i].VKTexture.ImageView,
                    m_DepthBufferImageView,
                    m_ResolveImageView,
            };

            VkFramebufferCreateInfo framebufferInfo{};
            framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            framebufferInfo.renderPass = m_RenderPass;
            framebufferInfo.attachmentCount = std::size(attachments);
            framebufferInfo.pAttachments = attachments;
            framebufferInfo.width = m_Extent.x;
            framebufferInfo.height = m_Extent.y;
            framebufferInfo.layers = 1;

            VkResult result = vkCreateFramebuffer(renderer->m_LogicalDevice, &framebufferInfo, nullptr,
                                                  &m_TextureFrameBuffers[i].FrameBuffer);
            ASSERT(result == VK_SUCCESS);
        }
        {
            VkImageView attachments[] = {
                    m_OffscreenFrameBuffer.VKTexture.ImageView,
                    m_DepthBufferImageView,
                    m_ResolveImageView,
            };

            VkFramebufferCreateInfo framebufferInfo{};
            framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            framebufferInfo.renderPass = m_RenderPass;
            framebufferInfo.attachmentCount = std::size(attachments);
            framebufferInfo.pAttachments = attachments;
            framebufferInfo.width = m_Extent.x;
            framebufferInfo.height = m_Extent.y;
            framebufferInfo.layers = 1;

            VkResult result = vkCreateFramebuffer(renderer->m_LogicalDevice, &framebufferInfo, nullptr,
                &m_OffscreenFrameBuffer.FrameBuffer);
            ASSERT(result == VK_SUCCESS);
        }
        LOG("Forward renderer: Framebuffers created.\n");
    }

    void SceneRenderer::CreateFrameBufferTextures() {
        auto *renderer = VulkanRenderer::Get();
        auto *window = renderer->GetVulkanWindow();

        VkFormat format = window->SurfaceFormat.format;

        uint32_t xWidth = m_Extent.x;
        uint32_t yHeight = m_Extent.y;

        for (auto &fbTexture: m_TextureFrameBuffers) {
            // Image
            VkImageCreateInfo imageInfo = {VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
            imageInfo.imageType = VK_IMAGE_TYPE_2D;
            imageInfo.extent.width = xWidth;
            imageInfo.extent.height = yHeight;
            imageInfo.extent.depth = 1;
            imageInfo.mipLevels = 1;
            imageInfo.arrayLayers = 1;
            imageInfo.format = format;
            imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
            imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            imageInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
            imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
            imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
            imageInfo.flags = 0;

            VmaAllocationCreateInfo imageAllocCreateInfo = {};
            imageAllocCreateInfo.usage = VMA_MEMORY_USAGE_AUTO;
            imageAllocCreateInfo.flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;
            imageAllocCreateInfo.priority = 1.f;

            vmaCreateImage(renderer->m_Allocator, &imageInfo, &imageAllocCreateInfo, &fbTexture.VKTexture.Image,
                           &fbTexture.VKTexture.Allocation, &fbTexture.VKTexture.AllocationInfo);

            // create image view
            VkImageViewCreateInfo textureImageViewInfo = {VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
            textureImageViewInfo.image = fbTexture.VKTexture.Image;
            textureImageViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            textureImageViewInfo.format = format;
            textureImageViewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            textureImageViewInfo.subresourceRange.baseMipLevel = 0;
            textureImageViewInfo.subresourceRange.levelCount = 1;
            textureImageViewInfo.subresourceRange.baseArrayLayer = 0;
            textureImageViewInfo.subresourceRange.layerCount = 1;
            vkCreateImageView(renderer->m_LogicalDevice, &textureImageViewInfo, nullptr, &fbTexture.VKTexture.ImageView);

            VkSampler sampler = renderer->GetFontSampler();
            fbTexture.VKTexture.DesctriptorSet = ImGui_ImplVulkan_AddTexture(sampler, fbTexture.VKTexture.ImageView,
                                                               VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        }
    }

    void SceneRenderer::CreateOffscreenFrameBuffer()
    {
        auto* renderer = VulkanRenderer::Get();
        auto* window = renderer->GetVulkanWindow();

        VkFormat format = window->SurfaceFormat.format;

        uint32_t xWidth = m_Extent.x;
        uint32_t yHeight = m_Extent.y;
        // Image
        VkImageCreateInfo imageInfo = { VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.extent.width = xWidth;
        imageInfo.extent.height = yHeight;
        imageInfo.extent.depth = 1;
        imageInfo.mipLevels = 1;
        imageInfo.arrayLayers = 1;
        imageInfo.format = format;
        imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.flags = 0;

        VmaAllocationCreateInfo imageAllocCreateInfo = {};
        imageAllocCreateInfo.usage = VMA_MEMORY_USAGE_AUTO;
        imageAllocCreateInfo.flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;
        imageAllocCreateInfo.priority = 1.f;

        vmaCreateImage(renderer->m_Allocator, &imageInfo, &imageAllocCreateInfo, &m_OffscreenFrameBuffer.VKTexture.Image,
            &m_OffscreenFrameBuffer.VKTexture.Allocation, &m_OffscreenFrameBuffer.VKTexture.AllocationInfo);

        // create image view
        VkImageViewCreateInfo textureImageViewInfo = { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
        textureImageViewInfo.image = m_OffscreenFrameBuffer.VKTexture.Image;
        textureImageViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        textureImageViewInfo.format = format;
        textureImageViewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        textureImageViewInfo.subresourceRange.baseMipLevel = 0;
        textureImageViewInfo.subresourceRange.levelCount = 1;
        textureImageViewInfo.subresourceRange.baseArrayLayer = 0;
        textureImageViewInfo.subresourceRange.layerCount = 1;
        vkCreateImageView(renderer->m_LogicalDevice, &textureImageViewInfo, nullptr, &m_OffscreenFrameBuffer.VKTexture.ImageView);

        VkSampler sampler = renderer->GetFontSampler();
        m_OffscreenFrameBuffer.VKTexture.DesctriptorSet = ImGui_ImplVulkan_AddTexture(sampler, m_OffscreenFrameBuffer.VKTexture.ImageView,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    }

    void SceneRenderer::CreateSyncObjects() {
        auto* renderer = VulkanRenderer::Get();

        VkSemaphoreCreateInfo semaphoreInfo{};
        semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        for (auto& semaphore : m_MainPassSignalSemaphores) {
            VkResult result = vkCreateSemaphore(renderer->GetDevice(),
                &semaphoreInfo, nullptr, &semaphore);
            ASSERT(result == VK_SUCCESS);
        }
        for (auto& semaphore : m_ShadowPassSignalSemaphores) {
            VkResult result = vkCreateSemaphore(renderer->GetDevice(),
                &semaphoreInfo, nullptr, &semaphore);
            ASSERT(result == VK_SUCCESS);
        }
        for (auto& semaphore : m_AlphaClearPassSignalSemaphores) {
            VkResult result = vkCreateSemaphore(renderer->GetDevice(),
                &semaphoreInfo, nullptr, &semaphore);
            ASSERT(result == VK_SUCCESS);
        }
    }

    void SceneRenderer::DestroyRenderPass() {
        auto *renderer = VulkanRenderer::Get();
        vkDestroyRenderPass(renderer->m_LogicalDevice, m_RenderPass, nullptr);
    }

    void SceneRenderer::DestoryFrameBuffers() {
        auto *renderer = VulkanRenderer::Get();

        for (auto &textureFrameBuffer: m_TextureFrameBuffers) {
            vkDestroyFramebuffer(renderer->m_LogicalDevice, textureFrameBuffer.FrameBuffer, nullptr);
            vkDestroyImageView(renderer->m_LogicalDevice, textureFrameBuffer.VKTexture.ImageView, nullptr);
            vmaDestroyImage(renderer->m_Allocator, textureFrameBuffer.VKTexture.Image,
                            textureFrameBuffer.VKTexture.Allocation);
            ImGui_ImplVulkan_RemoveTexture(textureFrameBuffer.VKTexture.DesctriptorSet);

            textureFrameBuffer.FrameBuffer = VK_NULL_HANDLE;
            textureFrameBuffer.VKTexture.ImageView = VK_NULL_HANDLE;
            textureFrameBuffer.VKTexture.Image = VK_NULL_HANDLE;
            textureFrameBuffer.VKTexture.DesctriptorSet = VK_NULL_HANDLE;
        }

        vkDestroyFramebuffer(renderer->m_LogicalDevice, m_OffscreenFrameBuffer.FrameBuffer, nullptr);
        vkDestroyImageView(renderer->m_LogicalDevice, m_OffscreenFrameBuffer.VKTexture.ImageView, nullptr);
        vmaDestroyImage(renderer->m_Allocator, m_OffscreenFrameBuffer.VKTexture.Image,
            m_OffscreenFrameBuffer.VKTexture.Allocation);
        ImGui_ImplVulkan_RemoveTexture(m_OffscreenFrameBuffer.VKTexture.DesctriptorSet);

        m_OffscreenFrameBuffer.FrameBuffer = VK_NULL_HANDLE;
        m_OffscreenFrameBuffer.VKTexture.ImageView = VK_NULL_HANDLE;
        m_OffscreenFrameBuffer.VKTexture.Image = VK_NULL_HANDLE;
        m_OffscreenFrameBuffer.VKTexture.DesctriptorSet = VK_NULL_HANDLE;
    }

    void SceneRenderer::DestroySyncObjects() {
        auto* renderer = VulkanRenderer::Get();
        for (auto& semaphore : m_MainPassSignalSemaphores) {
            vkDestroySemaphore(renderer->GetDevice(), semaphore, nullptr);
        }
        for (auto& semaphore : m_ShadowPassSignalSemaphores) {
            vkDestroySemaphore(renderer->GetDevice(), semaphore, nullptr);
        }
        for (auto& semaphore : m_AlphaClearPassSignalSemaphores) {
            vkDestroySemaphore(renderer->GetDevice(), semaphore, nullptr);
        }
    }

    void SceneRenderer::DebugDrawLine(const glm::vec3& start, const glm::vec3& end, glm::vec3 color) const
    {
        m_DebugVertexData.emplace_back(ColorVertex{ start, color });
        m_DebugVertexData.emplace_back(ColorVertex{ end, color });
    }

    void SceneRenderer::DrawDebugCameraLines(CameraComponent* camera) const
    {
        DebugDrawLine(camera->Position, camera->Position + camera->Forward * camera->Near, glm::vec3(1.f, 0.f, 0.f));
        uint32_t SHADOW_MAP_CASCADE_COUNT = m_SceneFrameData.CascadeCount;
        static constexpr float cascadeSplitLambda = 0.95f;

        std::vector<float> cascadeSplits(SHADOW_MAP_CASCADE_COUNT);

        float nearClip = camera->Near;
        float farClip = m_ShadowFarClip;
        float clipRange = farClip - nearClip;

        float minZ = nearClip;
        float maxZ = nearClip + clipRange;

        float range = maxZ - minZ;
        float ratio = maxZ / minZ;

        // Calculate split depths based on view camera frustum
        // Based on method presented in https://developer.nvidia.com/gpugems/GPUGems3/gpugems3_ch10.html
        for (uint32_t i = 0; i < SHADOW_MAP_CASCADE_COUNT; i++) {
            float p = (i + 1) / static_cast<float>(SHADOW_MAP_CASCADE_COUNT);
            float log = minZ * std::pow(ratio, p);
            float uniform = minZ + range * p;
            float d = cascadeSplitLambda * (log - uniform) + uniform;
            cascadeSplits[i] = (d - nearClip) / clipRange;
        }

        std::vector<glm::vec3> frustumColors = {
            glm::vec3(0.f, 0.f, 1.f),
            glm::vec3(0.f, 1.f, 0.f),
            glm::vec3(1.f, 0.f, 0.f),
            glm::vec3(1.f, 1.f, 1.f),
        };

        // Calculate orthographic projection matrix for each cascade
        glm::mat4 VP = glm::perspective(camera->FOV, camera->Aspect, camera->Near, farClip) * camera->GetView();
        glm::mat4 invCam = glm::inverse(VP);
        float lastSplitDist = 0.0;
        for (uint32_t i = 0; i < SHADOW_MAP_CASCADE_COUNT; i++) {
            float splitDist = cascadeSplits[i];

            glm::vec3 frustumCorners[8] = {
                glm::vec3(-1.0f,  1.0f, -1.0f),
                glm::vec3(1.0f,  1.0f, -1.0f),
                glm::vec3(1.0f, -1.0f, -1.0f),
                glm::vec3(-1.0f, -1.0f, -1.0f),
                glm::vec3(-1.0f,  1.0f,  1.0f),
                glm::vec3(1.0f,  1.0f,  1.0f),
                glm::vec3(1.0f, -1.0f,  1.0f),
                glm::vec3(-1.0f, -1.0f,  1.0f),
            };

            for (uint32_t i = 0; i < 8; i++) {
                glm::vec4 invCorner = invCam * glm::vec4(frustumCorners[i], 1.0f);
                frustumCorners[i] = invCorner / invCorner.w;
            }

            for (uint32_t i = 0; i < 4; i++) {
                glm::vec3 dist = frustumCorners[i + 4] - frustumCorners[i];
                frustumCorners[i + 4] = frustumCorners[i] + (dist * splitDist);
                frustumCorners[i] = frustumCorners[i] + (dist * lastSplitDist);
            }

            // Get frustum center
            glm::vec3 frustumCenter = glm::vec3(0.0f);
            for (uint32_t i = 0; i < 8; i++) {
                frustumCenter += frustumCorners[i];
            }
            frustumCenter /= 8.0f;

            float radius = 0.0f;
            for (uint32_t i = 0; i < 8; i++) {
                float distance = glm::length(frustumCorners[i] - frustumCenter);
                radius = glm::max(radius, distance);
            }
            radius = std::ceil(radius * 16.0f) / 16.0f;

            glm::vec3 maxExtents = glm::vec3(radius);
            glm::vec3 minExtents = -maxExtents;

            glm::vec3 lightDir = glm::normalize(-m_SceneFrameData.SunPosition);
            glm::vec3 eye = frustumCenter - (lightDir * -minExtents.z);

            DebugDrawFrustum(frustumCorners[3], frustumCorners[2], frustumCorners[0], frustumCorners[1],
                frustumCorners[7], frustumCorners[6], frustumCorners[4], frustumCorners[5], frustumColors[i]);

            DebugDrawLine(eye, eye + lightDir * 10.f, frustumColors[i]);

            lastSplitDist = cascadeSplits[i];
        }
    }

    void SceneRenderer::DrawDebugCameraLines() {
        m_DrawDebugCameraLines = true;
    }

    void SceneRenderer::DebugDrawFrustum(const glm::vec3& nbl, const glm::vec3& nbr, const glm::vec3& ntl,
        const glm::vec3& ntr, const glm::vec3& fbl, const glm::vec3& fbr, const glm::vec3& ftl, const glm::vec3& ftr, const glm::vec3& color) const
    {
        // draw lear rect
        DrawRect(nbl, nbr, ntl, ntr, color);
        // draw far rect
        DrawRect(fbl, fbr, ftl, ftr, color);
        // draw connecting lines
        DebugDrawLine(nbl, fbl, color);
        DebugDrawLine(nbr, fbr, color);
        DebugDrawLine(ntl, ftl, color);
        DebugDrawLine(ntr, ftr, color);
    }

    void SceneRenderer::DrawRect(const glm::vec3& bl, const glm::vec3& br, const glm::vec3& tl, const glm::vec3& tr, const glm::vec3& color) const 
    {
        DebugDrawLine(bl, br, color);
        DebugDrawLine(br, tr, color);
        DebugDrawLine(tr, tl, color);
        DebugDrawLine(tl, bl, color);
    }

    void SceneRenderer::DestoryDepthBuffer() {
        auto *renderer = VulkanRenderer::Get();

        vkDestroyImageView(renderer->m_LogicalDevice, m_DepthBufferImageView, nullptr);
        m_DepthBufferImageView = VK_NULL_HANDLE;

        vmaDestroyImage(renderer->m_Allocator, m_DepthBuffer.Image, m_DepthBuffer.Allocation);
        m_DepthBuffer.Image = VK_NULL_HANDLE;
    }

    void SceneRenderer::DestroyResolveBuffer()
    {
        auto* renderer = VulkanRenderer::Get();

        vkDestroyImageView(renderer->m_LogicalDevice, m_ResolveImageView, nullptr);
        m_ResolveImageView = VK_NULL_HANDLE;
        vmaDestroyImage(renderer->m_Allocator, m_ResolveImageData.Image, m_ResolveImageData.Allocation);
        m_ResolveImageData.Image = VK_NULL_HANDLE;
    }
}