#pragma once

#include "Floof.h"
#include <entt/entt.hpp>
// #include <physx.h> <3

namespace FLOOF {
    class Scene {
        friend class Application;

        Scene();

    public:
        // Get a finished render batch for scene renderer.
        entt::registry& GetCulledScene();
    private:
        /// <summary>
        /// Clears entt registry
        /// </summary>
        void Clear();
    private:
        entt::registry m_Scene;
    };
}