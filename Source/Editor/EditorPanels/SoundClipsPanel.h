#pragma once
#include "EditorPanel.h"
#include "../../Components.h"

namespace FLOOF {
	class SoundClipsPanel : public EditorPanel {
	public:
		SoundClipsPanel(EditorLayer* editorLayer) : EditorPanel(editorLayer) {}
		virtual void DrawPanel() override;
	};
}