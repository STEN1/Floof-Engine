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
        /// Clears entt registry, has no vulkan safety. 
        /// Make sure no registry resource is being used on the gpu
        /// </summary>
        void Clear();
    private:
        entt::registry m_Scene;
    };
}