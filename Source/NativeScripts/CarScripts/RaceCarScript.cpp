#include "RaceCarScript.h"
#include "../../SoundComponent.h"

void FLOOF::RaceCarScript::OnCreate(FLOOF::Scene *scene, entt::entity entity) {

    frame = entity;
    TruckCallback = std::make_shared<TruckCollisionCallback>(scene, entity);
    CarType = 1;

    BackWheelTurn = false;

    engine.maxEngineForce = 6000.f;
    engine.maxVelocity = 20.f;

    engine.suspensionStiffness = 20000.f;
    engine.suspensionDamping = 15000.f;
    engine.suspensionCompression = 4.4f;
    engine.WheelFriction = 4.f;

    engine.suspensionRestLength = 0.4;
    engine.TurnLowSpeed = SIMD_PI*0.1f;
    engine.TurnHighSpeed = SIMD_PI*0.05f;


    auto &mesh = scene->AddComponent<StaticMeshComponent>(frame, "Assets/2020_koenigsegg_jesco/jeskoNoWheel.gltf");

    {
        auto &transform = scene->GetComponent<TransformComponent>(frame);
        transform.Scale = glm::vec3(1.f);
        transform.Position = SpawnLocation;

        auto &body = scene->AddComponent<RigidBodyComponent>(frame, transform.Position, transform.Scale, transform.Rotation, 1500.f, "Assets/2020_koenigsegg_jesco/jeskoNoWheel.gltf");
        //body.RigidBody->setCollisionFlags(body.RigidBody->getCollisionFlags() | btCollisionObject::CF_NO_CONTACT_RESPONSE);
        body.setCollisionDispatcher(TruckCallback.get());



    }
    //Breaklight
    {
        BreakLight = scene->CreateEntity("BreakLight", frame);
        auto &light = scene->AddComponent<PointLightComponent>(BreakLight);
        //auto & mesh = scene->AddComponent<StaticMeshComponent>(BreakLight,"Assets/LowPolySphere.fbx");
        //mesh.meshes[0].MeshMaterial.Diffuse = Texture(TextureColor::Red);
        //mesh.meshes[0].MeshMaterial.UpdateDescriptorSet();

        light.intensity = 20.f;
        light.diffuse = glm::vec4(1.f, 0.f, 0.f, 1.f);

        auto &transform = scene->GetComponent<TransformComponent>(BreakLight);
        transform.Position = glm::vec3(-1.4f, 0.4f, 0.f);
        transform.Scale = glm::vec3(0.1f);
    }
    //Headlight Right
    {
        HeadLightR = scene->CreateEntity("Right Headlight", frame);
        auto &light = scene->AddComponent<PointLightComponent>(HeadLightR);
        auto & mesh = scene->AddComponent<StaticMeshComponent>(HeadLightR,"Assets/LowPolySphere.fbx");
        mesh.meshes[0].MeshMaterial.Diffuse = Texture(TextureColor::White);
        mesh.meshes[0].MeshMaterial.UpdateDescriptorSet();

        light.intensity = 10.f;

        auto &transform = scene->GetComponent<TransformComponent>(HeadLightR);
        transform.Position = glm::vec3(1.3f, 0.35f, -0.4f);
        transform.Scale = glm::vec3(0.1f);
    }
    //Headlight Left
    {
        HeadLightL = scene->CreateEntity("Left Headlight", frame);
        auto &light = scene->AddComponent<PointLightComponent>(HeadLightL);
        auto & mesh = scene->AddComponent<StaticMeshComponent>(HeadLightL,"Assets/LowPolySphere.fbx");
        mesh.meshes[0].MeshMaterial.Diffuse = Texture(TextureColor::White);
        mesh.meshes[0].MeshMaterial.UpdateDescriptorSet();

        light.intensity = 10.f;

        auto &transform = scene->GetComponent<TransformComponent>(HeadLightL);
        transform.Position = glm::vec3(1.3f, 0.35f, 0.4f);
        transform.Scale = glm::vec3(0.1f);
    }


    {
        Wheel_fr = scene->CreateEntity("Wheel Front Right");

        auto& sound = scene->AddComponent<SoundComponent>(Wheel_fr, "Vehicles_idle2.wav");
        auto &mesh = scene->AddComponent<StaticMeshComponent>(Wheel_fr, "Assets/Wheels/tuner-wheel/source/tunerWheelRight.fbx");
        mesh.meshes[1].MeshMaterial.Diffuse = Texture("Assets/Wheels/tuner-wheel/textures/TireColor2.png");
        mesh.meshes[1].MeshMaterial.Metallic = Texture(TextureColor::Black);
        mesh.meshes[1].MeshMaterial.Normals = Texture("Assets/Wheels/tuner-wheel/textures/TireNorm1.png");
        mesh.meshes[1].MeshMaterial.Roughness = Texture(TextureColor::White);
        mesh.meshes[1].MeshMaterial.UpdateDescriptorSet();

        mesh.meshes[0].MeshMaterial.Diffuse = Texture("Assets/Wheels/tuner-wheel/textures/WheelBaseColor1.png");
        mesh.meshes[0].MeshMaterial.Metallic = Texture("Assets/Wheels/tuner-wheel/textures/WheelBaseMetallic1.png");
        mesh.meshes[0].MeshMaterial.Roughness = Texture("Assets/Wheels/tuner-wheel/textures/WheelBaseRoughness1.png");
        mesh.meshes[0].MeshMaterial.UpdateDescriptorSet();

        mesh.meshes[2].MeshMaterial.Diffuse = Texture("Assets/Wheels/tuner-wheel/textures/WheelBaseColor1.png");
        mesh.meshes[2].MeshMaterial.Metallic = Texture("Assets/Wheels/tuner-wheel/textures/WheelBaseMetallic1.png");
        mesh.meshes[2].MeshMaterial.Roughness = Texture("Assets/Wheels/tuner-wheel/textures/WheelBaseRoughness1.png");
        mesh.meshes[2].MeshMaterial.UpdateDescriptorSet();


        auto &transform = scene->GetComponent<TransformComponent>(Wheel_fr);
        transform.Position = glm::vec3(5.8f, 0.6f, -3.1f) + SpawnLocation;
        transform.Scale = glm::vec3(2.8f);
        transform.Rotation = glm::vec3(glm::pi<float>() / 2.f, 0.f, 0.f);
        glm::vec3 scale = glm::vec3(1.4f, 0.6f, 1.4f);
        auto &body = scene->AddComponent<RigidBodyComponent>(Wheel_fr, transform.Position, scale, transform.Rotation, 300.f, bt::CollisionPrimitive::Cylinder);
        auto &RigidBody = body.RigidBody;
        RigidBody->setAnisotropicFriction(btVector3(1.f,0.5f,0.5f));
        RigidBody->setFriction(engine.WheelFriction);
        RigidBody->setRollingFriction(1);
        RigidBody->setSpinningFriction(1);
    }
    {
        Wheel_fl = scene->CreateEntity("Wheel Front Left");
        auto& sound = scene->AddComponent<SoundComponent>(Wheel_fl, "Vehicles_idle2.wav");
        auto &mesh = scene->AddComponent<StaticMeshComponent>(Wheel_fl, "Assets/Wheels/tuner-wheel/source/tunerWheelLeft.fbx");
        mesh.meshes[1].MeshMaterial.Diffuse = Texture("Assets/Wheels/tuner-wheel/textures/TireColor2.png");
        mesh.meshes[1].MeshMaterial.Metallic = Texture(TextureColor::Black);
        mesh.meshes[1].MeshMaterial.Normals = Texture("Assets/Wheels/tuner-wheel/textures/TireNorm1.png");
        mesh.meshes[1].MeshMaterial.Roughness = Texture(TextureColor::White);
        mesh.meshes[1].MeshMaterial.UpdateDescriptorSet();

        mesh.meshes[0].MeshMaterial.Diffuse = Texture("Assets/Wheels/tuner-wheel/textures/WheelBaseColor1.png");
        mesh.meshes[0].MeshMaterial.Metallic = Texture("Assets/Wheels/tuner-wheel/textures/WheelBaseMetallic1.png");
        mesh.meshes[0].MeshMaterial.Roughness = Texture("Assets/Wheels/tuner-wheel/textures/WheelBaseRoughness1.png");
        mesh.meshes[0].MeshMaterial.UpdateDescriptorSet();

        mesh.meshes[2].MeshMaterial.Diffuse = Texture("Assets/Wheels/tuner-wheel/textures/WheelBaseColor1.png");
        mesh.meshes[2].MeshMaterial.Metallic = Texture("Assets/Wheels/tuner-wheel/textures/WheelBaseMetallic1.png");
        mesh.meshes[2].MeshMaterial.Roughness = Texture("Assets/Wheels/tuner-wheel/textures/WheelBaseRoughness1.png");
        mesh.meshes[2].MeshMaterial.UpdateDescriptorSet();

        auto &transform = scene->GetComponent<TransformComponent>(Wheel_fl);
        transform.Position = glm::vec3(5.8f, 0.6f, 3.1f)+ SpawnLocation;
        transform.Scale = glm::vec3(2.8f);
        transform.Rotation = glm::vec3(glm::pi<float>() / 2.f, 0.f, 0.f);
        glm::vec3 scale = glm::vec3(1.4f, 0.6f, 1.4f);
        auto &body = scene->AddComponent<RigidBodyComponent>(Wheel_fl, transform.Position, scale, transform.Rotation, 300.f, bt::CollisionPrimitive::Cylinder);
        auto &RigidBody = body.RigidBody;
        RigidBody->setAnisotropicFriction(btVector3(1.f,0.5f,0.5f));
        RigidBody->setFriction(engine.WheelFriction);
        RigidBody->setRollingFriction(1);
        RigidBody->setSpinningFriction(1);
    }
    {
        Wheel_br = scene->CreateEntity("Wheel Back right");
        auto &mesh = scene->AddComponent<StaticMeshComponent>(Wheel_br, "Assets/Wheels/tuner-wheel/source/tunerWheelRight.fbx");
        mesh.meshes[1].MeshMaterial.Diffuse = Texture("Assets/Wheels/tuner-wheel/textures/TireColor2.png");
        mesh.meshes[1].MeshMaterial.Metallic = Texture(TextureColor::Black);
        mesh.meshes[1].MeshMaterial.Normals = Texture("Assets/Wheels/tuner-wheel/textures/TireNorm1.png");
        mesh.meshes[1].MeshMaterial.Roughness = Texture(TextureColor::White);
        mesh.meshes[1].MeshMaterial.UpdateDescriptorSet();

        mesh.meshes[0].MeshMaterial.Diffuse = Texture("Assets/Wheels/tuner-wheel/textures/WheelBaseColor1.png");
        mesh.meshes[0].MeshMaterial.Metallic = Texture("Assets/Wheels/tuner-wheel/textures/WheelBaseMetallic1.png");
        mesh.meshes[0].MeshMaterial.Roughness = Texture("Assets/Wheels/tuner-wheel/textures/WheelBaseRoughness1.png");
        mesh.meshes[0].MeshMaterial.UpdateDescriptorSet();

        mesh.meshes[2].MeshMaterial.Diffuse = Texture("Assets/Wheels/tuner-wheel/textures/WheelBaseColor1.png");
        mesh.meshes[2].MeshMaterial.Metallic = Texture("Assets/Wheels/tuner-wheel/textures/WheelBaseMetallic1.png");
        mesh.meshes[2].MeshMaterial.Roughness = Texture("Assets/Wheels/tuner-wheel/textures/WheelBaseRoughness1.png");
        mesh.meshes[2].MeshMaterial.UpdateDescriptorSet();

        auto &transform = scene->GetComponent<TransformComponent>(Wheel_br);
        transform.Position = glm::vec3(-4.7f, 0.6f, -3.1f) + SpawnLocation;
        transform.Scale = glm::vec3(2.8f);
        transform.Rotation = glm::vec3(glm::pi<float>() / 2.f, 0.f, 0.f);
        glm::vec3 scale = glm::vec3(1.4f, 0.6f, 1.4f);
        auto &body = scene->AddComponent<RigidBodyComponent>(Wheel_br, transform.Position, scale, transform.Rotation, 300.f, bt::CollisionPrimitive::Cylinder);
        auto &RigidBody = body.RigidBody;
        RigidBody->setAnisotropicFriction(btVector3(1.f,0.5f,0.5f));
        RigidBody->setFriction(engine.WheelFriction);
        RigidBody->setRollingFriction(1);
        RigidBody->setSpinningFriction(1);
    }
    {
        Wheel_bl = scene->CreateEntity("Wheel back left");
        auto &mesh = scene->AddComponent<StaticMeshComponent>(Wheel_bl, "Assets/Wheels/tuner-wheel/source/tunerWheelLeft.fbx");
        mesh.meshes[1].MeshMaterial.Diffuse = Texture("Assets/Wheels/tuner-wheel/textures/TireColor2.png");
        mesh.meshes[1].MeshMaterial.Metallic = Texture(TextureColor::Black);
        mesh.meshes[1].MeshMaterial.Normals = Texture("Assets/Wheels/tuner-wheel/textures/TireNorm1.png");
        mesh.meshes[1].MeshMaterial.Roughness = Texture(TextureColor::White);
        mesh.meshes[1].MeshMaterial.UpdateDescriptorSet();

        mesh.meshes[0].MeshMaterial.Diffuse = Texture("Assets/Wheels/tuner-wheel/textures/WheelBaseColor1.png");
        mesh.meshes[0].MeshMaterial.Metallic = Texture("Assets/Wheels/tuner-wheel/textures/WheelBaseMetallic1.png");
        mesh.meshes[0].MeshMaterial.Roughness = Texture("Assets/Wheels/tuner-wheel/textures/WheelBaseRoughness1.png");
        mesh.meshes[0].MeshMaterial.UpdateDescriptorSet();

        mesh.meshes[2].MeshMaterial.Diffuse = Texture("Assets/Wheels/tuner-wheel/textures/WheelBaseColor1.png");
        mesh.meshes[2].MeshMaterial.Metallic = Texture("Assets/Wheels/tuner-wheel/textures/WheelBaseMetallic1.png");
        mesh.meshes[2].MeshMaterial.Roughness = Texture("Assets/Wheels/tuner-wheel/textures/WheelBaseRoughness1.png");
        mesh.meshes[2].MeshMaterial.UpdateDescriptorSet();

        auto &transform = scene->GetComponent<TransformComponent>(Wheel_bl);
        transform.Position = glm::vec3(-4.7f, 0.6f, 3.1f) + SpawnLocation;
        transform.Scale = glm::vec3(2.8f);
        transform.Rotation = glm::vec3(glm::pi<float>() / 2.f, 0.f, 0.f);
        glm::vec3 scale = glm::vec3(1.4f, 0.6f, 1.4f);
        auto &body = scene->AddComponent<RigidBodyComponent>(Wheel_bl, transform.Position, scale, transform.Rotation, 300.f, bt::CollisionPrimitive::Cylinder);
        auto &RigidBody = body.RigidBody;
        RigidBody->setAnisotropicFriction(btVector3(1.f,0.5f,0.5f));
        RigidBody->setFriction(engine.WheelFriction);
        RigidBody->setRollingFriction(1);
        RigidBody->setSpinningFriction(1);
    }

    //make gears
    {
        engine.Gears.emplace_back(20.f,3.f);
        engine.Gears.emplace_back(30.f,2.f);
        engine.Gears.emplace_back(40.f,1.5f);
        engine.Gears.emplace_back(50.f,1.f);
        engine.Gears.emplace_back(70.f,0.5f);
        engine.Gears.emplace_back(90.f,0.25f);

        engine.maxVelocity = engine.Gears[engine.CurrentGear].first;
    }
    //camera locations
    {
        CamLocations.emplace_back("Third Person",glm::vec3(-2.5f,1.2f,0.f)*8.f,glm::vec3(2.f,0.4f,0.f)*8.f);
        CamLocations.emplace_back("Close Third Person",glm::vec3(-1.5f,0.8f,0.f)*8.f,glm::vec3(1.f,0.1f,0.f)*8.f);
        CamLocations.emplace_back("First Person",glm::vec3(0.0f,0.4f,-0.15f)*8.f,glm::vec3(2.4f,0.4f,0.f)*8.f);
        CamLocations.emplace_back("Cinematic",glm::vec3(-2.f,1.f,-1.f)*8.f,glm::vec3(2.f,0.4f,0.f)*8.f);
        CamLocations.emplace_back("G T FUCKING A ",glm::vec3(0.2f,0.0f,-0.5f)*8.f,glm::vec3(4.2f,0.5f,0.f)*8.f);
    }

    //parent oncreate last since it needs to hinge car togheter
    CarBaseScript::OnCreate(scene, entity);
}

void FLOOF::RaceCarScript::OnUpdate(float deltaTime) {
    CarBaseScript::OnUpdate(deltaTime);
}

void FLOOF::RaceCarScript::LastUpdate(float deltaTime) {
    CarBaseScript::LastUpdate(deltaTime);
}
