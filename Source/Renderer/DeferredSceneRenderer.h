#pragma once

#include "SceneRenderer.h"

namespace FLOOF {
    class DeferredSceneRenderer : public SceneRenderer {
    public:
        DeferredSceneRenderer();
        ~DeferredSceneRenderer();
        void Render(entt::registry& scene) override;
    };
}