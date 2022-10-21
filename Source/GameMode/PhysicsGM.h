#pragma once
#include "GameMode.h"
#include <glm/glm.hpp>
#include <memory>
#include "../PhysicsSystem.h"

namespace FLOOF
{
	class PhysicsGM : public GameMode
	{
		friend class Application;

		PhysicsGM(Scene& scene) : GameMode(scene) {};

		void OnCreate() override;

		void OnUpdateEditor(float deltaTime) override;

		const entt::entity SpawnBall(glm::vec3 location, const float radius, const float mass, const float elasticity = 0.5f, const std::string& texture = "Assets/LightBlue.png");


    private:


	};
}