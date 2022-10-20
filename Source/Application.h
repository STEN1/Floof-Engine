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
   
        entt::entity m_CameraEntity;
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

        void DrawDebugLines();

        void Draw();
        
        GLFWwindow* m_Window;
        ImGuiContext* m_ImguiContext;
        VulkanRenderer* m_Renderer;
       
        uint32_t m_MaxBSplineLines = 1000;

        enum DebugLine {
            WorldAxis = 0,
            TerrainTriangle,
            CollisionTriangle,
            Velocity,
            Acceleration,
            Friction,
            CollisionShape,
            ClosestPointToBall,
            GravitationalPull,
            Force,
            Path,
            BSpline,
            OctTree
        };
        std::unordered_map<DebugLine, bool> m_BDebugLines;
        std::chrono::high_resolution_clock::time_point m_Ballspawntime;

        float m_DeltaTimeModifier{ 1.f };

        // ----------- Debug utils ---------------
        void DebugInit();
        void DebugClearLineBuffer();
        void DebugClearSpherePositions();
        void DebugClearAABBTransforms();
        void DebugClearDebugData();
        void DebugUpdateLineBuffer();
        void DebugToggleDrawNormals();
        void DebugDrawPath(std::vector<glm::vec3>& path);
        bool m_BShowPointcloud{ false };
        // Draws line in world coords
        void DebugDrawLine(const glm::vec3& start, const glm::vec3& end, const glm::vec3 color);

        // Draws triangle in world coords
        void DebugDrawTriangle(const Triangle& triangle, const glm::vec3& color);
        std::vector<ColorVertex> m_DebugLineBuffer;
        entt::entity m_DebugLineEntity;

        // Draws a sphere in world coords with specified radius
        void DebugDrawSphere(const glm::vec3& pos, float radius);
        std::vector<glm::mat4> m_DebugSphereTransforms;
        entt::entity m_DebugSphereEntity;

        // Draws a AABB in world coords
        void DebugDrawAABB(const glm::vec3& pos, const glm::vec3& extents);
        std::vector<glm::mat4> m_DebugAABBTransforms;
        entt::entity m_DebugAABBEntity;

        bool m_DebugDraw = true;
        bool m_DrawNormals = false;
        bool m_ShowImguiDemo = false;
        uint32_t m_DebugLineSpace = 9000000;

        float m_CameraSpeed{ 100.f };

    public:
        /*SceneRenderer Methods*/
        void SetRendererType(SceneRendererType type);
        SceneRendererType GetRendererType() const;

        /*GameMode Methods*/
        void SetGameMode(class GameMode* gm);
        const GameMode* GetGameMode() const;
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

        /*SceneRenderer Objects*/     
        class SceneRenderer* m_SceneRenderer{nullptr};
        SceneRendererType m_SceneRendererType{ SceneRendererType::Forward };

        /*GameMode, temp*/
        GameMode* m_GameMode{ nullptr };
        
        /*Scene objects*/
        Scene m_Scene;
    };
}