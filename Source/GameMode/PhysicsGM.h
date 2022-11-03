#pragma once
#include "GameMode.h"
#include <glm/glm.hpp>
#include <memory>
#include "../PhysicsSystem.h"
#include "../Components.h"

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

        const entt::entity SpawnSoftMesh(glm::vec3 Location, glm::vec3 Scale, const float mass, const std::string FilePath, const std::string Texture);

        const entt::entity SpawnRigidMesh(glm::vec3 Location, glm::vec3 Scale, const float mass, const std::string FilePath, const std::string Texture, bt::CollisionPrimitive shape = bt::CollisionPrimitive::ConvexHull);

	};
}