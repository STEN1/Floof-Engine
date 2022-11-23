#pragma once
#include "entt/entt.hpp"

namespace FLOOF {
	class Scene;
	class NativeScript {
	public:
		virtual void OnCreate(Scene* scene, entt::entity entity) {};
		virtual void OnUpdate(float deltaTime) {};
        virtual void OnCollision(entt::entity& scriptOwner, entt::entity& collisionWith){};
	};
}