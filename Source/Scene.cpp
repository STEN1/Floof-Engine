#include "Scene.h"

namespace FLOOF {
    Scene::Scene() {
        m_PhysicSystem = std::make_shared<PhysicsSystem>(m_Scene);
        m_PhysicsDebugDrawer = std::make_unique<PhysicsDebugDraw>();

        m_PhysicsDebugDrawer->setDebugMode(btIDebugDraw::DBG_NoDebug);
        auto world = m_PhysicSystem->GetWorld();
        world->setDebugDrawer(m_PhysicsDebugDrawer.get());

    }
    Scene::~Scene() {
    }

    entt::registry& Scene::GetCulledScene() {
        return m_Scene;
    }

    void Scene::OnUpdate(float deltaTime) {
        m_PhysicSystem->OnUpdate(deltaTime);
    }
}