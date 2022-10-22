#include "Scene.h"

namespace FLOOF {
    Scene::Scene() {
        m_PhysicSystem = std::make_shared<PhysicsSystem>(m_Scene);
        m_PhysicsDebugDrawer = new PhysicsDebugDraw();
    }
    entt::registry& Scene::GetCulledScene() {
        return m_Scene;
    }
    void Scene::Clear()
    {
        m_Scene.clear();
        m_PhysicSystem->clear();
    }

    void Scene::ClearDebugSystem() {
        delete m_PhysicsDebugDrawer;
        m_PhysicsDebugDrawer = nullptr;
    }

    void Scene::OnUpdatePhysics(float deltaTime) {
        m_PhysicSystem->OnUpdate(deltaTime);
    }
}