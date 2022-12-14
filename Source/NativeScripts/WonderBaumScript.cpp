#include "WonderBaumScript.h"
#include "../Components.h"
#include "../Application.h"
#include "BulletSoftBody/btSoftBodyHelpers.h"

void FLOOF::WonderBaumScript::OnCreate(FLOOF::Scene *scene, entt::entity entity) {
    NativeScript::OnCreate(scene, entity);

    auto &mesh = scene->AddComponent<StaticMeshComponent>(entity, "Assets/IdentityCube.obj");
    //textures

    auto &transform = scene->GetComponent<TransformComponent>(entity);

    auto &body = scene->AddComponent<RigidBodyComponent>(entity, transform.Position, transform.Scale, transform.Rotation, 100.f, bt::CollisionPrimitive::Box);
    auto &sound = scene->AddComponent<SoundComponent>(entity, "Voices, Baby, Toddler, 18 Months Old, Boy, Say Banana 02 SND23664.wav");
    //sound.Play();

    auto rope = scene->CreateEntity("Rope");
    btSoftBody *psb = btSoftBodyHelpers::CreateRope(*m_Scene->GetPhysicSystem()->getSoftBodyWorldInfo(), btVector3(0.f,0.f,0.f) , btVector3(0.f,0.f,0.f) + btVector3(10, 0, 0), 8, 1);
    psb->setTotalMass(50);
    //psb->appendAnchor(psb->m_nodes.size() - 1, body.RigidBody.get());

    auto& soft = scene->AddComponent<SoftBodyComponent>(rope, 0.1,0.1,1,psb);

}
