#include "Scene.h"

namespace FLOOF {
    Scene::Scene() {
        m_PhysicSystem = std::make_shared<PhysicsSystem>(&this);
    }
    entt::registry& Scene::GetCulledScene() {
        return m_Scene;
    }
    void Scene::Clear()
    {
        m_Scene.clear();
    }

    void Scene::OnUpdatePhysics(float deltaTime) {
        m_PhysicSystem->OnUpdate(deltaTime);
    }
}