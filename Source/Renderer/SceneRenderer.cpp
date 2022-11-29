#include "SceneRenderer.h"
#include "VulkanRenderer.h"
#include "../Application.h"
#include <iostream>
#include "../PhysicsSystem.h"

namespace FLOOF {
    SceneRenderer::SceneRenderer() {
        CreateTextureRenderer();
        /*std::array<std::string, 6> skyboxPaths = {
            "Assets/Skybox/back.jpg",
            "Assets/Skybox/front.jpg",
            "Assets/Skybox/bottom.jpg",
            "Assets/Skybox/top.jpg",
            "Assets/Skybox/left.jpg",
            "Assets/Skybox/right.jpg",
        };*/
        m_Skybox = std::make_unique<Skybox>("Assets/Skybox/belfast_sunset_puresky_4k.hdr");
        m_IrradienceMap = m_Skybox->m_Cubemap.GetIrradienceMap();
        m_PrefilterMap = m_Skybox->m_Cubemap.GetPrefilterMap();
        m_BRDFLut = TextureManager::GetBRDFLut();
    }

    SceneRenderer::~SceneRenderer() {
        DestroyTextureRenderer();
    }

    SceneRenderFinishedData SceneRenderer::RenderToTexture(Scene* scene, glm::vec2 extent, CameraComponent* camera,
        RenderPipelineKeys drawMode, PhysicsDebugDraw* physicDrawer, VkSemaphore waitSemaphore) {
        if (extent == glm::vec2(0.f))
            return SceneRenderFinishedData();
        if (extent != m_Extent)
            ResizeBuffers(extent);

        auto* renderer = VulkanRenderer::Get();
        auto* window = renderer->GetVulkanWindow();
        auto frameIndex = window->FrameIndex;
        auto& frameData = window->Frames[frameIndex];
        auto signalSemaphore = m_SignalSemaphores[frameIndex];
        auto commandBuffer = m_CommandBuffers[frameIndex];

        VkExtent2D vkExtent;
        vkExtent.width = m_Extent.x;
        vkExtent.height = m_Extent.y;

        VkRenderPassBeginInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = m_RenderPass;
        renderPassInfo.framebuffer = m_TextureFrameBuffers[frameIndex].FrameBuffer;
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

        renderer->ResetAndBeginCommandBuffer(commandBuffer);
        renderer->StartRenderPass(commandBuffer, &renderPassInfo);

        // Camera setup
        glm::mat4 cameraProjection = camera->GetPerspective(glm::radians(70.f), vkExtent.width / (float)vkExtent.height, 0.5f, 1000000.f);
        glm::mat4 cameraView = camera->GetView();
        glm::mat4 vp = cameraProjection * cameraView;

        if (m_Skybox) {
            auto pipelineLayout = renderer->BindGraphicsPipeline(commandBuffer, RenderPipelineKeys::Skybox);

            MeshPushConstants constants;
            glm::mat4 modelMat = glm::mat4(1.f);
            constants.VP = cameraProjection * glm::mat4(glm::mat3(camera->GetView()));
            constants.Model = modelMat;
            constants.InvModelMat = glm::inverse(modelMat);
            vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT,
                0, sizeof(MeshPushConstants), &constants);

            m_Skybox->Draw(commandBuffer, pipelineLayout);
        }

        auto pipelineLayout = renderer->BindGraphicsPipeline(commandBuffer, drawMode);

        if (drawMode == RenderPipelineKeys::PBR) {
            std::vector<PointLightComponent::PointLight> pointLights;
            auto lightView = scene->m_Registry.view<TransformComponent, PointLightComponent>();
            for (auto [entity, transform, lightComp] : lightView.each()) {
                PointLightComponent::PointLight light;
                light.position = glm::vec4(transform.GetWorldPosition(), 1.f);
                light.diffuse = lightComp.diffuse;
                light.ambient = lightComp.ambient;
                light.lightRange = lightComp.lightRange;
                light.linear = 4.5f / light.lightRange;
                light.quadratic = 75.f / (light.lightRange * light.lightRange);
                pointLights.push_back(light);
            }

            m_LightSSBO.Update(pointLights);
            auto lightDescriptor = m_LightSSBO.GetDescriptorSet();

            vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 2, 1,
                &lightDescriptor, 0, nullptr);

            static float accumulator = 0.f;
            accumulator += 0.01f;
            float r = (sinf(accumulator) + 1.f) * 0.5f;
            float b = (cosf(accumulator) + 1.f) * 0.5f;
            m_SceneFrameData.CameraPos = glm::vec4(camera->Position, 1.f);
            m_SceneFrameData.LightCount = pointLights.size();
            m_SceneDataUBO.Update(m_SceneFrameData);
            auto sceneDescriptor = m_SceneDataUBO.GetDescriptorSet();

            vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 1, 1,
                &sceneDescriptor, 0, nullptr);

            vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 3, 1,
                &m_IrradienceMap.DesctriptorSet, 0, nullptr);

            vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 4, 1,
                &m_PrefilterMap.DesctriptorSet, 0, nullptr);

            vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 5, 1,
                &m_BRDFLut.DesctriptorSet, 0, nullptr);
        }

        // Draw models
        {
            auto pipelineLayout = renderer->BindGraphicsPipeline(commandBuffer, drawMode);
            auto view = scene->m_Registry.view<TransformComponent, StaticMeshComponent>();
            for (auto [entity, transform, staticMesh] : view.each()) {
                MeshPushConstants constants;
                glm::mat4 modelMat = transform.GetTransform();
                constants.VP = vp;
                constants.Model = modelMat;
                constants.InvModelMat = glm::inverse(modelMat);
                vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT,
                    0, sizeof(MeshPushConstants), &constants);
                for (auto& mesh : staticMesh.meshes) {
                    if (drawMode == RenderPipelineKeys::PBR || drawMode == RenderPipelineKeys::UnLit || drawMode == RenderPipelineKeys::Normals) {
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
                    }
                    else {
                        vkCmdDraw(commandBuffer, mesh.VertexCount, 1, 0, 0);
                    }
                }
            }
        }

        // Draw debug lines
        if (physicDrawer) {
            auto pipelineLayout = renderer->BindGraphicsPipeline(commandBuffer, RenderPipelineKeys::Line);
            auto* lineMesh = physicDrawer->GetUpdatedLineMesh();
            MeshPushConstants constants;
            constants.VP = vp;
            constants.Model = glm::mat4(1.f);
            constants.InvModelMat = glm::mat4(1.f);
            vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(MeshPushConstants),
                &constants);
            lineMesh->Draw(commandBuffer);
        }
        // Draw landscape
        {
            auto pipelineLayout = renderer->BindGraphicsPipeline(commandBuffer, RenderPipelineKeys::Landscape);
            auto view = scene->m_Registry.view<TransformComponent, LandscapeComponent>();
            for (auto [entity, transform, landscape] : view.each()) {
                MeshPushConstants constants;
                glm::mat4 modelMat = transform.GetTransform();
                constants.VP = vp;
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
                    6, 1, &landscape.meshData.MeshMaterial2.DescriptorSet, 0, nullptr);
                vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout,
                    7, 1, &landscape.meshData.MeshMaterial3.DescriptorSet, 0, nullptr);

                VkDeviceSize offset{ 0 };
                vkCmdBindVertexBuffers(commandBuffer, 0, 1, &landscape.meshData.VertexBuffer.Buffer, &offset);
                vkCmdBindIndexBuffer(commandBuffer, landscape.meshData.IndexBuffer.Buffer, 0, VK_INDEX_TYPE_UINT32);
                vkCmdDrawIndexed(commandBuffer, landscape.meshData.IndexCount, 1, 0, 0, 0);
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

        SceneRenderFinishedData renderFinishedData{};
        renderFinishedData.Texture = m_TextureFrameBuffers[frameIndex].VKTexture.DesctriptorSet;
        renderFinishedData.SceneRenderFinishedSemaphore = signalSemaphore;

        return renderFinishedData;
    }

    void SceneRenderer::ShadowPass()
    {
    }

    void SceneRenderer::CreateTextureRenderer() {
        auto *renderer = VulkanRenderer::Get();
        m_TextureFrameBuffers.resize(VulkanGlobals::MAX_FRAMES_IN_FLIGHT);
        m_SignalSemaphores.resize(VulkanGlobals::MAX_FRAMES_IN_FLIGHT);
        m_waitSemaphores.resize(VulkanGlobals::MAX_FRAMES_IN_FLIGHT);
        m_CommandBuffers.resize(VulkanGlobals::MAX_FRAMES_IN_FLIGHT);
        for (auto& commandBuffer : m_CommandBuffers) {
            commandBuffer = renderer->AllocateCommandBuffer();
        }

        CreateSyncObjects();

        CreateRenderPass();
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
            params.DescriptorSetLayoutBindings.resize(6);
            params.DescriptorSetLayoutBindings[0] = renderer->m_DescriptorSetLayouts[RenderSetLayouts::Material];
            params.DescriptorSetLayoutBindings[1] = renderer->m_DescriptorSetLayouts[RenderSetLayouts::SceneFrameUBO];
            params.DescriptorSetLayoutBindings[2] = renderer->m_DescriptorSetLayouts[RenderSetLayouts::LightSSBO];
            params.DescriptorSetLayoutBindings[3] = renderer->m_DescriptorSetLayouts[RenderSetLayouts::DiffuseTextureClamped];
            params.DescriptorSetLayoutBindings[4] = renderer->m_DescriptorSetLayouts[RenderSetLayouts::DiffuseTextureClamped];
            params.DescriptorSetLayoutBindings[5] = renderer->m_DescriptorSetLayouts[RenderSetLayouts::DiffuseTextureClamped];
            params.Renderpass = m_RenderPass;
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
            params.DescriptorSetLayoutBindings.resize(8);
            params.DescriptorSetLayoutBindings[0] = renderer->m_DescriptorSetLayouts[RenderSetLayouts::Material];
            params.DescriptorSetLayoutBindings[1] = renderer->m_DescriptorSetLayouts[RenderSetLayouts::SceneFrameUBO];
            params.DescriptorSetLayoutBindings[2] = renderer->m_DescriptorSetLayouts[RenderSetLayouts::LightSSBO];
            params.DescriptorSetLayoutBindings[3] = renderer->m_DescriptorSetLayouts[RenderSetLayouts::DiffuseTextureClamped];
            params.DescriptorSetLayoutBindings[4] = renderer->m_DescriptorSetLayouts[RenderSetLayouts::DiffuseTextureClamped];
            params.DescriptorSetLayoutBindings[5] = renderer->m_DescriptorSetLayouts[RenderSetLayouts::DiffuseTextureClamped];
            params.DescriptorSetLayoutBindings[6] = renderer->m_DescriptorSetLayouts[RenderSetLayouts::Material];
            params.DescriptorSetLayoutBindings[7] = renderer->m_DescriptorSetLayouts[RenderSetLayouts::Material];
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
            params.PushConstantSize = sizeof(MeshPushConstants);
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
            params.DescriptorSetLayoutBindings.resize(1);
            params.DescriptorSetLayoutBindings[0] = renderer->m_DescriptorSetLayouts[RenderSetLayouts::Material];
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
            params.DescriptorSetLayoutBindings.resize(1);
            params.DescriptorSetLayoutBindings[0] = renderer->m_DescriptorSetLayouts[RenderSetLayouts::Material];
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
            params.PushConstantSize = sizeof(MeshPushConstants);
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

        renderer->FreeTextureDescriptorSet(m_IrradienceMap.DesctriptorSet);
        vkDestroyImageView(renderer->GetDevice(), m_IrradienceMap.ImageView, nullptr);
        vmaDestroyImage(renderer->GetAllocator(), m_IrradienceMap.Image, m_IrradienceMap.Allocation);

        renderer->FreeTextureDescriptorSet(m_PrefilterMap.DesctriptorSet);
        vkDestroyImageView(renderer->GetDevice(), m_PrefilterMap.ImageView, nullptr);
        vmaDestroyImage(renderer->GetAllocator(), m_PrefilterMap.Image, m_PrefilterMap.Allocation);

        renderer->FreeTextureDescriptorSet(m_BRDFLut.DesctriptorSet);
        vkDestroyImageView(renderer->GetDevice(), m_BRDFLut.ImageView, nullptr);
        vmaDestroyImage(renderer->GetAllocator(), m_BRDFLut.Image, m_BRDFLut.Allocation);
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
        colorAttachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
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

        // Use subpass dependencies for layout transitions
        std::array<VkSubpassDependency, 2> dependencies;

        dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
        dependencies[0].dstSubpass = 0;

        dependencies[0].srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
        dependencies[0].srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;

        dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

        dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

        dependencies[1].srcSubpass = 0;
        dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;

        dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

        dependencies[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        dependencies[1].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;

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
            &m_ResolveBuffer.Image,
            &m_ResolveBuffer.Allocation,
            &m_ResolveBuffer.AllocationInfo);

        resolveImageViewInfo.image = m_ResolveBuffer.Image;

        auto result = vkCreateImageView(renderer->m_LogicalDevice, &resolveImageViewInfo, nullptr,
            &m_ResolveImageView);
        ASSERT(result == VK_SUCCESS);
    }

    void SceneRenderer::CreateFrameBuffers() {
        auto *renderer = VulkanRenderer::Get();

        CreateFrameBufferTextures();

        for (auto &textureFrameBuffer: m_TextureFrameBuffers) {
            VkImageView attachments[] = {
                    textureFrameBuffer.VKTexture.ImageView,
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
                                                  &textureFrameBuffer.FrameBuffer);
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

    void SceneRenderer::CreateSyncObjects() {
        auto* renderer = VulkanRenderer::Get();

        VkSemaphoreCreateInfo semaphoreInfo{};
        semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        for (auto& semaphore : m_SignalSemaphores) {
            VkResult result = vkCreateSemaphore(renderer->GetDevice(),
                &semaphoreInfo, nullptr, &semaphore);
            ASSERT(result == VK_SUCCESS);
        }
        for (auto& semaphore : m_waitSemaphores) {
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
    }

    void SceneRenderer::DestroySyncObjects() {
        auto* renderer = VulkanRenderer::Get();
        for (auto& semaphore : m_SignalSemaphores) {
            vkDestroySemaphore(renderer->GetDevice(), semaphore, nullptr);
        }
        for (auto& semaphore : m_waitSemaphores) {
            vkDestroySemaphore(renderer->GetDevice(), semaphore, nullptr);
        }
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
        vmaDestroyImage(renderer->m_Allocator, m_ResolveBuffer.Image, m_ResolveBuffer.Allocation);
        m_ResolveBuffer.Image = VK_NULL_HANDLE;
    }
}