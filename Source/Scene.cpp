#include "Scene.h"
#include "Components.h"
#include "SoundComponent.h"
#include "Application.h"
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
        
        auto view = m_Registry.view<NativeScriptComponent>();
        for (auto [entity, script] : view.each()) {
            m_Registry.remove<NativeScriptComponent>(entity);
        }
        m_Registry.clear();
    }

    entt::entity Scene::CreateEntity(const std::string &tag, entt::entity parent) {
        entt::entity entity = m_Registry.create();

        auto &transform = m_Registry.emplace<TransformComponent>(entity);
        auto &rel = m_Registry.emplace<Relationship>(entity);
        auto &tagComponent = m_Registry.emplace<TagComponent>(entity);

        tagComponent.Tag = tag;

        if (parent != entt::null) {
            rel.Parent = parent;

            auto &parentRel = m_Registry.get<Relationship>(parent);
            auto &parentTransform = m_Registry.get<TransformComponent>(parent);

            transform.Parent = &parentTransform;
            parentRel.Children.push_back(entity);
        }

        return entity;
    }

    void Scene::DestroyEntity(entt::entity entity) {
        auto &rel = m_Registry.get<Relationship>(entity);

        if (rel.Parent != entt::null) {
            auto &parentRel = m_Registry.get<Relationship>(rel.Parent);
            std::remove(parentRel.Children.begin(), parentRel.Children.end(), entity);
        }

        for (auto childEntity: rel.Children) {
            DestroyChildEntity(childEntity);

        }
        //remove from physics
        if(m_PhysicSystem){
            auto *rigid = TryGetComponent<RigidBodyComponent>(entity);
            if (rigid) {
                for(int i {0}; i < rigid->RigidBody->getNumConstraintRefs(); i++){
                    auto* ref = rigid->RigidBody->getConstraintRef(i);
                    m_PhysicSystem->GetWorld()->removeConstraint(ref);
                }

                m_PhysicSystem->GetWorld()->removeRigidBody(rigid->RigidBody.get());
            }
        }

        m_Registry.destroy(entity);
    }

    void Scene::DestroyChildEntity(entt::entity entity) {
        auto &rel = m_Registry.get<Relationship>(entity);

        for (auto childEntity: rel.Children) {
            DestroyChildEntity(childEntity);
        }
        //remove from physics
        if(m_PhysicSystem){
            auto *rigid = TryGetComponent<RigidBodyComponent>(entity);
            if (rigid) {
                for(int i {0}; i < rigid->RigidBody->getNumConstraintRefs(); i++){
                    auto* ref = rigid->RigidBody->getConstraintRef(i);
                    m_PhysicSystem->GetWorld()->removeConstraint(ref);
                }
                m_PhysicSystem->GetWorld()->removeRigidBody(rigid->RigidBody.get());
            }
        }

        m_Registry.destroy(entity);
    }

    entt::registry &Scene::GetCulledScene() {
        return m_Registry;
    }

    entt::registry &Scene::GetRegistry() {
        return m_Registry;
    }

    CameraComponent *Scene::GetFirstSceneCamera() {
        CameraComponent *camera = &m_EditorCamera;

        auto view = m_Registry.view<CameraComponent>();
        for (auto [entity, cameraComp]: view.each()) {
            camera = &cameraComp;
            break;
        }

        return camera;
    }

    void Scene::OnUpdate(float deltaTime) {
        auto nativeScriptView = m_Registry.view<NativeScriptComponent>();
        for (auto [entity, nativeScript]: nativeScriptView.each()) {
            nativeScript.Script->OnUpdate(deltaTime);
        }
        /* tmp remove Python
        auto PythonScriptView = m_Registry.view<ScriptComponent>();
        for (auto [entity, Script] : PythonScriptView.each()) {
            Script.OnUpdate(deltaTime);
        }
*/
        m_PhysicSystem->OnUpdate(deltaTime);

        SoundManager::Update(this, GetActiveCamera());

        for (auto [entity, nativeScript]: nativeScriptView.each()) {
            nativeScript.Script->LastUpdate(deltaTime);
        }
    }

    void Scene::OnCreate() {
        /* tmp remove python
        auto PythonScriptView = m_Registry.view<ScriptComponent>();
        for (auto [entity, Script]: PythonScriptView.each()) {
            Script.OnCreate();
        }
         */
    }

    CameraComponent *Scene::GetActiveCamera() {
        CameraComponent *camera = &m_EditorCamera;

        auto view = m_Registry.view<PlayerControllerComponent, Relationship>();
        for (auto [entity, controller, rel]: view.each()) {
            if (controller.mPlayer == ActivePlayer) {
                for (auto ent: rel.Children) {
                    auto *cam = TryGetComponent<CameraComponent>(ent);
                    if (cam)
                        camera = cam;
                }
                break;
            }
        }

        return camera;
    }

    void Scene::OnEditorUpdate(float deltaTime) {
        auto nativeScriptView = m_Registry.view<NativeScriptComponent>();
        for (auto [entity, nativeScript]: nativeScriptView.each()) {
            nativeScript.Script->EditorUpdate(deltaTime);
        }
    }
}
