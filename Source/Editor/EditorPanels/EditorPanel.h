#pragma once
#include "entt/entt.hpp"
#include "imgui.h"

namespace FLOOF {
	class EditorLayer;
	class EditorPanel {
	public:
		EditorPanel() = delete;
		EditorPanel(EditorLayer* editorLayer) : m_EditorLayer(editorLayer) {}
		virtual void DrawPanel() = 0;
	protected:
		EditorLayer* m_EditorLayer = nullptr;
	};
}