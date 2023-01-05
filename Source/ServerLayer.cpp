#include "ServerLayer.h"
#include "Application.h"
#include "NativeScripts/GameModeScript.h"
#include "NativeScripts/EnvironmentSoundScript.h"

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
        std::cout << "Server staret on port 9012" << std::endl;
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

    //terrain
    {
        auto entity = m_Scene->CreateEntity("Terrain");
        auto &mesh = m_Scene->AddComponent<LandscapeComponent>(entity, "Assets/Terrain/Terrain_Tough/heightmap.png", "Assets/Terrain/Terrain_Tough/texture.png");

        auto &transform = m_Scene->GetComponent<TransformComponent>(entity);

        auto *state = new btDefaultMotionState();
        btRigidBody::btRigidBodyConstructionInfo info(0, state, mesh.HeightFieldShape->mHeightfieldShape);
        auto *body = new btRigidBody(info);
        body->setFriction(1.5f);
        m_Scene->GetPhysicSystem()->AddRigidBody(body);
    }

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
        auto ent = m_Scene->CreateEntity("EnviromentSound");
        auto &script = m_Scene->AddComponent<NativeScriptComponent>(ent, std::make_unique<EnvironmentSoundScript>(), m_Scene.get(), ent);
    }

    //make ramps
    {

        struct ramptform {
            glm::vec3 pos{0.f};
            glm::vec3 rot{0.f};
            glm::vec3 scale{1.f};
        };
        std::vector<ramptform> ramps;

        ramptform tf;
        tf.pos = glm::vec3(-370.f,0.5f,133.f);
        tf.rot= glm::vec3(-1.8f,0.f,0.f);
        tf.scale = glm::vec3(0.1f,0.1f,0.06f);
        ramps.emplace_back(tf);
        //-370, 0.5, 133
        //-1.8,0,0
        //0.1, 0.1, 0.06

        //135, -44, 200
        //half pi, 2, 0
        //0.2, 0.05, 0.05
        tf.pos = glm::vec3(135.f,-44.f,200.f);
        tf.rot= glm::vec3(-glm::pi<float>()/2.f,2.f,0.f);
        tf.scale = glm::vec3(0.2f,0.05f,0.05f);
        ramps.emplace_back(tf);
        //56, -27, -279.1
        //-1.6, 5.6, 0
        //0.2
        tf.pos = glm::vec3(56.f,-27.f,-279.1f);
        tf.rot= glm::vec3(-1.6f,5.6f,0.f);
        tf.scale = glm::vec3(0.2f);
        ramps.emplace_back(tf);

        for (auto ramp: ramps) {
            auto extents = ramp.scale;
            auto location = ramp.pos;
            auto rotation = ramp.rot;
            float mass = 0.f;
            std::string name = "Ramp ";

            auto entity = m_Scene->CreateEntity(name);
            auto &collision = m_Scene->AddComponent<RigidBodyComponent>(entity, location, extents, rotation, mass, "Assets/ramp/scene.gltf");
            auto &mesh = m_Scene->AddComponent<StaticMeshComponent>(entity, "Assets/ramp/scene.gltf");
            auto &transform = m_Scene->GetComponent<TransformComponent>(entity);
            transform.Position = glm::vec3(collision.Transform.getOrigin().getX(), collision.Transform.getOrigin().getY(), collision.Transform.getOrigin().getZ());
            transform.Scale = extents;
            transform.Rotation = rotation;
            collision.RigidBody->setFriction(2.f);

        }
    }

}

