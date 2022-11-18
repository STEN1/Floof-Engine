#include "MonsterTruckScript.h"
#include "../Components.h"
#include "../Renderer/Mesh.h"
#include "../Renderer/ModelManager.h"
#include "../Input.h"
#include "../Application.h"


const float RollingFriction{0.5f};
const float SpinningFriction{0.3f};
const float WheelFriction{0.9f};

void FLOOF::MonsterTruckScript::OnCreate(std::shared_ptr<Scene> scene, entt::entity entity) {
    NativeScript::OnCreate(scene, entity);

    {
        frame = entity;
        auto &mesh = scene->AddComponent<StaticMeshComponent>(frame, "Assets/MonsterTruck/MonstertruckFrameRotated.fbx");
        for (auto &mesh: mesh.meshes) {
            mesh.MeshMaterial.Diffuse = Texture("Assets/MonsterTruck/texturesFrame/HarmaaRunko_albedo.jpg");
            mesh.MeshMaterial.Metallic = Texture("Assets/MonsterTruck/texturesFrame/HarmaaRunko_metallic.jpg");
            mesh.MeshMaterial.AO = Texture("Assets/MonsterTruck/texturesFrame/HarmaaRunko_AO.jpg");
            mesh.MeshMaterial.Normals = Texture("Assets/MonsterTruck/texturesFrame/HarmaaRunko_normal.jpg");
            mesh.MeshMaterial.Roughness = Texture("Assets/MonsterTruck/texturesFrame/HarmaaRunko_roughness.jpg");
            mesh.MeshMaterial.UpdateDescriptorSet();
        }

        auto &engine = scene->AddComponent<EngineComponent>(frame);

        auto &transform = scene->GetComponent<TransformComponent>(frame);
        transform.Scale = glm::vec3(1.f);
        //transform.Rotation = glm::vec3(1.5f, 0.f, 0.f);

        auto &body = scene->AddComponent<RigidBodyComponent>(frame, transform.Position, transform.Scale, transform.Rotation, 2000.f, "Assets/MonsterTruck/MonstertruckFrameRotated.fbx");

        auto& sound = scene->AddComponent<SoundSourceComponent>(frame,"Vehicles_idle.wav");
        sound.Looping(true);
        sound.Play();

    }
    //frame
    {
        auto ent = scene->CreateEntity("CyberTruck", frame);
        auto &mesh = scene->AddComponent<StaticMeshComponent>(ent, "Assets/cyber-truck/source/CybertruckNowheel.fbx");
        for (auto &mesh: mesh.meshes) {
            mesh.MeshMaterial.Diffuse = Texture("Assets/cyber-truck/textures/cyber_Albedo.tga.png");
            mesh.MeshMaterial.Metallic = Texture("Assets/cyber-truck/textures/cyber_Metallic.tga.png");
            mesh.MeshMaterial.AO = Texture("Assets/cyber-truck/textures/cyber_Occlusion.tga.png");
            mesh.MeshMaterial.Normals = Texture("Assets/cyber-truck/textures/cyber_Normal.tga.png");
            mesh.MeshMaterial.UpdateDescriptorSet();
        }

        auto &transform = scene->GetComponent<TransformComponent>(ent);
        transform.Scale = glm::vec3(8.2f, 8.f, 9.5f);
        transform.Position = glm::vec3(1.f, 3.f, 0.f);
        transform.Rotation = glm::vec3(0.f, 3.14f, 0.f);
    }
    {
        Wheel_fr = scene->CreateEntity("Wheel Front Right");
        auto &mesh = scene->AddComponent<StaticMeshComponent>(Wheel_fr, "Assets/MonsterTruck/LPWheelFixed.fbx");
        for (auto &mesh: mesh.meshes) {
            mesh.MeshMaterial.Diffuse = Texture("Assets/MonsterTruck/texturesWheel/lambert1_Base_Color.png");
            mesh.MeshMaterial.Metallic = Texture("Assets/MonsterTruck/texturesWheel/lambert1_Metallic.png");
            mesh.MeshMaterial.AO = Texture("Assets/MonsterTruck/texturesWheel/lambert1_Mixed_AO.png");
            mesh.MeshMaterial.Normals = Texture("Assets/MonsterTruck/texturesWheel/lambert1_Normal_OpenGL.png", false, true);
            mesh.MeshMaterial.Roughness = Texture("Assets/MonsterTruck/texturesWheel/lambert1_Roughness.png");
            mesh.MeshMaterial.UpdateDescriptorSet();
        }

        auto &transform = scene->GetComponent<TransformComponent>(Wheel_fr);
        transform.Position = glm::vec3(35.f, -10.f, -20.f) / 5.f;
        transform.Scale = glm::vec3(1.5f);
        transform.Rotation = glm::vec3(0.f, 1.5f, 0.f);

        auto &frameTrans = scene->GetComponent<TransformComponent>(frame);
        //scene->AddComponent<RigidBodyComponent>(Wheel_fr, transform.Position-frameTrans.Position, transform.Scale+frameTrans.Scale,500.f,"Assets/MonsterTruck/LPWheelFixed.fbx");
        auto &body = scene->AddComponent<RigidBodyComponent>(Wheel_fr, transform.Position, transform.Scale, transform.Rotation, 50.f, "Assets/MonsterTruck/LPWheelFixed.fbx");
        auto &RigidBody = body.RigidBody;
        RigidBody->setFriction(WheelFriction);
        RigidBody->setRollingFriction(RollingFriction);
        RigidBody->setSpinningFriction(SpinningFriction);
    }
    {
        Wheel_fl = scene->CreateEntity("Wheel Front Left");
        auto &mesh = scene->AddComponent<StaticMeshComponent>(Wheel_fl, "Assets/MonsterTruck/LPWheelFixed.fbx");
        for (auto &mesh: mesh.meshes) {
            mesh.MeshMaterial.Diffuse = Texture("Assets/MonsterTruck/texturesWheel/lambert1_Base_Color.png");
            mesh.MeshMaterial.Metallic = Texture("Assets/MonsterTruck/texturesWheel/lambert1_Metallic.png");
            mesh.MeshMaterial.AO = Texture("Assets/MonsterTruck/texturesWheel/lambert1_Mixed_AO.png");
            mesh.MeshMaterial.Normals = Texture("Assets/MonsterTruck/texturesWheel/lambert1_Normal_OpenGL.png", false, true);
            mesh.MeshMaterial.Roughness = Texture("Assets/MonsterTruck/texturesWheel/lambert1_Roughness.png");
            mesh.MeshMaterial.UpdateDescriptorSet();
        }

        auto &transform = scene->GetComponent<TransformComponent>(Wheel_fl);
        transform.Position = glm::vec3(35.f, -10.f, 20.f) / 5.f;
        transform.Scale = glm::vec3(1.5f);
        transform.Rotation = glm::vec3(0.f, -1.5f, 0.f);

        auto &frameTrans = scene->GetComponent<TransformComponent>(frame);
        //scene->AddComponent<RigidBodyComponent>(Wheel_fl, transform.Position-frameTrans.Position, transform.Scale+frameTrans.Scale,500.f,"Assets/MonsterTruck/LPWheelFixed.fbx");
        auto &body = scene->AddComponent<RigidBodyComponent>(Wheel_fl, transform.Position, transform.Scale, transform.Rotation, 50.f, "Assets/MonsterTruck/LPWheelFixed.fbx");
        auto &RigidBody = body.RigidBody;
        RigidBody->setFriction(WheelFriction);
        RigidBody->setRollingFriction(RollingFriction);
        RigidBody->setSpinningFriction(SpinningFriction);
    }
    {
        Wheel_br = scene->CreateEntity("Wheel Back right");
        auto &mesh = scene->AddComponent<StaticMeshComponent>(Wheel_br, "Assets/MonsterTruck/LPWheelFixed.fbx");
        for (auto &mesh: mesh.meshes) {
            mesh.MeshMaterial.Diffuse = Texture("Assets/MonsterTruck/texturesWheel/lambert1_Base_Color.png");
            mesh.MeshMaterial.Metallic = Texture("Assets/MonsterTruck/texturesWheel/lambert1_Metallic.png");
            mesh.MeshMaterial.AO = Texture("Assets/MonsterTruck/texturesWheel/lambert1_Mixed_AO.png");
            mesh.MeshMaterial.Normals = Texture("Assets/MonsterTruck/texturesWheel/lambert1_Normal_OpenGL.png", false, true);
            mesh.MeshMaterial.Roughness = Texture("Assets/MonsterTruck/texturesWheel/lambert1_Roughness.png");
            mesh.MeshMaterial.UpdateDescriptorSet();
        }

        auto &transform = scene->GetComponent<TransformComponent>(Wheel_br);
        transform.Position = glm::vec3(-35.f, -10.f, -20.f) / 5.f;
        transform.Scale = glm::vec3(1.5f);
        transform.Rotation = glm::vec3(0.f, 1.5f, 0.f);

        auto &frameTrans = scene->GetComponent<TransformComponent>(frame);
        //scene->AddComponent<RigidBodyComponent>(Wheel_br, transform.Position-frameTrans.Position, transform.Scale+frameTrans.Scale,500.f,"Assets/MonsterTruck/LPWheelFixed.fbx");
        auto &body = scene->AddComponent<RigidBodyComponent>(Wheel_br, transform.Position, transform.Scale, transform.Rotation, 50.f, "Assets/MonsterTruck/LPWheelFixed.fbx");
        body.RigidBody->setFriction(1000.f);
    }
    {
        Wheel_bl = scene->CreateEntity("Wheel back left");
        auto &mesh = scene->AddComponent<StaticMeshComponent>(Wheel_bl, "Assets/MonsterTruck/LPWheelFixed.fbx");
        for (auto &mesh: mesh.meshes) {
            mesh.MeshMaterial.Diffuse = Texture("Assets/MonsterTruck/texturesWheel/lambert1_Base_Color.png");
            mesh.MeshMaterial.Metallic = Texture("Assets/MonsterTruck/texturesWheel/lambert1_Metallic.png");
            mesh.MeshMaterial.AO = Texture("Assets/MonsterTruck/texturesWheel/lambert1_Mixed_AO.png");
            mesh.MeshMaterial.Normals = Texture("Assets/MonsterTruck/texturesWheel/lambert1_Normal_OpenGL.png", false, true);
            mesh.MeshMaterial.Roughness = Texture("Assets/MonsterTruck/texturesWheel/lambert1_Roughness.png");
            mesh.MeshMaterial.UpdateDescriptorSet();
        }

        auto &transform = scene->GetComponent<TransformComponent>(Wheel_bl);
        transform.Position = glm::vec3(-35.f, -10.f, 20.f) / 5.f;
        transform.Scale = glm::vec3(1.5f);
        transform.Rotation = glm::vec3(0.f, -1.5f, 0.f);

        auto &frameTrans = scene->GetComponent<TransformComponent>(frame);
        //scene->AddComponent<RigidBodyComponent>(Wheel_bl, transform.Position-frameTrans.Position, transform.Scale+frameTrans.Scale,500.f,"Assets/MonsterTruck/LPWheelFixed.fbx");
        auto &body = scene->AddComponent<RigidBodyComponent>(Wheel_bl, transform.Position, transform.Scale, transform.Rotation, 50.f, "Assets/MonsterTruck/LPWheelFixed.fbx");
        auto &RigidBody = body.RigidBody;
        RigidBody->setFriction(WheelFriction);
        RigidBody->setRollingFriction(RollingFriction);
        RigidBody->setSpinningFriction(SpinningFriction);
    }
    //hinge car togheter
    {

        auto &frameBody = scene->GetComponent<RigidBodyComponent>(frame);
        auto &WheelfrBody = scene->GetComponent<RigidBodyComponent>(Wheel_fr);
        auto &WheelflBody = scene->GetComponent<RigidBodyComponent>(Wheel_fl);
        auto &WheelbrBody = scene->GetComponent<RigidBodyComponent>(Wheel_br);
        auto &WheelblBody = scene->GetComponent<RigidBodyComponent>(Wheel_bl);

        auto &engine = scene->GetComponent<EngineComponent>(frame);

        frameBody.RigidBody->setActivationState(DISABLE_DEACTIVATION);
        WheelfrBody.RigidBody->setActivationState(DISABLE_DEACTIVATION);
        WheelflBody.RigidBody->setActivationState(DISABLE_DEACTIVATION);
        WheelbrBody.RigidBody->setActivationState(DISABLE_DEACTIVATION);
        WheelblBody.RigidBody->setActivationState(DISABLE_DEACTIVATION);


        btVector3 parentAxis(0.f, 1.f, 0.f);
        btVector3 childAxis(0.f, 0.f, 1.f);


        btVector3 anchor = WheelfrBody.RigidBody->getWorldTransform().getOrigin();
        btHinge2Constraint *Hinge1 = new btHinge2Constraint(*frameBody.RigidBody.get(), *WheelfrBody.RigidBody.get(), anchor, parentAxis, childAxis);
        scene->GetPhysicSystem()->AddConstraint(Hinge1, true);

        anchor = WheelflBody.RigidBody->getWorldTransform().getOrigin();
        btHinge2Constraint *Hinge2 = new btHinge2Constraint(*frameBody.RigidBody.get(), *WheelflBody.RigidBody.get(), anchor, parentAxis, childAxis);
        scene->GetPhysicSystem()->AddConstraint(Hinge2, true);

        anchor = WheelbrBody.RigidBody->getWorldTransform().getOrigin();
        btHinge2Constraint *Hinge3 = new btHinge2Constraint(*frameBody.RigidBody.get(), *WheelbrBody.RigidBody.get(), anchor, parentAxis, childAxis);
        scene->GetPhysicSystem()->AddConstraint(Hinge3, true);

        anchor = WheelblBody.RigidBody->getWorldTransform().getOrigin();
        btHinge2Constraint *Hinge4 = new btHinge2Constraint(*frameBody.RigidBody.get(), *WheelblBody.RigidBody.get(), anchor, parentAxis, childAxis);
        scene->GetPhysicSystem()->AddConstraint(Hinge4, true);


        axles.emplace_back(Hinge1);
        axles.emplace_back(Hinge2);
        axles.emplace_back(Hinge3);
        axles.emplace_back(Hinge4);

        engine.axles = axles;

        for (auto &hinge: axles) {

            // Drive engine.
            hinge->enableMotor(3, true);
            hinge->setMaxMotorForce(3, engine.maxEngineForce);
            hinge->setTargetVelocity(3, 0);

            // Steering engine.
            hinge->enableMotor(5, true);
            hinge->setMaxMotorForce(5, engine.maxTurnForce);
            hinge->setTargetVelocity(5, 0);

            hinge->setParam(BT_CONSTRAINT_CFM, 0.15f, 1);
            hinge->setParam(BT_CONSTRAINT_ERP, 0.35f, 1);

            hinge->setLimit(2, 0.f, 1.f);

            hinge->setDamping(2, engine.suspensionDamping);
            hinge->setStiffness(2, engine.suspensionStiffness);

            hinge->setLowerLimit(-SIMD_HALF_PI * 0.4f);
            hinge->setUpperLimit(SIMD_HALF_PI * 0.4f);

            //back wheels
            if (hinge == Hinge3 || hinge == Hinge4) {
                hinge->setLowerLimit(-SIMD_HALF_PI * 0.01f);
                hinge->setUpperLimit(SIMD_HALF_PI * 0.01f);
            }

            hinge->setDbgDrawSize(btScalar(2.f));
        }
    }
}

