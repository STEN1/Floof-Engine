#include "Scene.h"

namespace FLOOF {
    Scene::Scene() {
        m_PhysicSystem = std::make_shared<PhysicsSystem>(m_Scene);
        m_PhysicsDebugDrawer = new PhysicsDebugDraw();

        m_PhysicsDebugDrawer->setDebugMode(btIDebugDraw::DBG_NoDebug);
       auto world =  m_PhysicSystem->GetWorld();
       world->setDebugDrawer(m_PhysicsDebugDrawer);

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

    void Scene::OnUpdate(float deltaTime) {
        m_PhysicSystem->OnUpdate(deltaTime);
    }
}