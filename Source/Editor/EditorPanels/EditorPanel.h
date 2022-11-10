#pragma once
#include "entt/entt.hpp"
#include "imgui.h"

namespace FLOOF {
	class EditorPanel {
	public:
		virtual void DrawPanel() = 0;
	};
}