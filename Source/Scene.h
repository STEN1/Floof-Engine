#include "Floof.h"
#include <entt/entt.hpp>
// #include <physx.h> <3

namespace FLOOF {
    class Scene {
        Scene();
        // Get a finished render batch for scene renderer.
        entt::registry& GetCulledScene();

    private:
        entt::registry m_Scene;
    };
}