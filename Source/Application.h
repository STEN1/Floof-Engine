#pragma once
#include "Renderer/VulkanRenderer.h"

#include <entt/entt.hpp>

#include <unordered_map>
#include <memory>
#include <chrono>
#include <unordered_map>
#include "LasLoader.h"
#include "Scene.h"
#include "Renderer/SceneRenderer.h"
#include "Scene.h"
#include "Components.h"
#include "SoundManager.h"
#include "ApplicationLayer.h"

namespace FLOOF {
    static const char* ApplicationDrawModes[] = {
        "Wireframe",
        "UnLit",
        "PBR",
        "Normals",
        "UV",
    };
    class Application {
        friend class SoundManager;
        friend class ForwardSceneRenderer;
        friend class EditorLayer;
        friend class ApplicationPanel;
        friend class SceneGraphPanel;
        friend class ComponentsPanel;
        friend class RendererPanel;
        Application();
    public:
        int Run();
        static Application& Get() { 
            static Application app;
            return app; 
        }
    private:
        /**
         * @brief Updates GUI for application
        */
        void UpdateImGui(float deltaTime);
        
        /**
         * @brief Updates all cameras
        */
        void UpdateCameraSystem(float deltaTime);

        void Update(double deltaTime);

        void Draw();

        void CleanApplication();
        
        GLFWwindow* m_Window;
        ImGuiContext* m_ImguiContext;
        VulkanRenderer* m_Renderer;

        std::vector<std::unique_ptr<ApplicationLayer>> m_ApplicationLayers;

        void SelectDebugScene(DebugScenes type);
        DebugScenes m_CurrentDebugScene;

    public:
        PhysicsDebugDraw* GetPhysicsSystemDrawer() { return m_Scene->GetPhysicsDebugDrawer(); }

        void MakePhysicsScene();
        void MakePhysicsPlayGround();
        void MakeRenderingDemoScene();
        void MakeAudioTestScene();
        void MakeLandscapeScene();

        void SetDrawMode(RenderPipelineKeys drawMode) { m_DrawMode = drawMode; }
        RenderPipelineKeys GetDrawMode() { return m_DrawMode; }

        /**
         * @brief Temp, currently editor camera is set as render camera. 
         * @brief Rendercamera can be changed at any stage. (i.e playerController->GetCamera() -> SetRenderCamera).
         * @param cam 
        */
        void SetRenderCamera(CameraComponent& cam);

        /**
         * @brief SceneRenderer uses this function to get the render camera
         * @return Non owning camera pointer
        */
        CameraComponent* GetRenderCamera();

    private:
        /*SceneRenderer Objects*/     
        std::unique_ptr<SceneRenderer> m_SceneRenderer;

        /*SceneCamera, temp*/
        float m_CameraSpeed{ 100.f };
        CameraComponent m_EditorCamera;   
        CameraComponent* m_RenderCamera{nullptr};
        RenderPipelineKeys m_DrawMode{ RenderPipelineKeys::PBR };

    public:
        std::shared_ptr<Scene> m_Scene;
    };
}
