#include "ServerLayer.h"
#include "Application.h"
#include "NativeScripts/LightSwarm.h"
#include "NativeScripts/GameModeScript.h"

FLOOF::ServerLayer::ServerLayer() {

    MakeServerScene();
}

FLOOF::ServerLayer::~ServerLayer() {

}

void FLOOF::ServerLayer::OnUpdate(double deltaTime) {
    ApplicationLayer::OnUpdate(deltaTime);

    auto &Server = Application::Get().server;

    if (Server) {
        Server->Update();
    }
    else{
        Server = std::make_unique<FloofServer>(9012); //todo make port editable
        Server->mScene = m_Scene.get();

        Server->Start();
    }

    m_Scene->GetPhysicSystem()->OnEditorUpdate(deltaTime);

    if (IsPlaying())
        m_Scene->OnUpdate(deltaTime);
    else {
        m_Scene->OnEditorUpdate(deltaTime);
    }
}

void FLOOF::ServerLayer::OnImGuiUpdate(double deltaTime) {
    ApplicationLayer::OnImGuiUpdate(deltaTime);
}

VkSemaphore FLOOF::ServerLayer::OnDraw(double deltaTime, VkSemaphore waitSemaphore) {
    return ApplicationLayer::OnDraw(deltaTime, waitSemaphore);

}

void FLOOF::ServerLayer::StartPlay() {
    m_PlayModeActive = true;
}

void FLOOF::ServerLayer::StopPlay() {
    m_PlayModeActive = false;
}

void FLOOF::ServerLayer::MakeServerScene() {
    m_Scene = std::make_unique<Scene>();


    //Gamemode Script
    {
        auto ent = m_Scene->CreateEntity("GameMode");
        auto &script = m_Scene->AddComponent<NativeScriptComponent>(ent, std::make_unique<GameModeScript>(), m_Scene.get(), ent);

        const auto entity = m_Scene->CreateEntity("RaceTrack");
        m_Scene->AddComponent<NativeScriptComponent>(entity, std::make_unique<RaceTrackScript>(), m_Scene.get(), entity);

        auto cpScript = dynamic_cast<GameModeScript *>(script.Script.get());
        if (cpScript)
            cpScript->RaceTrack = entity;
    }

    {
        const auto entity = m_Scene->CreateEntity();
        auto &transform = m_Scene->GetComponent<TransformComponent>(entity);
        transform.Position = glm::vec4(0.f, 10.f, -0.5f, 1.f);
        m_Scene->AddComponent<StaticMeshComponent>(entity, "Assets/Ball.obj");
    }

    //make race track
    {

    }

    //make flooring
    {
        auto location = glm::vec3(0.f, -50.f, 0.f);
        auto extents = glm::vec3(1000.f, 5.f, 1000.f);
        auto mass = 0.f;

        auto entity = m_Scene->CreateEntity("flooring");
        auto &collision = m_Scene->AddComponent<RigidBodyComponent>(entity, location, extents, glm::vec3(0.f), mass, bt::CollisionPrimitive::Box);
        auto &mesh = m_Scene->AddComponent<StaticMeshComponent>(entity, "Assets/Primitives/IdentityCube.fbx");
        mesh.meshes[0].MeshMaterial.Diffuse = Texture("Assets/crisscross-foam1-ue/crisscross-foam_albedo.png");
        mesh.meshes[0].MeshMaterial.AO = Texture("Assets/crisscross-foam1-ue/crisscross-foam_ao.png");
        mesh.meshes[0].MeshMaterial.Metallic = Texture("Assets/crisscross-foam1-ue/crisscross-foam_metallic.png");
        mesh.meshes[0].MeshMaterial.Normals = Texture("Assets/crisscross-foam1-ue/crisscross-foam_normal-dx.png");
        mesh.meshes[0].MeshMaterial.Roughness = Texture("Assets/crisscross-foam1-ue/crisscross-foam_roughness.png");
        mesh.meshes[0].MeshMaterial.UpdateDescriptorSet();

        auto &transform = m_Scene->GetComponent<TransformComponent>(entity);
        transform.Position = glm::vec3(collision.Transform.getOrigin().getX(),
                                       collision.Transform.getOrigin().getY(),
                                       collision.Transform.getOrigin().getZ());
        transform.Scale = extents;
        collision.RigidBody->setFriction(1.0f);

        //place random ramps
        {
            auto mass = 0.f;

            //blanket textures;
            const char *albedos[4]{
                    "Assets/soft-blanket-ue/soft-blanket_Blue_albedo.png",
                    "Assets/soft-blanket-ue/soft-blanket_Pink_albedo.png",
                    "Assets/soft-blanket-ue/soft-blanket_Red_albedo.png",
                    "Assets/soft-blanket-ue/soft-blanket_Yellow_albedo.png",
            };

            for (int i{0}; i < 10.f; i++) {
                auto extents = glm::vec3(Math::RandFloat(5.f, 30.f), Math::RandFloat(2.f, 10.f), Math::RandFloat(5.f, 30.f));
                auto location = glm::vec3(Math::RandFloat(-200.f, 200.f), -43.f + extents.y, Math::RandFloat(-200.f, 200.f));
                auto rotation = glm::vec3(0.f, Math::RandFloat(0.f, 6.28f), 0.f);
                std::string name = "Random ramp ";
                name += std::to_string(i);

                auto entity = m_Scene->CreateEntity(name);
                auto &collision = m_Scene->AddComponent<RigidBodyComponent>(entity, location, extents, rotation, mass, "Assets/Primitives/IdentityRamp.fbx");
                auto &mesh = m_Scene->AddComponent<StaticMeshComponent>(entity, "Assets/Primitives/IdentityRamp.fbx");
                auto texture = Math::RandInt(0, 3);
                mesh.meshes[0].MeshMaterial.Diffuse = Texture(albedos[texture]);
                mesh.meshes[0].MeshMaterial.AO = Texture("Assets/soft-blanket-ue/soft-blanket_ao.png");
                mesh.meshes[0].MeshMaterial.Metallic = Texture("Assets/soft-blanket-ue/soft-blanket_metallic.png");
                mesh.meshes[0].MeshMaterial.Normals = Texture("Assets/soft-blanket-ue/soft-blanket_normal-dx.png");
                mesh.meshes[0].MeshMaterial.Roughness = Texture("Assets/soft-blanket-ue/soft-blanket_roughness.png");

                mesh.meshes[0].MeshMaterial.UpdateDescriptorSet();

                auto &transform = m_Scene->GetComponent<TransformComponent>(entity);
                transform.Position = glm::vec3(collision.Transform.getOrigin().getX(), collision.Transform.getOrigin().getY(), collision.Transform.getOrigin().getZ());
                transform.Scale = extents;
                transform.Rotation = rotation;
                collision.RigidBody->setFriction(0.9f);
            }
        }

    }
    {
        const auto lightSwarmEntity = m_Scene->CreateEntity();
        m_Scene->AddComponent<NativeScriptComponent>(lightSwarmEntity, std::make_unique<LightSwarm>(), m_Scene.get(), lightSwarmEntity);
    }
}
