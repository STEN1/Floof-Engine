#pragma once

#include "SceneRenderer.h"

namespace FLOOF {
    class DeferredSceneRenderer : public SceneRenderer {
    public:
        DeferredSceneRenderer();
        ~DeferredSceneRenderer();
        VkDescriptorSet RenderToTexture(entt::registry& scene, glm::vec2 extent) override;
    };
}