#pragma once
#include "GameMode.h"

namespace FLOOF {
	class HeightmapTest : public GameMode {
		friend class Application;
	public:
		HeightmapTest(Scene& scene) : GameMode(scene) {};

	private:
		void OnCreate() override;

		void OnUpdateEditor(float deltaTime) override;
	};
}