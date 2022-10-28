#include "DeferredSceneRenderer.h"

namespace FLOOF {
    DeferredSceneRenderer::DeferredSceneRenderer() {
    }
    DeferredSceneRenderer::~DeferredSceneRenderer() {
    }
    void DeferredSceneRenderer::Render(entt::registry& scene) {
        auto m_Renderer = VulkanRenderer::Get();
        auto* vulkanWindow = m_Renderer->GetVulkanWindow();
        auto& currentFrameData = vulkanWindow->Frames[vulkanWindow->FrameIndex];
        
        m_Renderer->StartRenderPass(
            currentFrameData.MainCommandBuffer,
            m_Renderer->GetImguiRenderPass(),
            vulkanWindow->FrameBuffers[vulkanWindow->ImageIndex],
            vulkanWindow->Extent);

        // TODO: Deferred rendering goes here bruh.

        // End main renderpass
        VulkanSubmitInfo submitInfo{};
        submitInfo.CommandBuffer = currentFrameData.MainCommandBuffer;
        submitInfo.WaitSemaphore = currentFrameData.ImageAvailableSemaphore;
        submitInfo.SignalSemaphore = currentFrameData.MainPassEndSemaphore;
        submitInfo.WaitStages = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        m_Renderer->EndRenderPass(submitInfo);
    }

    VkDescriptorSet DeferredSceneRenderer::RenderToTexture(entt::registry& scene, glm::vec2 extent) {
        return VK_NULL_HANDLE;
    }
}