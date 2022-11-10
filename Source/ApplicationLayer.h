#pragma once

namespace FLOOF {
	class ApplicationLayer {
	public:
		virtual void OnUpdate(float deltaTime) {};
		virtual void OnImGuiUpdate(float deltaTime) {};
	};
}