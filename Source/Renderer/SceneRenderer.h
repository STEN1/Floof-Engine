#include "../Floof.h"
#include "VulkanRenderer.h"
#include <entt/entt.hpp>

namespace FLOOF {
    class SceneRenderer {
    public:
        SceneRenderer() = default;
        virtual ~SceneRenderer() = default;
        virtual void Render(entt::registry& scene) = 0;
    };
}