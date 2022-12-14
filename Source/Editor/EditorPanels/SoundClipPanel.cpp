#include "SoundClipPanel.h"
#include "../../Application.h"
#include "../EditorLayer.h"
#include "../../Components.h"
#include "../../SoundComponent.h"

namespace FLOOF {
	void SoundClipPanel::DrawPanel() {
		ImGui::Begin("Sound Clip");


		if (m_EditorLayer->GetScene() && m_EditorLayer->GetScene()->m_SelectedClip) {
			
			ImGui::Separator();
			ImGui::Text("Sound Component");
			ImGui::Text(m_EditorLayer->GetScene()->m_SelectedClip->m_Path.c_str());

			if (ImGui::DragFloat("Volume", &m_EditorLayer->GetScene()->m_SelectedClip->m_Volume, 0.001f, 0.f, 1.f)) {
				m_EditorLayer->GetScene()->m_SelectedClip->Volume();
			}
			if (ImGui::DragFloat("Pitch", &m_EditorLayer->GetScene()->m_SelectedClip->m_Pitch, 0.01f, 0.f, 50.f)) {
				m_EditorLayer->GetScene()->m_SelectedClip->Pitch();
			}

			if (m_EditorLayer->GetScene()->m_SelectedClip->isPlaying) { if (ImGui::Button("Stop")) { m_EditorLayer->GetScene()->m_SelectedClip->Stop(); } }
			if (!m_EditorLayer->GetScene()->m_SelectedClip->isPlaying) { if (ImGui::Button("Play")) { m_EditorLayer->GetScene()->m_SelectedClip->Play(); } }
			if (m_EditorLayer->GetScene()->m_SelectedClip->isLooping) { if (ImGui::Button("Looping")) { m_EditorLayer->GetScene()->m_SelectedClip->Looping(false); } }
			if (!m_EditorLayer->GetScene()->m_SelectedClip->isLooping) { if (ImGui::Button("Not Looping")) { m_EditorLayer->GetScene()->m_SelectedClip->Looping(true); } }
			m_EditorLayer->GetScene()->m_SelectedClip->UpdateIsPlaying();
			
		}

		ImGui::End();
	}
}
