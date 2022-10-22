#pragma once

#include "Floof.h"
#include <entt/entt.hpp>
// #include <physx.h> <3
#include "PhysicsSystem.h"

namespace FLOOF {
    class Scene {
        friend class Application;
    public:
        Scene();
        // Get a finished render batch for scene renderer.
        entt::registry& GetCulledScene();

        std::shared_ptr<PhysicsSystem> GetPhysicSystem(){return m_PhysicSystem;}
        PhysicsDebugDraw* GetPhysicsDebugDrawer() { return m_PhysicsDebugDrawer; }
    private:
        /// <summary>
        /// Clears entt registry, has no vulkan safety. 
        /// Make sure no registry resource is being used on the gpu
        /// </summary>
        void Clear();
        void ClearDebugSystem();
        void OnUpdatePhysics(float deltaTime);
    private:
        entt::registry m_Scene;
        std::shared_ptr<PhysicsSystem> m_PhysicSystem;
        PhysicsDebugDraw* m_PhysicsDebugDrawer;
    };
}