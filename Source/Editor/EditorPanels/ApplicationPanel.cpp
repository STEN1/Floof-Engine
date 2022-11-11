#include "ApplicationPanel.h"
#include "../../Application.h"
#include "../../Scene.h"

namespace FLOOF {
	void ApplicationPanel::DrawPanel()
	{
        auto& app = Application::Get();
        static int selectedScene = static_cast<int>(app.m_CurrentDebugScene);
        static int selectedDrawMode = static_cast<int>(app.m_DrawMode);

        ImGui::Begin("Application");
        if (ImGui::Combo("DrawMode",
            &selectedDrawMode,
            ApplicationDrawModes,
            IM_ARRAYSIZE(ApplicationDrawModes)))
        {
            app.SetDrawMode(static_cast<RenderPipelineKeys>(selectedDrawMode));
        }
        ImGui::NewLine();
        ImGui::Separator();
        ImGui::NewLine();
        if (ImGui::Combo("DebugScenes",
            &selectedScene,
            DebugSceneNames,
            IM_ARRAYSIZE(DebugSceneNames)))
        {
            app.SelectDebugScene(static_cast<DebugScenes>(selectedScene));
        }
        ImGui::End();
	}
}