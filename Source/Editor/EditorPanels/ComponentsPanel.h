#pragma once
#include "EditorPanel.h"

namespace FLOOF {
	class ComponentsPanel : public EditorPanel {
	public:
		ComponentsPanel(EditorLayer* editorLayer) : EditorPanel(editorLayer) {}
		virtual void DrawPanel();
	};
}