#include "Scene.h"
#include "Components.h"

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

    entt::entity Scene::CreateEntity(const std::string& tag, entt::entity parent) {
        entt::entity entity = m_Scene.create();

        auto& transform = m_Scene.emplace<TransformComponent>(entity);
        auto& rel = m_Scene.emplace<Relationship>(entity);
        auto& tagComponent = m_Scene.emplace<TagComponent>(entity);

        tagComponent.Tag = tag;

        if (parent != entt::null) {
            rel.Parent = parent;

            auto& parentRel = m_Scene.get<Relationship>(parent);
            auto& parentTransform = m_Scene.get<TransformComponent>(parent);

            transform.Parent = &parentTransform;
            parentRel.Children.push_back(entity);
        }

        return entity;
    }

    entt::registry& Scene::GetCulledScene() {
        return m_Scene;
    }

    entt::registry& Scene::GetRegistry() {
        return m_Scene;
    }

    void Scene::OnUpdate(float deltaTime) {
        m_PhysicSystem->OnUpdate(deltaTime);
    }
}