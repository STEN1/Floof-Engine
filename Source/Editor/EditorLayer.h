#pragma once
#include "../ApplicationLayer.h"
#include "EditorPanels/ComponentsPanel.h"
#include "EditorPanels/SceneGraphPanel.h"
#include "entt/entt.hpp"
#include <unordered_map>
#include <string>

namespace FLOOF {
	class EditorLayer : public ApplicationLayer {
	public:
		EditorLayer();
		virtual void OnUpdate(float deltaTime) override;
		virtual void OnImGuiUpdate(float deltaTime) override;

	private:
		std::unordered_map<std::string, std::unique_ptr<EditorPanel>> m_EditorPanels;
	};
}