#include "CheckPointScript.h"
#include "../Components.h"
#include "RaceTrackScript.h"

void FLOOF::CheckPointScript::OnCreate(FLOOF::Scene *scene, entt::entity entity) {
    NativeScript::OnCreate(scene, entity);

    Trigger = entity;
    mCheckPointCollision = std::make_shared<CheckPointCollision>(scene, Trigger);

    auto &transform = m_Scene->GetComponent<TransformComponent>(entity);


    //create trigger
    auto &body = scene->AddComponent<TriggerVolumeComponent>(Trigger, transform.Position, transform.Scale, transform.Rotation, 0.f, bt::CollisionPrimitive::Box);
    body.setCollisionDispatcher(mCheckPointCollision.get());

    //create flags
    {
        Pole = scene->CreateEntity("Flag", Trigger);
        auto &mesh = m_Scene->AddComponent<StaticMeshComponent>(Pole, "Assets/Ball.obj");
        auto &tform = m_Scene->GetComponent<TransformComponent>(Pole);

        mesh.meshes[0].MeshMaterial.Diffuse = Texture(TextureColor::Red);
        mesh.meshes[0].MeshMaterial.Metallic = Texture(TextureColor::White);
        mesh.meshes[0].MeshMaterial.Normals = Texture(TextureColor::FlatNormal);
        mesh.meshes[0].MeshMaterial.Roughness = Texture(TextureColor::DarkGrey);
        mesh.meshes[0].MeshMaterial.UpdateDescriptorSet();
        
        auto &sound = scene->AddComponent<SoundComponent>(Pole, "checkpoint.wav");
		mCheckPointCollision->SetImpactSound(sound.GetClip("checkpoint.wav"));

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
        auto *mesh = m_Scene->TryGetComponent<StaticMeshComponent>(Pole);
        if (mesh) {
            mesh->meshes[0].MeshMaterial.Diffuse = Texture(TextureColor::Green);
            mesh->meshes[0].MeshMaterial.UpdateDescriptorSet();
        }
    } else {
        auto *mesh = m_Scene->TryGetComponent<StaticMeshComponent>(Pole);
        if (mesh) {
            mesh->meshes[0].MeshMaterial.Diffuse = Texture(TextureColor::Red);
            mesh->meshes[0].MeshMaterial.UpdateDescriptorSet();
        }
    }

}

void FLOOF::CheckPointScript::CheckPointCollision::onBeginOverlap(void *obj1, void *obj2) {

    if(!IsActive)
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
