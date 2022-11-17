#pragma once
#include "../ApplicationLayer.h"
#include "entt/entt.hpp"
#include <unordered_map>
#include <string>
#include "EditorPanels/EnginePanel.h"

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