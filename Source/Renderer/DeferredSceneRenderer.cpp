#include "DeferredSceneRenderer.h"

namespace FLOOF {
    DeferredSceneRenderer::DeferredSceneRenderer() {
    }
    DeferredSceneRenderer::~DeferredSceneRenderer() {
    }

    VkDescriptorSet DeferredSceneRenderer::RenderToTexture(entt::registry& scene, glm::vec2 extent) {
        return VK_NULL_HANDLE;
    }
}