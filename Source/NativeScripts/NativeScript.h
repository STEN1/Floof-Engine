#pragma once
#include "../Scene.h"

namespace FLOOF {
	class NativeScript {
	public:
		virtual void OnCreate(std::shared_ptr<Scene> scene, entt::entity entity) {};
		virtual void OnUpdate(float deltaTime) {};
	};
}