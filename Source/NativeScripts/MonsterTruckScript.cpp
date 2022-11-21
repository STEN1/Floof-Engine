#include "MonsterTruckScript.h"
#include "../Components.h"
#include "../Renderer/Mesh.h"
#include "../Renderer/ModelManager.h"
#include "../Input.h"
#include "../Application.h"

void FLOOF::MonsterTruckScript::OnCreate(std::shared_ptr<Scene> scene, entt::entity entity) {
    NativeScript::OnCreate(scene, entity);

    {
        frame = entity;
        auto &mesh = scene->AddComponent<StaticMeshComponent>(frame, "Assets/Wheels/tesla-cybertruck-technic-animation-studios/source/Cybertruck_Frame.fbx", false);
        //textures
        {
            const std::string path = "Assets/Wheels/tesla-cybertruck-technic-animation-studios/textures/";
            const std::string diffuse = "_baseColor.png";
            const std::string Metallic = "_metallicRoughness_png@channels=B.png";
            const std::string Roughness = "_metallicRoughness_png@channels=G.png";
            const std::string Normal = "_normal.png";

            const char *textureNames[] = {
                    "char",
                    "dashbord",
                    "farman",
                    "glass",
                    "glass_black",
                    "inside_body",
                    "lastik",
                    "miror",
                    "ring_plus",
                    "separ"
            };
            enum CyberTruckTexture {
                chair = 0,
                dashboard = 1,
                farman = 2,
                glass = 3,
                glassblack = 4,
                inside = 5,
                lastik = 6,
                mirror = 7,
                ring = 8,
                separ = 9,
                metal = 10,
                white = 11,
                red = 12,
            };
            std::vector<CyberTruckTexture> textureorder{
                    CyberTruckTexture::chair,
                    CyberTruckTexture::farman,
                    CyberTruckTexture::glass,
                    CyberTruckTexture::glass,
                    CyberTruckTexture::white,
                    CyberTruckTexture::separ, //separ
                    CyberTruckTexture::metal, // chassi
                    CyberTruckTexture::glass,
                    CyberTruckTexture::glassblack,
                    CyberTruckTexture::red,
                    CyberTruckTexture::dashboard,
                    CyberTruckTexture::inside,
                    CyberTruckTexture::mirror,
                    CyberTruckTexture::metal, // screen
                    CyberTruckTexture::separ
            };
            for (int i{0}; i < mesh.meshes.size(); i++) {
                std::string norm = path;
                std::string rough = path;
                std::string met = path;
                std::string diff = path;

                switch (textureorder[i]) {
                    case CyberTruckTexture::metal:
                        mesh.meshes[i].MeshMaterial.Diffuse = Texture(TextureColor::LightGrey);
                        mesh.meshes[i].MeshMaterial.Metallic = Texture(TextureColor::White);
                        mesh.meshes[i].MeshMaterial.Roughness = Texture(TextureColor::DarkGrey);
                        break;
                    case CyberTruckTexture::red:
                        mesh.meshes[i].MeshMaterial.Diffuse = Texture(TextureColor::Red);
                        break;
                    case CyberTruckTexture::white:
                        mesh.meshes[i].MeshMaterial.Diffuse = Texture(TextureColor::White);
                        break;
                    case CyberTruckTexture::glassblack:
                        mesh.meshes[i].MeshMaterial.Diffuse = Texture(TextureColor::LightGrey);
                        mesh.meshes[i].MeshMaterial.Metallic = Texture(TextureColor::White);
                        mesh.meshes[i].MeshMaterial.Roughness = Texture(TextureColor::DarkGrey);

                        norm += textureNames[textureorder[i]];
                        norm += Normal;
                        mesh.meshes[i].MeshMaterial.Normals = Texture(norm);
                        break;
                    default :

                        diff += textureNames[textureorder[i]];
                        diff += diffuse;

                        met += textureNames[textureorder[i]];
                        met += Metallic;

                        rough += textureNames[textureorder[i]];
                        rough += Roughness;
                        //std::string norm = path;
                        norm += textureNames[textureorder[i]];
                        norm += Normal;

                        mesh.meshes[i].MeshMaterial.Diffuse = Texture(diff);
                        mesh.meshes[i].MeshMaterial.Metallic = Texture(met);
                        mesh.meshes[i].MeshMaterial.Roughness = Texture(rough);
                        mesh.meshes[i].MeshMaterial.Normals = Texture(norm);
                        break;
                }
                mesh.meshes[i].MeshMaterial.UpdateDescriptorSet();
            }
        }
        auto &engine = scene->AddComponent<EngineComponent>(frame);

        auto &transform = scene->GetComponent<TransformComponent>(frame);
        transform.Scale = glm::vec3(8.f);

        auto &body = scene->AddComponent<RigidBodyComponent>(frame, transform.Position, transform.Scale, transform.Rotation, 3000.f, "Assets/Wheels/tesla-cybertruck-technic-animation-studios/source/Cybertruck_Frame.fbx");
        body.RigidBody->setCollisionFlags(body.RigidBody->getCollisionFlags() | btCollisionObject::CF_NO_CONTACT_RESPONSE);
        auto &sound = scene->AddComponent<SoundSourceComponent>(frame, "Vehicles_idle2.wav");
        sound.Looping(true);
        //sound.Play();

        //ledbar
        {
            Ledbar = scene->CreateEntity("Ledbar", frame);
            auto &light = scene->AddComponent<PointLightComponent>(Ledbar);
            //auto & mesh = scene->AddComponent<StaticMeshComponent>(Ledbar,"Assets/LowPolySphere.fbx");
            //mesh.meshes[0].MeshMaterial.Diffuse = Texture(TextureColor::White);
            //mesh.meshes[0].MeshMaterial.UpdateDescriptorSet();

            light.lightRange = 10.f;

            auto &transform = scene->GetComponent<TransformComponent>(Ledbar);
            transform.Position = glm::vec3(0.25f, 0.7f, 0.f);
            transform.Scale = glm::vec3(0.1f);
        }
        //Breaklight
        {
            BreakLight = scene->CreateEntity("BreakLight", frame);
            auto &light = scene->AddComponent<PointLightComponent>(BreakLight);
            //auto & mesh = scene->AddComponent<StaticMeshComponent>(BreakLight,"Assets/LowPolySphere.fbx");
            //mesh.meshes[0].MeshMaterial.Diffuse = Texture(TextureColor::Red);
            //mesh.meshes[0].MeshMaterial.UpdateDescriptorSet();

            light.lightRange = 20.f;
            light.diffuse = glm::vec4(1.f, 0.f, 0.f, 1.f);

            auto &transform = scene->GetComponent<TransformComponent>(BreakLight);
            transform.Position = glm::vec3(-1.4f, 0.4f, 0.f);
            transform.Scale = glm::vec3(0.1f);
        }
        //Headlight Right
        {
            HeadLightR = scene->CreateEntity("Right Headlight", frame);
            auto &light = scene->AddComponent<PointLightComponent>(HeadLightR);
            //auto & mesh = scene->AddComponent<StaticMeshComponent>(HeadLightR,"Assets/LowPolySphere.fbx");
            //mesh.meshes[0].MeshMaterial.Diffuse = Texture(TextureColor::White);
            //mesh.meshes[0].MeshMaterial.UpdateDescriptorSet();

            light.lightRange = 10.f;

            auto &transform = scene->GetComponent<TransformComponent>(HeadLightR);
            transform.Position = glm::vec3(1.3f, 0.35f, -0.4f);
            transform.Scale = glm::vec3(0.1f);
        }
        //Headlight Left
        {
            HeadLightL = scene->CreateEntity("Left Headlight", frame);
            auto &light = scene->AddComponent<PointLightComponent>(HeadLightL);
            //auto & mesh = scene->AddComponent<StaticMeshComponent>(HeadLightL,"Assets/LowPolySphere.fbx");
            //mesh.meshes[0].MeshMaterial.Diffuse = Texture(TextureColor::White);
            //mesh.meshes[0].MeshMaterial.UpdateDescriptorSet();

            light.lightRange = 10.f;

            auto &transform = scene->GetComponent<TransformComponent>(HeadLightL);
            transform.Position = glm::vec3(1.3f, 0.35f, 0.4f);
            transform.Scale = glm::vec3(0.1f);
        }

    }
    {
        Wheel_fr = scene->CreateEntity("Wheel Front Right");
        auto &mesh = scene->AddComponent<StaticMeshComponent>(Wheel_fr, "Assets/MonsterTruck/LPWheelFixed_right.fbx");
        for (auto &mesh: mesh.meshes) {
            mesh.MeshMaterial.Diffuse = Texture("Assets/MonsterTruck/texturesWheel/lambert1_Base_Color.png");
            mesh.MeshMaterial.Metallic = Texture("Assets/MonsterTruck/texturesWheel/lambert1_Metallic.png");
            mesh.MeshMaterial.AO = Texture("Assets/MonsterTruck/texturesWheel/lambert1_Mixed_AO.png");
            mesh.MeshMaterial.Normals = Texture("Assets/MonsterTruck/texturesWheel/lambert1_Normal_OpenGL.png", false, true);
            mesh.MeshMaterial.Roughness = Texture("Assets/MonsterTruck/texturesWheel/lambert1_Roughness.png");
            mesh.MeshMaterial.UpdateDescriptorSet();
        }

        auto &transform = scene->GetComponent<TransformComponent>(Wheel_fr);
        transform.Position = glm::vec3(5.5f, -0.5f, -2.5f);
        transform.Scale = glm::vec3(2.5f);
        transform.Rotation = glm::vec3(glm::pi<float>() / 2.f, 0.f, 0.f);
        glm::vec3 scale = glm::vec3(1.3f, 0.7f, 1.3f);
        auto &body = scene->AddComponent<RigidBodyComponent>(Wheel_fr, transform.Position, scale, transform.Rotation, 300.f, bt::CollisionPrimitive::Cylinder);
        auto &RigidBody = body.RigidBody;
        auto &engine = scene->GetComponent<EngineComponent>(frame);
        RigidBody->setFriction(engine.WheelFriction);
        RigidBody->setRollingFriction(engine.RollingFriction);
        RigidBody->setSpinningFriction(engine.SpinningFriction);
    }
    {
        Wheel_fl = scene->CreateEntity("Wheel Front Left");
        auto &mesh = scene->AddComponent<StaticMeshComponent>(Wheel_fl, "Assets/MonsterTruck/LPWheelFixed_Left.fbx");
        for (auto &mesh: mesh.meshes) {
            mesh.MeshMaterial.Diffuse = Texture("Assets/MonsterTruck/texturesWheel/lambert1_Base_Color.png");
            mesh.MeshMaterial.Metallic = Texture("Assets/MonsterTruck/texturesWheel/lambert1_Metallic.png");
            mesh.MeshMaterial.AO = Texture("Assets/MonsterTruck/texturesWheel/lambert1_Mixed_AO.png");
            mesh.MeshMaterial.Normals = Texture("Assets/MonsterTruck/texturesWheel/lambert1_Normal_OpenGL.png", false, true);
            mesh.MeshMaterial.Roughness = Texture("Assets/MonsterTruck/texturesWheel/lambert1_Roughness.png");
            mesh.MeshMaterial.UpdateDescriptorSet();
        }

        auto &transform = scene->GetComponent<TransformComponent>(Wheel_fl);
        transform.Position = glm::vec3(5.5f, -0.5f, 2.5f);
        transform.Scale = glm::vec3(2.5f);
        transform.Rotation = glm::vec3(glm::pi<float>() / 2.f, 0.f, 0.f);
        glm::vec3 scale = glm::vec3(1.3f, 0.7f, 1.3f);
        auto &body = scene->AddComponent<RigidBodyComponent>(Wheel_fl, transform.Position, scale, transform.Rotation, 300.f, bt::CollisionPrimitive::Cylinder);
        auto &RigidBody = body.RigidBody;
        auto &engine = scene->GetComponent<EngineComponent>(frame);
        RigidBody->setFriction(engine.WheelFriction);
        RigidBody->setRollingFriction(engine.RollingFriction);
        RigidBody->setSpinningFriction(engine.SpinningFriction);
    }
    {
        Wheel_br = scene->CreateEntity("Wheel Back right");
        auto &mesh = scene->AddComponent<StaticMeshComponent>(Wheel_br, "Assets/MonsterTruck/LPWheelFixed_right.fbx");
        for (auto &mesh: mesh.meshes) {
            mesh.MeshMaterial.Diffuse = Texture("Assets/MonsterTruck/texturesWheel/lambert1_Base_Color.png");
            mesh.MeshMaterial.Metallic = Texture("Assets/MonsterTruck/texturesWheel/lambert1_Metallic.png");
            mesh.MeshMaterial.AO = Texture("Assets/MonsterTruck/texturesWheel/lambert1_Mixed_AO.png");
            mesh.MeshMaterial.Normals = Texture("Assets/MonsterTruck/texturesWheel/lambert1_Normal_OpenGL.png", false, true);
            mesh.MeshMaterial.Roughness = Texture("Assets/MonsterTruck/texturesWheel/lambert1_Roughness.png");
            mesh.MeshMaterial.UpdateDescriptorSet();
        }

        auto &transform = scene->GetComponent<TransformComponent>(Wheel_br);
        transform.Position = glm::vec3(-4.5f, -0.5f, -2.5f);
        transform.Scale = glm::vec3(2.5f);
        transform.Rotation = glm::vec3(glm::pi<float>() / 2.f, 0.f, 0.f);
        glm::vec3 scale = glm::vec3(1.3f, 0.7f, 1.3f);
        auto &body = scene->AddComponent<RigidBodyComponent>(Wheel_br, transform.Position, scale, transform.Rotation, 300.f, bt::CollisionPrimitive::Cylinder);
        auto &RigidBody = body.RigidBody;
        auto &engine = scene->GetComponent<EngineComponent>(frame);
        RigidBody->setFriction(engine.WheelFriction);
        RigidBody->setRollingFriction(engine.RollingFriction);
        RigidBody->setSpinningFriction(engine.SpinningFriction);
    }
    {
        Wheel_bl = scene->CreateEntity("Wheel back left");
        auto &mesh = scene->AddComponent<StaticMeshComponent>(Wheel_bl, "Assets/MonsterTruck/LPWheelFixed_Left.fbx");
        for (auto &mesh: mesh.meshes) {
            mesh.MeshMaterial.Diffuse = Texture("Assets/MonsterTruck/texturesWheel/lambert1_Base_Color.png");
            mesh.MeshMaterial.Metallic = Texture("Assets/MonsterTruck/texturesWheel/lambert1_Metallic.png");
            mesh.MeshMaterial.AO = Texture("Assets/MonsterTruck/texturesWheel/lambert1_Mixed_AO.png");
            mesh.MeshMaterial.Normals = Texture("Assets/MonsterTruck/texturesWheel/lambert1_Normal_OpenGL.png", false, true);
            mesh.MeshMaterial.Roughness = Texture("Assets/MonsterTruck/texturesWheel/lambert1_Roughness.png");
            mesh.MeshMaterial.UpdateDescriptorSet();
        }

        auto &transform = scene->GetComponent<TransformComponent>(Wheel_bl);
        transform.Position = glm::vec3(-4.5f, -0.5f, 2.5f);
        transform.Scale = glm::vec3(2.5f);
        transform.Rotation = glm::vec3(glm::pi<float>() / 2.f, 0.f, 0.f);
        glm::vec3 scale = glm::vec3(1.3f, 0.7f, 1.3f);
        auto &body = scene->AddComponent<RigidBodyComponent>(Wheel_bl, transform.Position, scale, transform.Rotation, 300.f, bt::CollisionPrimitive::Cylinder);
        auto &RigidBody = body.RigidBody;
        auto &engine = scene->GetComponent<EngineComponent>(frame);
        RigidBody->setFriction(engine.WheelFriction);
        RigidBody->setRollingFriction(engine.RollingFriction);
        RigidBody->setSpinningFriction(engine.SpinningFriction);
    }
    //back dif
    {
        BackDif = scene->CreateEntity("BackDif");
        auto &mesh = scene->AddComponent<StaticMeshComponent>(BackDif, "Assets/LowPolyCylinder.fbx");


        auto &transform = scene->GetComponent<TransformComponent>(BackDif);
        transform.Position = glm::vec3(-0.830f, 0.f, 0.f);
        transform.Scale = glm::vec3(0.1f, 2.5f, 0.1f);
        transform.Rotation = glm::vec3(glm::pi<float>() / 2.f, 0.f, 0.f);


        auto &body = scene->AddComponent<RigidBodyComponent>(BackDif, transform.Position, transform.Scale, transform.Rotation, 100.f, bt::CollisionPrimitive::Cylinder);
        auto &RigidBody = body.RigidBody;
        RigidBody->setCollisionFlags(RigidBody->getCollisionFlags() | btCollisionObject::CF_NO_CONTACT_RESPONSE);
    }
    //hinge car togheter
    {

        auto &frameBody = scene->GetComponent<RigidBodyComponent>(frame);
        auto &WheelfrBody = scene->GetComponent<RigidBodyComponent>(Wheel_fr);
        auto &WheelflBody = scene->GetComponent<RigidBodyComponent>(Wheel_fl);
        auto &WheelbrBody = scene->GetComponent<RigidBodyComponent>(Wheel_br);
        auto &WheelblBody = scene->GetComponent<RigidBodyComponent>(Wheel_bl);
        auto &BackDifBody = scene->GetComponent<RigidBodyComponent>(BackDif);

        auto &engine = scene->GetComponent<EngineComponent>(frame);

        frameBody.RigidBody->setActivationState(DISABLE_DEACTIVATION);
        WheelfrBody.RigidBody->setActivationState(DISABLE_DEACTIVATION);
        WheelflBody.RigidBody->setActivationState(DISABLE_DEACTIVATION);
        WheelbrBody.RigidBody->setActivationState(DISABLE_DEACTIVATION);
        WheelblBody.RigidBody->setActivationState(DISABLE_DEACTIVATION);
        BackDifBody.RigidBody->setActivationState(DISABLE_DEACTIVATION);


        btVector3 parentAxis(0.f, 1.f, 0.f);
        btVector3 childAxis(0.f, 0.f, 1.f);


        btVector3 anchor = WheelfrBody.RigidBody->getWorldTransform().getOrigin();
        btHinge2Constraint *Hinge1 = new btHinge2Constraint(*frameBody.RigidBody.get(), *WheelfrBody.RigidBody.get(), anchor, parentAxis, childAxis);
        scene->GetPhysicSystem()->AddConstraint(Hinge1, true);

        anchor = WheelflBody.RigidBody->getWorldTransform().getOrigin();
        btHinge2Constraint *Hinge2 = new btHinge2Constraint(*frameBody.RigidBody.get(), *WheelflBody.RigidBody.get(), anchor, parentAxis, childAxis);
        scene->GetPhysicSystem()->AddConstraint(Hinge2, true);

        //anchor = WheelbrBody.RigidBody->getWorldTransform().getOrigin();
        //btHinge2Constraint *Hinge3 = new btHinge2Constraint(*frameBody.RigidBody.get(), *WheelbrBody.RigidBody.get(), anchor, parentAxis, childAxis);
        //scene->GetPhysicSystem()->AddConstraint(Hinge3, true);

        //anchor = WheelblBody.RigidBody->getWorldTransform().getOrigin();
        //btHinge2Constraint *Hinge4 = new btHinge2Constraint(*frameBody.RigidBody.get(), *WheelblBody.RigidBody.get(), anchor, parentAxis, childAxis);
        //scene->GetPhysicSystem()->AddConstraint(Hinge4, true);

        // anchor = WheelblBody.RigidBody->getWorldTransform().getOrigin();
        //btHinge2Constraint *Hinge5 = new btHinge2Constraint(*BackDifBody.RigidBody.get(), *WheelblBody.RigidBody.get(), anchor, parentAxis, childAxis);
        //scene->GetPhysicSystem()->AddConstraint(Hinge5, true);

        btHingeConstraint *Hinge6 = new btHingeConstraint(*BackDifBody.RigidBody.get(), *WheelblBody.RigidBody.get(), btVector3(0.f, 2.5f, 0.f), btVector3(0.f, 0.f, 0.f), parentAxis, parentAxis);
        scene->GetPhysicSystem()->AddConstraint(Hinge6, true);
        Hinge6->setDbgDrawSize(btScalar(1.5f));
        btHingeConstraint *Hinge7 = new btHingeConstraint(*BackDifBody.RigidBody.get(), *WheelbrBody.RigidBody.get(), btVector3(0.f, -2.5f, 0.f), btVector3(0.f, 0.f, 0.f), parentAxis, parentAxis);
        scene->GetPhysicSystem()->AddConstraint(Hinge7, true);
        Hinge7->setDbgDrawSize(btScalar(1.5f));

        //frame to back axle
        //btHingeConstraint* Hinge8 = new btHingeConstraint(*BackDifBody.RigidBody.get(), *frameBody.RigidBody.get(),btVector3(0.f,0.f,0.f),btVector3(-4.8f, -0.5f, 0.f),parentAxis,childAxis);
        btHingeConstraint *Hinge8 = new btHingeConstraint(*frameBody.RigidBody.get(), *BackDifBody.RigidBody.get(), btVector3(-4.8f, -0.0f, 0.f), btVector3(0.f, 0.f, 0.f), childAxis, parentAxis);
        scene->GetPhysicSystem()->AddConstraint(Hinge8, true);
        Hinge8->setDbgDrawSize(btScalar(1.5f));


        axles.emplace_back(Hinge1);
        axles.emplace_back(Hinge2);
        //axles.emplace_back(Hinge3);
        //axles.emplace_back(Hinge4);

        //back dif
        singleAxles.emplace_back(Hinge8);

        // Drive engine.
        Hinge8->enableMotor(true);
        Hinge8->setMaxMotorImpulse(engine.maxEngineForce / 100.f);
        Hinge8->setMotorTargetVelocity(0);

        //lock hinge wheel towards axle
        Hinge6->setLimit(0, 0.1); // a tiny bit of slack
        Hinge7->setLimit(0, 0.1);

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

            hinge->setLimit(2, 0, engine.suspensionRestLength);

            hinge->setDamping(2, engine.suspensionDamping);
            hinge->setStiffness(2, engine.suspensionStiffness);

            hinge->setLowerLimit(-SIMD_PI * 0.2f);
            hinge->setUpperLimit(SIMD_PI * 0.2f);

            //back wheels
            // if (hinge == Hinge3 || hinge == Hinge4) {
            //    hinge->setLowerLimit(-SIMD_HALF_PI * 0.01f);
            //   hinge->setUpperLimit(SIMD_HALF_PI * 0.01f);
            //}

            hinge->setDbgDrawSize(btScalar(1.5f));
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
    if (Input::Key(ImGuiKey_Space)) {
        auto &bl = scene->GetComponent<PointLightComponent>(BreakLight);
        bl.lightRange = 100.f;
        //todo brake
    } else {
        //reset lights
        auto &bl = scene->GetComponent<PointLightComponent>(BreakLight);

        bl.lightRange = 20.f;
    }
    if (ImGui::IsKeyPressed(ImGuiKey_F, false)) {
        auto &fhr = scene->GetComponent<PointLightComponent>(HeadLightR);
        auto &fhl = scene->GetComponent<PointLightComponent>(HeadLightL);
        auto &led = scene->GetComponent<PointLightComponent>(Ledbar);
        if (fhr.lightRange > 50) {
            fhr.lightRange = 10.f;
            fhl.lightRange = 10.f;
            led.lightRange = 10.f;
        } else {
            fhr.lightRange = 100.f;
            fhl.lightRange = 100.f;
            led.lightRange = 100.f;
        }
    }

    auto &wfr = scene->GetComponent<RigidBodyComponent>(Wheel_fr).RigidBody;
    auto &wfl = scene->GetComponent<RigidBodyComponent>(Wheel_fl).RigidBody;
    auto &wbr = scene->GetComponent<RigidBodyComponent>(Wheel_br).RigidBody;
    auto &wbl = scene->GetComponent<RigidBodyComponent>(Wheel_bl).RigidBody;

    std::vector<std::shared_ptr<btRigidBody>> wheels;
    wheels.emplace_back(wfr);
    wheels.emplace_back(wfl);
    wheels.emplace_back(wbr);
    wheels.emplace_back(wbl);

    for (auto &axle: axles) {
        axle->setTargetVelocity(3, engine.EngineForce);
    }
    for (auto &axle: singleAxles) {
        axle->setMotorTargetVelocity(engine.EngineForce);
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
