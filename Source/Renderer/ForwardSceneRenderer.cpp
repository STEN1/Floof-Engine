#include "ForwardSceneRenderer.h"
#include "VulkanRenderer.h"
#include "../Components.h"
#include "../Application.h"

namespace FLOOF {
    ForwardSceneRenderer::ForwardSceneRenderer() {
    }
    ForwardSceneRenderer::~ForwardSceneRenderer() {
    }
    void ForwardSceneRenderer::Render(entt::registry& m_Registry) {
        auto m_Renderer = VulkanRenderer::Get();
        auto& app = Application::Get();
        auto* vulkanWindow = m_Renderer->GetVulkanWindow();
        auto& currentFrameData = vulkanWindow->Frames[vulkanWindow->FrameIndex];

        m_Renderer->StartRenderPass(
            currentFrameData.MainCommandBuffer,
            m_Renderer->GetMainRenderPass(),
            vulkanWindow->FrameBuffers[vulkanWindow->ImageIndex],
            vulkanWindow->Extent
        );
        auto commandBuffer = vulkanWindow->Frames[vulkanWindow->FrameIndex].MainCommandBuffer;

        // Camera setup
        auto extent = m_Renderer->GetExtent();
        CameraComponent* camera = app.GetRenderCamera();
        glm::mat4 vp = camera->GetVP(glm::radians(70.f), extent.width / (float)extent.height, 0.01f, 5000.f);
        
        // Draw models
        auto pipelineLayout = m_Renderer->BindGraphicsPipeline(commandBuffer, RenderPipelineKeys::Basic);
        {
            auto view = m_Registry.view<TransformComponent, MeshComponent, TextureComponent>();
            for (auto [entity, transform, mesh, texture] : view.each()) {
                MeshPushConstants constants;
                glm::mat4 modelMat = transform.GetTransform();
                constants.MVP = vp * modelMat;
                constants.InvModelMat = glm::inverse(modelMat);
                vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT,
                    0, sizeof(MeshPushConstants), &constants);

                texture.Bind(commandBuffer);
                mesh.Draw(commandBuffer);
            }
        }

        auto view = m_Registry.view<TransformComponent, StaticMeshComponent, TextureComponent>();
        for (auto [entity, transform, staticMesh, texture] : view.each()) {
            MeshPushConstants constants;
            glm::mat4 modelMat = transform.GetTransform();
            constants.MVP = vp * modelMat;
            constants.InvModelMat = glm::inverse(modelMat);
            vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT,
                0, sizeof(MeshPushConstants), &constants);
            texture.Bind(commandBuffer);
            for (auto& mesh : staticMesh.meshes)
            {
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
        // Draw debug lines
        auto* physicDrawer = app.GetPhysicsSystemDrawer();
        if (physicDrawer) {
            auto pipelineLayout = m_Renderer->BindGraphicsPipeline(commandBuffer, RenderPipelineKeys::Line);
            auto* lineMesh = physicDrawer->GetUpdatedLineMesh();
            ColorPushConstants constants;
            constants.MVP = vp;
            vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT,0, sizeof(ColorPushConstants), &constants);
            lineMesh->Draw(commandBuffer);
        }

        // End main renderpass
        VulkanSubmitInfo submitInfo{};
        submitInfo.CommandBuffer = commandBuffer;
        submitInfo.WaitSemaphore = currentFrameData.ImageAvailableSemaphore;
        submitInfo.SignalSemaphore = currentFrameData.MainPassEndSemaphore;
        submitInfo.WaitStages = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        m_Renderer->EndRenderPass(submitInfo);
    }
}