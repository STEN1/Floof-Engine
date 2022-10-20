#include "SceneRenderer.h"

namespace FLOOF {
    class ForwardSceneRenderer : public SceneRenderer {
    public:
        ForwardSceneRenderer();
        ~ForwardSceneRenderer();
        void Render(entt::registry& scene) override;
    };
}