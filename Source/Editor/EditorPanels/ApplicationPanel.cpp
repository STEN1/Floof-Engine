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
        //choose player controller
        auto view = m_EditorLayer->GetScene()->GetRegistry().view<PlayerControllerComponent>();
        int Players{0};
        for (auto [entity,controller] : view.each()) {
            Players++;
        }
        std::string txt = "Player ID : ";
        txt +=std::to_string(m_EditorLayer->GetScene()->ActivePlayer);
        ImGui::Text(txt.c_str());

        ImGui::End();
	}
}