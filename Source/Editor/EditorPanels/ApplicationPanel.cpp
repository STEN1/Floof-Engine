#include "ApplicationPanel.h"
#include "../../Application.h"

namespace FLOOF {
	void ApplicationPanel::DrawPanel()
	{
        auto& app = Application::Get();
        static int selectedRenderType = static_cast<int>(app.m_SceneRendererType);
        static int selectedGameType = static_cast<int>(app.m_GameModeType);
        static int selectedDrawMode = static_cast<int>(app.m_DrawMode);

        ImGui::Begin("Application");
        if (ImGui::Combo("SceneRendererType",
            &selectedRenderType,
            SceneRendererTypeStrings,
            IM_ARRAYSIZE(SceneRendererTypeStrings)))
        {
            app.SetRendererType(static_cast<SceneRendererType>(selectedRenderType));
        }
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
        if (ImGui::Combo("GameMode",
            &selectedGameType,
            GameModeTypeStrings,
            IM_ARRAYSIZE(GameModeTypeStrings)))
        {
            app.SetGameModeType(static_cast<GameModeType>(selectedGameType));
        }
        ImGui::End();
	}
}