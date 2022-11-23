#pragma once
#include "EditorPanel.h"

namespace FLOOF {
	class RendererPanel : public EditorPanel {
	public:
		RendererPanel(EditorLayer* editorLayer) : EditorPanel(editorLayer) {}
		virtual void DrawPanel() override;
	};
}