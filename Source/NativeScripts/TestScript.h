#pragma once
#include "NativeScript.h"

namespace FLOOF {
	class TestScript : public NativeScript {
	public:
		virtual void OnCreate(std::shared_ptr<Scene> scene, entt::entity entity) override;
	};
}