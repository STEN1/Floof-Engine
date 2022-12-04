#pragma once

#include "Floof.h"
#include <entt/entt.hpp>
#include "PhysicsSystem.h"
#include "Renderer/SceneRenderer.h"
#include "Components.h"

namespace FLOOF {

    enum DebugScenes {
        Physics,
        PhysicsPlayground,
        Audio,
        Landscape,
        RenderingDemo,
        Sponza,
    };
    static const char* DebugSceneNames[] = {
        "Physics",
        "Physics Playground",
        "Audio",
        "Landscape",
        "RenderingDemo",
        "Sponza",
    };

    class Scene {
        friend class Application;
        friend class SceneRenderer;
        friend class EditorLayer;
        friend class SceneGraphPanel;
        friend class ComponentsPanel;
        friend class EditorLayer;
        friend class SoundSourcesPanel;
        friend class SoundClipPanel;

    public:
        Scene();
        ~Scene();
        // Get a finished render batch for scene renderer.
        entt::registry& GetCulledScene();

        // Get ref to scene registry.
        entt::registry& GetRegistry();

        bool IsActiveScene() { return m_IsActiveScene; }

        CameraComponent* GetEditorCamera() { return &m_EditorCamera; }

        // Returns the first scene camera, or the
        // editor camera if no camera was found.
        CameraComponent* GetFirstSceneCamera();
        CameraComponent* GetActiveCamera();

        uint32_t GetSceneID() { return m_SceneID; }

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
            return m_Registry.emplace<Type>(entity, std::forward<Args>(args)...);
        }

        template<typename Type>
        Type& GetComponent(entt::entity entity) {
            return m_Registry.get<Type>(entity);
        }

        template<typename Type>
        Type* TryGetComponent(entt::entity entity) {
            return m_Registry.try_get<Type>(entity);
        }

        PhysicsSystem* GetPhysicSystem(){return m_PhysicSystem.get();}
        PhysicsDebugDraw* GetPhysicsDebugDrawer() { return m_PhysicsDebugDrawer.get(); }

        int ActivePlayer{0};
    private:
        void OnUpdate(float deltaTime);
        void OnEditorUpdate(float deltaTime);
        void OnCreate();
    private:
        entt::registry m_Registry;
        std::unique_ptr<PhysicsSystem> m_PhysicSystem;       
        std::unique_ptr<PhysicsDebugDraw> m_PhysicsDebugDrawer;

        entt::entity m_SelectedEntity = entt::null;
        std::shared_ptr<SoundSourceComponent::SoundClip> m_SelectedClip;
        entt::entity m_LastSelectedEntity = entt::null;

        std::unique_ptr<SceneRenderer> m_SceneRenderer;
        uint32_t m_SceneID;
        bool m_IsActiveScene = false;

        CameraComponent m_EditorCamera;

    };
}