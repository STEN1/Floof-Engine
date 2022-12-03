#pragma once
#include "Renderer/VulkanRenderer.h"

#include <entt/entt.hpp>

#include <memory>
#include <unordered_map>
#include "ApplicationLayer.h"
#include "Network/FloofNetworking.h"

namespace FLOOF {
    class Application {
        Application();
    public:
        static Application& Get() { 
            static Application app;
            return app; 
        }

        int Run();

        void SetDrawMode(RenderPipelineKeys drawMode) { m_DrawMode = drawMode; }

        RenderPipelineKeys GetDrawMode() { return m_DrawMode; }

        void UpdateImGui(double deltaTime);

        void Update(double deltaTime);

        void Draw(double deltaTime);

        void CleanApplication();

    public:
        FloofClient client;
        std::unique_ptr<FloofServer> server;
    private:

        GLFWwindow* m_Window;
        ImGuiContext* m_ImguiContext;
        VulkanRenderer* m_Renderer;

        std::vector<std::unique_ptr<ApplicationLayer>> m_ApplicationLayers;

        // TODO: Move to editor layer?
        RenderPipelineKeys m_DrawMode{ RenderPipelineKeys::PBR };
    };
}
