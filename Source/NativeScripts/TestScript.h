#pragma once
#include "NativeScript.h"

namespace FLOOF {
	class TestScript : public NativeScript {
	public:
		virtual void OnCreate(Scene* scene, entt::entity entity) override;
	};
}