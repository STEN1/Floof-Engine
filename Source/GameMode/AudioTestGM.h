#pragma once
#include "GameMode.h"

namespace FLOOF {
	class AudioTestGM : public GameMode {
		friend class Application;
	public:
		AudioTestGM(Scene& scene) : GameMode(scene) {};

	private:
		void OnCreate() override;

		void OnUpdateEditor(float deltaTime) override;
	};
}