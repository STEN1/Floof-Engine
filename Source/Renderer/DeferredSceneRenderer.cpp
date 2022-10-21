#include "DeferredSceneRenderer.h"

namespace FLOOF {
    DeferredSceneRenderer::DeferredSceneRenderer() {
    }
    DeferredSceneRenderer::~DeferredSceneRenderer() {
    }
    void DeferredSceneRenderer::Render(entt::registry& scene) {
        auto m_Renderer = VulkanRenderer::Get();
        auto* vulkanWindow = m_Renderer->GetVulkanWindow();
        m_Renderer->StartRecordingGraphics(*vulkanWindow);
        auto commandBuffer = vulkanWindow->Frames[vulkanWindow->FrameIndex].CommandBuffer;

        {	// Draw ImGui
            ImGui::Render();
            ImDrawData* drawData = ImGui::GetDrawData();
            ImGui_ImplVulkan_RenderDrawData(drawData, commandBuffer);
        }
    }
}