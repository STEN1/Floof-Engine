#include "Scene.h"

#include "Application.h"
#include "Components.h"
#include "SoundManager.h"

namespace FLOOF {
    Scene::Scene() {
        static uint32_t sceneID = 0;
        m_SceneID = sceneID++;

        m_SceneRenderer = std::make_unique<SceneRenderer>();

        m_PhysicSystem = std::make_unique<PhysicsSystem>(m_Registry);
        m_PhysicsDebugDrawer = std::make_unique<PhysicsDebugDraw>();

        m_PhysicsDebugDrawer->setDebugMode(btIDebugDraw::DBG_NoDebug);
        auto world = m_PhysicSystem->GetWorld();
        world->setDebugDrawer(m_PhysicsDebugDrawer.get());
    }

    Scene::~Scene() {
    }

    entt::entity Scene::CreateEntity(const std::string& tag, entt::entity parent) {
        entt::entity entity = m_Registry.create();

        auto& transform = m_Registry.emplace<TransformComponent>(entity);
        auto& rel = m_Registry.emplace<Relationship>(entity);
        auto& tagComponent = m_Registry.emplace<TagComponent>(entity);

        tagComponent.Tag = tag;

        if (parent != entt::null) {
            rel.Parent = parent;

            auto& parentRel = m_Registry.get<Relationship>(parent);
            auto& parentTransform = m_Registry.get<TransformComponent>(parent);

            transform.Parent = &parentTransform;
            parentRel.Children.push_back(entity);
        }

        return entity;
    }

    entt::registry& Scene::GetCulledScene() {
        return m_Registry;
    }

    entt::registry& Scene::GetRegistry() {
        return m_Registry;
    }

    CameraComponent* Scene::GetFirstSceneCamera()
    {
        CameraComponent* camera = &m_EditorCamera;

        auto view = m_Registry.view<CameraComponent>();
        for (auto [entity, cameraComp] : view.each()) {
            camera = &cameraComp;
            break;
        }

        return camera;
    }

    void Scene::OnUpdate(float deltaTime) {
        auto nativeScriptView = m_Registry.view<NativeScriptComponent>();
        for (auto [entity, nativeScript] : nativeScriptView.each()) {
            nativeScript.Script->OnUpdate(deltaTime);
        }
        auto PythonScriptView = m_Registry.view<ScriptComponent>();
        for (auto [entity, Script] : PythonScriptView.each()) {
            Script.OnUpdate(deltaTime);
        }

        m_PhysicSystem->OnUpdate(deltaTime);

        SoundManager::Update(this, GetFirstSceneCamera());
    }

    void Scene::OnCreate() {
        auto PythonScriptView = m_Registry.view<ScriptComponent>();
        for (auto [entity, Script] : PythonScriptView.each()) {
            Script.OnCreate();
        }
    }
}
