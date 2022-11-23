#include "TestScript.h"
#include <iostream>

namespace FLOOF {
	void TestScript::OnCreate(Scene* scene, entt::entity entity) {
		std::cout << "Hello from test script!\n";
	}
}