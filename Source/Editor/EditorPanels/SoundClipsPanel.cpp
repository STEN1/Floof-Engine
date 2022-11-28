#include "SoundClipsPanel.h"
#include "../../Application.h"
#include "../EditorLayer.h"

namespace FLOOF {
	void SoundClipsPanel::DrawPanel() {
		ImGui::Begin("Sounds Clips");


		if (m_EditorLayer->GetScene() && m_EditorLayer->GetScene()->m_SelectedEntity != entt::null) {
			if (auto* soundComponent = m_EditorLayer->GetScene()->GetRegistry().try_get<SoundSourceComponent>(
				m_EditorLayer->GetScene()->m_SelectedEntity)) {
				ImGui::Separator();
				ImGui::Text("Sound Component");



				
				/*ImGui::Text(soundComponent->m_Path.c_str());

				if (ImGui::DragFloat("Volume", &soundComponent->m_Volume, 0.001f, 0.f, 1.f)) {
					soundComponent->Update();
				}
				if (ImGui::DragFloat("Pitch", &soundComponent->m_Pitch, 0.01f, 0.f, 50.f)) {
					soundComponent->Pitch();
				}

				if (soundComponent->isPlaying) { if (ImGui::Button("Stop")) { soundComponent->Stop(); } }
				if (!soundComponent->isPlaying) { if (ImGui::Button("Play")) { soundComponent->Play(); } }
				if (soundComponent->isLooping) { if (ImGui::Button("Looping")) { soundComponent->Looping(false); } }
				if (!soundComponent->isLooping) { if (ImGui::Button("Not Looping")) { soundComponent->Looping(true); } }*/
			}
		}

		ImGui::End();
	}
}
