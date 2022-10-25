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
        ~Scene();
        // Get a finished render batch for scene renderer.
        entt::registry& GetCulledScene();

        // Get ref to scene registry.
        entt::registry& GetRegistry();

        /**
         * @brief Creates entity with Transform, tag, and relationship components.
         * @param tag: Entity name tag.
         * @return The created entity.
        */
        entt::entity CreateEntity(const std::string& tag = "Entity", entt::entity parent = entt::null);

        /**
         * @brief Adds component to the given entity with constructor args.
         * @return Reference to the added component.
        */
        template<typename Type, typename... Args>
        Type& AddComponent(entt::entity entity, Args &&...args) {
            return m_Scene.emplace<Type>(entity, std::forward<Args>(args)...);
        }

        template<typename Type>
        Type& GetComponent(entt::entity entity) {
            return m_Scene.get<Type>(entity);
        }

        std::shared_ptr<PhysicsSystem> GetPhysicSystem(){return m_PhysicSystem;}
        PhysicsDebugDraw* GetPhysicsDebugDrawer() { return m_PhysicsDebugDrawer.get(); }
    private:
        /// <summary>
        /// Clears entt registry, has no vulkan safety. 
        /// Make sure no registry resource is being used on the gpu
        /// </summary>
        void OnUpdate(float deltaTime);
    private:
        entt::registry m_Scene;
        std::shared_ptr<PhysicsSystem> m_PhysicSystem;       
        std::unique_ptr<PhysicsDebugDraw> m_PhysicsDebugDrawer;
    };
}