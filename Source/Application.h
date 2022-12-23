#pragma once
#include "Renderer/VulkanRenderer.h"

#include <entt/entt.hpp>

#include <memory>
#include <unordered_map>
#include "ApplicationLayer.h"
#include "Network/FloofNetworking.h"

namespace FLOOF {
    class Application {
    public:
        enum LayerType{
            EDITOR,
            SERVER,
        };
    private:
        Application(LayerType layer);
    public:
        static Application& Get(LayerType layer) {
            static Application app(layer);
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
        std::unique_ptr<FloofClient> client;
        std::unique_ptr<FloofServer> server;
    private:

        void Create(LayerType layer);

        GLFWwindow* m_Window;
        ImGuiContext* m_ImguiContext;
        VulkanRenderer* m_Renderer;

        std::vector<std::unique_ptr<ApplicationLayer>> m_ApplicationLayers;

        // TODO: Move to editor layer?
        RenderPipelineKeys m_DrawMode{ RenderPipelineKeys::PBR };
    };
}
