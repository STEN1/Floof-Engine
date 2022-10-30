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
	public:
		PhysicsGM(Scene& scene) : GameMode(scene) {};

    private:
		void OnCreate() override;

		void OnUpdateEditor(float deltaTime) override;

		const entt::entity SpawnBall(glm::vec3 location, const float radius, const float mass, const float elasticity = 0.5f, const std::string& texture = "Assets/LightBlue.png");

        const entt::entity SpawnSoftBall(glm::vec3 location, const float radius, const float mass, const std::string& texture ="Assets/LightBlue.png");

        const entt::entity SpawnCube(glm::vec3 Location, glm::vec3 Extents, const float mass, const std::string& texture = "Assets/LightBlue.png");

        const entt::entity SpawnStatue(glm::vec3 Location, glm::vec3 Scale, const float mass);


	};
}