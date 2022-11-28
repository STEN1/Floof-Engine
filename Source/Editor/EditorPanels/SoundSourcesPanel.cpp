#include "SoundSourcesPanel.h"
#include "../../Application.h"
#include "../EditorLayer.h"

namespace FLOOF {
	void SoundSourcesPanel::DrawPanel() {
		ImGui::Begin("Sounds Sources");

		auto view = m_EditorLayer->GetScene()->GetRegistry().view<SoundSourceComponent, TagComponent, Relationship>();
		for (auto [entity, soundsource, tag, rel] : view.each()) {
			// only care about entitys without parent.
			// deal with child entitys later.
			if (rel.Parent != entt::null)
				continue;

			MakeTreeNode(entity, tag.Tag.c_str(), rel);
		}



		//if (m_EditorLayer->GetScene() && m_EditorLayer->GetScene()->m_SelectedEntity != entt::null) {
		//	if (auto* soundComponent = m_EditorLayer->GetScene()->GetRegistry().try_get<SoundSourceComponent>(
		//		m_EditorLayer->GetScene()->m_SelectedEntity)) {
		//		ImGui::Separator();
		//		ImGui::Text("Sound Component");
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
				//	}
				//}

		ImGui::End();
	}

	void SoundSourcesPanel::MakeTreeNode(entt::entity entity, const char* tag, Relationship& rel) {
		auto& app = Application::Get();
		static ImGuiTreeNodeFlags base_flags =
			ImGuiTreeNodeFlags_OpenOnArrow |
			ImGuiTreeNodeFlags_OpenOnDoubleClick |
			ImGuiTreeNodeFlags_SpanAvailWidth;

		ImGuiTreeNodeFlags node_flags = base_flags;

		//if (entity == m_EditorLayer->GetScene()->m_SelectedEntity)
		//	node_flags |= ImGuiTreeNodeFlags_Selected;


		// is parent to children and has to make a tree
		bool node_open = ImGui::TreeNodeEx((void*)(intptr_t)entity, node_flags, "%s\t\tEntity id: %d", tag, static_cast<uint32_t>(entity));


		if (node_open) {
			for (auto& clip : m_EditorLayer->GetScene()->GetRegistry().try_get<SoundSourceComponent>(entity)->mClips) {

				node_flags = base_flags;

				if (m_EditorLayer->GetScene()->GetRegistry().try_get<SoundSourceComponent>(entity)->m_SelectedClip == clip.second)
					node_flags |= ImGuiTreeNodeFlags_Selected;

				node_flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
				ImGui::TreeNodeEx((void*)&clip, node_flags, clip.second->m_Path.c_str());
				if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen()) {
					m_EditorLayer->GetScene()->m_SelectedEntity = entity;
					m_EditorLayer->GetScene()->GetRegistry().try_get<SoundSourceComponent>(entity)->m_SelectedClip = clip.second;
				}
			}
			ImGui::TreePop();
		}



		//if (rel.Children.empty()) {
		//	// Parent without children are just nodes.
		//	// using tree node since it allows for selection
		//	node_flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
		//	ImGui::TreeNodeEx((void*)&entity, node_flags, "%s\t\tEntity id: %d", tag, static_cast<uint32_t>(entity));
		//	if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen()) {
		//		m_EditorLayer->GetScene()->m_SelectedEntity = entity;
		//	}

		//}
		//else {
		//	// is parent to children and has to make a tree
		//	bool node_open = ImGui::TreeNodeEx((void*)(intptr_t)entity, node_flags, "%s\t\tEntity id: %d", tag, static_cast<uint32_t>(entity));
		//	if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen()) {
		//		m_EditorLayer->GetScene()->m_SelectedEntity = entity;
		//	}
		//	bool hasSoundChild = false;
		//	if (node_open) {
		//		for (auto& childEntity : rel.Children) {
		//			if (m_EditorLayer->GetScene()->GetRegistry().try_get<SoundSourceComponent>(childEntity)) {
		//				auto& childTag = m_EditorLayer->GetScene()->GetComponent<TagComponent>(childEntity);
		//				auto& childRel = m_EditorLayer->GetScene()->GetComponent<Relationship>(childEntity);
		//				MakeTreeNode(childEntity, childTag.Tag.c_str(), childRel);
		//			}
		//		}
		//		ImGui::TreePop();
		//	}
		//}
	}
}
