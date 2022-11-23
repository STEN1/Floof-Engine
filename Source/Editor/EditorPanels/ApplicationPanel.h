#pragma once
#include "EditorPanel.h"

namespace FLOOF {
	class ApplicationPanel : public EditorPanel {
	public:
		ApplicationPanel(EditorLayer* editorLayer) : EditorPanel(editorLayer) {}
		virtual void DrawPanel() override;
	};
}