#pragma once

#include "SceneRenderer.h"

namespace FLOOF {
    class DeferredSceneRenderer : public SceneRenderer {
    public:
        DeferredSceneRenderer();
        ~DeferredSceneRenderer();
        VkDescriptorSet RenderToTexture(std::shared_ptr<Scene> scene, glm::vec2 extent) override;
    };
}