#include "SoundSourcesPanel.h"
#include "../../Application.h"
#include "../EditorLayer.h"
#include "../../SoundComponent.h"
namespace FLOOF {
	void SoundSourcesPanel::DrawPanel() {
		ImGui::Begin("Sounds Sources");

		auto view = m_EditorLayer->GetScene()->GetRegistry().view<SoundComponent, TagComponent, Relationship>();
		for (auto [entity, soundsource, tag, rel] : view.each()) {
			// only care about entitys without parent.
			// deal with child entitys later.
			//if (rel.Parent != entt::null)
			//	continue;

			MakeTreeNode(entity, tag.Tag.c_str(), rel);
		}

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
			for (auto& clip : m_EditorLayer->GetScene()->GetRegistry().try_get<SoundComponent>(entity)->mClips) {

				node_flags = base_flags;

				if (m_EditorLayer->GetScene()->m_SelectedClip == clip.second.get())
					node_flags |= ImGuiTreeNodeFlags_Selected;

				node_flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
				ImGui::TreeNodeEx((void*)&clip, node_flags, clip.second->m_Path.c_str());
				if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen()) {
					m_EditorLayer->GetScene()->m_SelectedEntity = entity;
					m_EditorLayer->GetScene()->m_SelectedClip = clip.second.get();
				}
			}
			ImGui::TreePop();
		}
	}
}