void FLOOF::MonsterTruckScript::OnUpdate(float deltaTime) {
    NativeScript::OnUpdate(deltaTime);

    auto scene = Application::Get().m_Scene;
    auto &engine = scene->GetComponent<EngineComponent>(frame);

    if (Input::Key(ImGuiKey_RightArrow)) {
        engine.TurnForce = -engine.maxTurnForce;
        engine.VehicleSteering += engine.steeringIncrement;
        if (engine.VehicleSteering > engine.steeringClamp) {
            engine.VehicleSteering = engine.steeringClamp;
        }
    }
    if (Input::Key(ImGuiKey_LeftArrow)) {
        engine.TurnForce = engine.maxTurnForce;
        engine.VehicleSteering -= engine.steeringIncrement;
        if (engine.VehicleSteering < -engine.steeringClamp) {
            engine.VehicleSteering = -engine.steeringClamp;
        }
    }
    if (Input::Key(ImGuiKey_UpArrow)) {
        engine.EngineForce = engine.maxVelocity;
        engine.BreakingForce = 0.f;
    }
    if (Input::Key(ImGuiKey_DownArrow)) {
        engine.EngineForce = -engine.maxVelocity;
        engine.BreakingForce = 0.f;
    }

    auto wfr = scene->GetComponent<RigidBodyComponent>(Wheel_fr).RigidBody;
    auto wfl = scene->GetComponent<RigidBodyComponent>(Wheel_fl).RigidBody;
    auto wbr = scene->GetComponent<RigidBodyComponent>(Wheel_br).RigidBody;
    auto wbl = scene->GetComponent<RigidBodyComponent>(Wheel_bl).RigidBody;

    std::vector<std::shared_ptr<btRigidBody>> wheels;
    wheels.emplace_back(wfr);
    wheels.emplace_back(wfl);
    wheels.emplace_back(wbr);
    wheels.emplace_back(wbl);


    for (auto &axle: axles) {
        axle->setTargetVelocity(3, engine.EngineForce);
    }
    //turning
    {
        axles[0]->setTargetVelocity(5, engine.TurnForce); // front right
        axles[1]->setTargetVelocity(5, engine.TurnForce); // front left

    }

    //makes you need to hold button for power
    engine.EngineForce = 0.f;
    engine.TurnForce = 0.f;
}
