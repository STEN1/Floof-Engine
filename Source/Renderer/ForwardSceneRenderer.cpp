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
        m_Renderer->StartRecordingGraphics(*vulkanWindow);
        auto commandBuffer = vulkanWindow->Frames[vulkanWindow->FrameIndex].CommandBuffer;

        // Camera setup
        auto extent = m_Renderer->GetExtent();
        CameraComponent* camera = app.GetRenderCamera();
        glm::mat4 vp = camera->GetVP(glm::radians(70.f), extent.width / (float)extent.height, 0.01f, 2000.f);
        
        // Draw debug lines
        auto* physicDrawer = app.GetPhysicsSystemDrawer();
        if (physicDrawer) {
            auto pipelineLayout = m_Renderer->BindGraphicsPipeline(commandBuffer, RenderPipelineKeys::LineWithDepth);
            auto* lineMesh = physicDrawer->GetUpdatedLineMesh();
            ColorPushConstants constants;
            constants.MVP = vp;
            vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT,
                0, sizeof(ColorPushConstants), &constants);
            lineMesh->Draw(commandBuffer);
        }

        // Draw models
        auto pipelineLayout = m_Renderer->BindGraphicsPipeline(commandBuffer, RenderPipelineKeys::Basic);
        auto view = m_Registry.view<TransformComponent, MeshComponent, TextureComponent>();
        for (auto [entity, transform, mesh, texture] : view.each()) {
            MeshPushConstants constants;
            //constants.MVP = vp * transform.GetTransform();
            glm::mat4 modelMat = glm::translate(transform.Position);
            modelMat = glm::scale(modelMat, transform.Scale);
            constants.MVP = vp * modelMat;
            constants.InvModelMat = glm::inverse(modelMat);
            vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT,
                0, sizeof(MeshPushConstants), &constants);

            texture.Bind(commandBuffer);
            mesh.Draw(commandBuffer);
        }

        {	// Draw ImGui
            ImGui::Render();
            ImDrawData* drawData = ImGui::GetDrawData();
            ImGui_ImplVulkan_RenderDrawData(drawData, commandBuffer);
        }
    }
}