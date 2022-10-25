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
    static const char* ApplicationDrawModes[] = {
        "Lit",
        "Wireframe",
    };

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

        void MakePhysicsScene();
        void MakeSponsaScene();

        /*GameMode Methods*/
        void SetGameModeType(GameModeType type);
        GameModeType GetGameModeType() const;

        /*SceneRenderer Methods*/
        void SetRendererType(SceneRendererType type);
        SceneRendererType GetRendererType() const;

        void SetDrawMode(RenderPipelineKeys drawMode) { m_DrawMode = drawMode; }
        RenderPipelineKeys GetDrawMode() { return m_DrawMode; }

        /**
         * @brief Temp, currently editor camera is set as render camera. 
         * @brief Rendercamera can be changed at any stage. (i.e playerController->GetCamera() -> SetRenderCamera).
         * @param cam 
        */
        void SetRenderCamera(CameraComponent& cam);

        // SceneRenderer uses this function to get the render camera
        CameraComponent* GetRenderCamera();

    private:
        /*SceneRenderer Objects*/     
        std::unique_ptr<SceneRenderer> m_SceneRenderer;
        SceneRendererType m_SceneRendererType{ SceneRendererType::Forward };

        /*SceneCamera, temp*/
        float m_CameraSpeed{ 100.f };
        CameraComponent m_EditorCamera;   
        CameraComponent* m_RenderCamera{nullptr};

        /*GameMode, temp*/
        std::unique_ptr<GameMode> m_GameMode;
        GameModeType m_GameModeType{ GameModeType::Physics };
        RenderPipelineKeys m_DrawMode{ RenderPipelineKeys::Basic };

        std::unique_ptr<Scene> m_Scene;
    };
}