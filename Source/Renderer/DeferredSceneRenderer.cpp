#include "DeferredSceneRenderer.h"

namespace FLOOF {
    DeferredSceneRenderer::DeferredSceneRenderer() {
    }
    DeferredSceneRenderer::~DeferredSceneRenderer() {
    }
    void DeferredSceneRenderer::Render(entt::registry& scene) {
        auto m_Renderer = VulkanRenderer::Get();
        auto* vulkanWindow = m_Renderer->GetVulkanWindow();
        m_Renderer->NewFrame(*vulkanWindow);
        m_Renderer->StartRenderPass(
            vulkanWindow->Frames[vulkanWindow->FrameIndex].MainCommandBuffer,
            m_Renderer->GetMainRenderPass(),
            vulkanWindow->FrameBuffers[vulkanWindow->ImageIndex],
            vulkanWindow->Extent
        );
    }
}