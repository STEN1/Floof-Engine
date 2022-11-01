#include "DeferredSceneRenderer.h"
#include "../Scene.h"

namespace FLOOF {
    DeferredSceneRenderer::DeferredSceneRenderer() {
    }
    DeferredSceneRenderer::~DeferredSceneRenderer() {
    }

    VkDescriptorSet DeferredSceneRenderer::RenderToTexture(std::shared_ptr<Scene> scene, glm::vec2 extent) {

        return VK_NULL_HANDLE;
    }
}