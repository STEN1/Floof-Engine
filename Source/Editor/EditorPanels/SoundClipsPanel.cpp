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

				ImGui::Text(soundComponent->m_SelectedClip->m_Path.c_str());

				if (ImGui::DragFloat("Volume", &soundComponent->m_Volume, 0.001f, 0.f, 1.f)) {
					soundComponent->m_SelectedClip->Update();
				}
				if (ImGui::DragFloat("Pitch", &soundComponent->m_SelectedClip->m_Pitch, 0.01f, 0.f, 50.f)) {
					soundComponent->m_SelectedClip->Pitch();
				}

				if (soundComponent->m_SelectedClip->isPlaying) { if (ImGui::Button("Stop")) { soundComponent->m_SelectedClip->Stop(); } }
				if (!soundComponent->m_SelectedClip->isPlaying) { if (ImGui::Button("Play")) { soundComponent->m_SelectedClip->Play(); } }
				if (soundComponent->m_SelectedClip->isLooping) { if (ImGui::Button("Looping")) { soundComponent->m_SelectedClip->Looping(false); } }
				if (!soundComponent->m_SelectedClip->isLooping) { if (ImGui::Button("Not Looping")) { soundComponent->m_SelectedClip->Looping(true); } }
			}
		}

		ImGui::End();
	}
}
