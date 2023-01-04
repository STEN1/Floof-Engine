#include "CheckPointScript.h"
#include "../Components.h"
#include "RaceTrackScript.h"

void FLOOF::CheckPointScript::OnCreate(FLOOF::Scene *scene, entt::entity entity) {
    NativeScript::OnCreate(scene, entity);

    Trigger = entity;
    mCheckPointCollision = std::make_shared<CheckPointCollision>(scene, Trigger);

    auto &transform = m_Scene->GetComponent<TransformComponent>(entity);
    transform.Scale = glm::vec3(10.f);

    //create trigger
    auto &body = scene->AddComponent<TriggerVolumeComponent>(Trigger, transform.Position, transform.Scale, transform.Rotation, 0.f, bt::CollisionPrimitive::Box);
    body.setCollisionDispatcher(mCheckPointCollision.get());

    //create flags
    {
        Pole = m_Scene->CreateEntity("torii_gate", Trigger);
        auto &sm = m_Scene->AddComponent<StaticMeshComponent>(Pole, "Assets/torii_gate/scene.gltf");
        auto &GateTransform = m_Scene->GetComponent<TransformComponent>(entity);

        GateTransform = transform;

        GateTransform.Scale = glm::vec3(0.2f);

        { // add light
            const auto lightEntity = m_Scene->CreateEntity("Light", Pole);
            auto &light = m_Scene->AddComponent<PointLightComponent>(lightEntity);
            auto &transform = m_Scene->GetComponent<TransformComponent>(lightEntity);
            //auto &mesh = m_Scene->AddComponent<StaticMeshComponent>(lightEntity,"Assets/Ball.obj");
            transform.Position.x = 61.7f;
            transform.Position.y = 77.2f;

            light.innerRange = 64.f;
            light.outerRange = 128.f;
            light.intensity = 1000.f;
            light.diffuse = glm::vec4(1.f, 0.f, 0.f, 1.f); // red light
        }
        { // add light
            const auto lightEntity = m_Scene->CreateEntity("Light", Pole);
            auto &light = m_Scene->AddComponent<PointLightComponent>(lightEntity);
            auto &transform = m_Scene->GetComponent<TransformComponent>(lightEntity);
            //auto &mesh = m_Scene->AddComponent<StaticMeshComponent>(lightEntity,"Assets/Ball.obj");
            transform.Position.x = -58.5f;
            transform.Position.y = 77.2f;

            light.innerRange = 64.f;
            light.outerRange = 128.f;
            light.intensity = 1000.f;
            light.diffuse = glm::vec4(1.f, 0.f, 0.f, 1.f); // red light
        }
    }


    //add to physics world
    auto physics = m_Scene->GetPhysicSystem();
    physics->AddRigidBody(body.RigidBody.get());
}

void FLOOF::CheckPointScript::OnUpdate(float deltaTime) {
    NativeScript::OnUpdate(deltaTime);


}

void FLOOF::CheckPointScript::SetActive(bool active) {
    ActiveCheckPoint = active;
    mCheckPointCollision->IsActive = active;
    if (active) {
        auto *rel = m_Scene->TryGetComponent<Relationship>(Pole);
        if (rel) {
            for (auto child: rel->Children) {
                auto *light = TryGetComponent<PointLightComponent>(child);
                if (light) {
                    light->diffuse = glm::vec4(0.f, 1.f, 0.f, 1.f);
                }
            }
        }
    } else {
        auto *rel = m_Scene->TryGetComponent<Relationship>(Pole);
        if (rel) {
            for (auto child: rel->Children) {
                auto *light = TryGetComponent<PointLightComponent>(child);
                if (light) {
                    light->diffuse = glm::vec4(1.f, 0.f, 0.f, 1.f);
                }
            }
        }
    }

}

void FLOOF::CheckPointScript::CheckPointCollision::onBeginOverlap(void *obj1, void *obj2) {

    if (!IsActive)
        return;

    auto view = mScene->GetRegistry().view<NativeScriptComponent>();
    for (auto [entity, script]: view.each()) {
        auto cpScript = dynamic_cast<RaceTrackScript *>(script.Script.get());
        if (cpScript)
            cpScript->NextCheckPoint();
    }

    if (ImpactSound) { ImpactSound->Play(); }

}

void FLOOF::CheckPointScript::CheckPointCollision::onOverlap(void *obj1, void *obj2) {
    CollisionDispatcher::onOverlap(obj1, obj2);


}

void FLOOF::CheckPointScript::CheckPointCollision::onEndOverlap(void *obj) {
    CollisionDispatcher::onEndOverlap(obj);


}
