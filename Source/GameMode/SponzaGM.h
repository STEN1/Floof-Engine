#pragma once
#include "GameMode.h"
#include <glm/glm.hpp>

namespace FLOOF
{
	class SponzaGM : public GameMode
	{
		friend class Application;

	public:
		SponzaGM(Scene& scene) : GameMode(scene) {};

	private:
		void OnCreate() override;

		void OnUpdateEditor(float deltaTime) override;

		const void SpawnBall(glm::vec3 location, const float radius, const float mass, const float elasticity = 0.5f, const std::string& texture = "Assets/LightBlue.png");
	};
}