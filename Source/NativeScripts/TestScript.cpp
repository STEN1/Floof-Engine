#include "TestScript.h"

namespace FLOOF {
	void TestScript::OnCreate(std::shared_ptr<Scene> scene, entt::entity entity) {
		std::cout << "Hello from test script!\n";
	}
}