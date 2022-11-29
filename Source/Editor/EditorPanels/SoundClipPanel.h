#pragma once
#include "EditorPanel.h"
#include "../../Components.h"

namespace FLOOF {
	class SoundClipPanel : public EditorPanel {
	public:
		SoundClipPanel(EditorLayer* editorLayer) : EditorPanel(editorLayer) {}
		virtual void DrawPanel() override;
	};
}