#pragma once
#include "Renderer/VulkanRenderer.h"

#include <entt/entt.hpp>

#include <unordered_map>
#include "Logger.h"
#include <memory>
#include <chrono>
#include <unordered_map>
#include "Physics.h"
#include "LasLoader.h"
#include "Scene.h"
#include "Renderer/SceneRenderer.h"
#include "GameMode/GameMode.h"
#include "Scene.h"
#include "Components.h"

namespace FLOOF {
    class Application {
        Application();
        ~Application();
        
    public:
        int Run();

        static Application& Get()
        {
            static Application app;
            return app;
        };
    private:
        /// <summary>
        /// Updates GUI for application
        /// </summary>
        /// <param name="deltaTime"></param>
        void UpdateImGui(float deltaTime);

        /// <summary>
        /// Updates all cameras
        /// </summary>
        /// <param name="deltaTime"></param>
        void UpdateCameraSystem(float deltaTime);

        void Update(double deltaTime);

        void Draw();
        
        GLFWwindow* m_Window;
        ImGuiContext* m_ImguiContext;
        VulkanRenderer* m_Renderer;

    public:
        PhysicsDebugDraw* GetPhysicsSystemDrawer() { return m_Scene->GetPhysicsDebugDrawer(); }

        /*GameMode Methods*/
        void SetGameModeType(GameModeType type);
        GameModeType GetGameModeType() const;

        /*SceneRenderer Methods*/
        void SetRendererType(SceneRendererType type);
        SceneRendererType GetRendererType() const;

        /// <summary>
        /// Temp, currently editor camera is set as render camera.
        /// Rendercamera can be changed at any stage 
        /// (i.e playerController->GetCamera() -> SetRenderCamera)
        /// </summary>
        void SetRenderCamera(CameraComponent& cam);

        /// <summary>
        /// SceneRenderer uses this function to get the render camera
        /// </summary>
        CameraComponent* GetRenderCamera();
    private: 
        /// <summary>
        /// Creates a new scene renderer based on current SceneRenderType
        /// </summary>
        void CreateSceneRenderer();

        /// <summary>
        /// Deletes current scene renderer
        /// </summary>
        void DestroySceneRenderer();

        /// <summary>
        /// Deletes and Creates a new SceneRenderer based on SceneRendererType
        /// </summary>
        void UpdateSceneRenderer();

        /// <summary>
        /// Creates a new game mode based on current GameModeType
        /// </summary>
        void CreateGameMode();

        /// <summary>
        /// Deletes current game mode
        /// Clears scene too(for now)
        /// </summary>
        void DestroyGameMode();

        /// <summary>
        /// Deletes and Creates a new game mode based on GameModeType
        /// </summary>
        void UpdateGameMode();

        /*SceneRenderer Objects*/     
        class SceneRenderer* m_SceneRenderer{nullptr};
        SceneRendererType m_SceneRendererType{ SceneRendererType::Forward };

        /*SceneCamera, temp*/
        float m_CameraSpeed{ 100.f };
        CameraComponent m_EditorCamera;   
        CameraComponent* m_RenderCamera{nullptr};

        /*GameMode, temp*/
        GameMode* m_GameMode{ nullptr };
        GameModeType m_GameModeType{ GameModeType::Physics };
        
        /// <summary>
        /// Currently being cleared on game mode change
        /// </summary>
        std::unique_ptr<Scene> m_Scene;
    };
}