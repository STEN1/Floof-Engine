#include "SceneGraphPanel.h"
#include "../../Application.h"
#include "../EditorLayer.h"

namespace FLOOF {
	void SceneGraphPanel::DrawPanel()
	{
        ImGui::Begin("Scene graph");
        if (ImGui::Button("Add entity")) {
            AddEntity();
        }
        ImGui::Separator();
        auto view = m_EditorLayer->GetScene()->GetRegistry().view<TransformComponent, TagComponent, Relationship>();
        for (auto [entity, transform, tag, rel] : view.each()) {
            // only care about entitys without parent.
            // deal with child entitys later.
            if (rel.Parent != entt::null)
                continue;

            MakeTreeNode(entity, tag.Tag.c_str(), rel);
        }
        ImGui::End();
	}

    void SceneGraphPanel::AddEntity()
    {
        m_EditorLayer->GetScene()->CreateEntity();
    }

    void SceneGraphPanel::MakeTreeNode(entt::entity entity, const char* tag, Relationship& rel) {
        auto& app = Application::Get();
        static ImGuiTreeNodeFlags base_flags =
            ImGuiTreeNodeFlags_OpenOnArrow |
            ImGuiTreeNodeFlags_OpenOnDoubleClick |
            ImGuiTreeNodeFlags_SpanAvailWidth;

        ImGuiTreeNodeFlags node_flags = base_flags;

        if (entity == m_EditorLayer->GetScene()->m_SelectedEntity)
            node_flags |= ImGuiTreeNodeFlags_Selected;

        if (rel.Children.empty()) {
            // Parent without children are just nodes.
            // using tree node since it allows for selection
            node_flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
            ImGui::TreeNodeEx((void*)&entity, node_flags, "%s\t\tEntity id: %d", tag, static_cast<uint32_t>(entity));
            if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen()) {
                m_EditorLayer->GetScene()->m_SelectedEntity = entity;
            }

        } else {
            // is parent to children and has to make a tree
            bool node_open = ImGui::TreeNodeEx((void*)(intptr_t)entity, node_flags, "%s\t\tEntity id: %d", tag, static_cast<uint32_t>(entity));
            if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen()) {
                m_EditorLayer->GetScene()->m_SelectedEntity = entity;
            }

            if (node_open) {
                for (auto& childEntity : rel.Children) {
                    auto& childTag = m_EditorLayer->GetScene()->GetComponent<TagComponent>(childEntity);
                    auto& childRel = m_EditorLayer->GetScene()->GetComponent<Relationship>(childEntity);
                    MakeTreeNode(childEntity, childTag.Tag.c_str(), childRel);
                }
                ImGui::TreePop();
            }
        }
    }
}