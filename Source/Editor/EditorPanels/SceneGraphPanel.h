#pragma once
#include "EditorPanel.h"
#include "../../Components.h"

namespace FLOOF {
	class SceneGraphPanel : public EditorPanel {
	public:
		SceneGraphPanel(EditorLayer* editorLayer) : EditorPanel(editorLayer) {}
		virtual void DrawPanel() override;
	private:
		void MakeTreeNode(entt::entity entity, const char* tag, Relationship& rel);
	};
}