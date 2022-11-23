#pragma once
#include "EditorPanel.h"

namespace FLOOF {
	class SoundSettingsPanel : public EditorPanel {
	public:
		SoundSettingsPanel(EditorLayer* editorLayer) : EditorPanel(editorLayer) {}
		virtual void DrawPanel() override;
	};
}