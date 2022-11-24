#include "ApplicationPanel.h"
#include "../../Application.h"
#include "../../Scene.h"
#include "../EditorLayer.h"

namespace FLOOF {
	void ApplicationPanel::DrawPanel()
	{
        static int selectedScene = static_cast<int>(m_EditorLayer->GetCurrentDebugScene());

        ImGui::Begin("Application");
        if (ImGui::Combo("DebugScenes",
            &selectedScene,
            DebugSceneNames,
            IM_ARRAYSIZE(DebugSceneNames)))
        {
            m_EditorLayer->SelectDebugScene(static_cast<DebugScenes>(selectedScene));
        }
        ImGui::NewLine();
        ImGui::Separator();
        ImGui::NewLine();

        if (!m_EditorLayer->IsPlaying()) {
            if (ImGui::Button("Start play")) {
                m_EditorLayer->StartPlay();
            }
        } else {
            if (ImGui::Button("Stop play")) {
                m_EditorLayer->StopPlay();
            }
        }

        ImGui::End();
	}
}