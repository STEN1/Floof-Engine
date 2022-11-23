#include "ApplicationPanel.h"
#include "../../Application.h"
#include "../../Scene.h"
#include "../EditorLayer.h"

namespace FLOOF {
	void ApplicationPanel::DrawPanel()
	{
        auto& app = Application::Get();

        static int selectedScene = static_cast<int>(m_EditorLayer->GetCurrentDebugScene());

        ImGui::Begin("Application");
        if (ImGui::Combo("DebugScenes",
            &selectedScene,
            DebugSceneNames,
            IM_ARRAYSIZE(DebugSceneNames)))
        {
            m_EditorLayer->SelectDebugScene(static_cast<DebugScenes>(selectedScene));
        }
        ImGui::End();
	}
}