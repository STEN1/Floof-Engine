#include "MonsterTruckScript.h"
#include "../Components.h"
#include "../Renderer/Mesh.h"
#include "../Renderer/ModelManager.h"
#include "../Input.h"
#include "../Application.h"
#include "BulletSoftBody/btSoftBodyHelpers.h"
#include "../Utils.h"
#include "../Network/FloofNetworking.h"

FLOOF::MonsterTruckScript::MonsterTruckScript(glm::vec3 Pos):SpawnLocation(Pos) {

}

void FLOOF::MonsterTruckScript::OnCreate(Scene* scene, entt::entity entity) {
    NativeScript::OnCreate(scene, entity);
    TruckCallback = std::make_shared<TruckCollisionCallback>(scene, entity);
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
                        mesh.meshes[i].MeshMaterial.Diffuse = Texture(TextureColor::DarkGrey);
                        mesh.meshes[i].MeshMaterial.Metallic = Texture(TextureColor::White);
                        mesh.meshes[i].MeshMaterial.Roughness = Texture(TextureColor::LightGrey);

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

        auto &transform = scene->GetComponent<TransformComponent>(frame);
        transform.Scale = glm::vec3(8.f);
        transform.Position = SpawnLocation;

        auto &body = scene->AddComponent<RigidBodyComponent>(frame, transform.Position, transform.Scale, transform.Rotation, 3000.f, "Assets/Wheels/tesla-cybertruck-technic-animation-studios/source/Cybertruck_Frame.fbx");
        //body.RigidBody->setCollisionFlags(body.RigidBody->getCollisionFlags() | btCollisionObject::CF_NO_CONTACT_RESPONSE);
        body.setCollisionDispatcher(TruckCallback.get());
        auto &sound = scene->AddComponent<SoundSourceComponent>(frame, "Vehicles_idle2.wav");
        sound.GetClip("Vehicles_idle2.wav")->Looping(true);
        sound.AddClip("pinchcliffe.wav");


        //camera
        {
            Camera = scene->CreateEntity("camera", frame);
            scene->AddComponent<CameraComponent>(Camera);

            auto &tf = scene->GetComponent<TransformComponent>(Camera);
            tf.Position = glm::vec3(-3.f,1.7f,0.f);
            tf.Scale = glm::vec3(0.1f);
            //scene->AddComponent<StaticMeshComponent>(Camera,"Assets/LowPolySphere.fbx");
        }
        {
            CamTarget = scene->CreateEntity("CameraTarget",frame);
            auto &tf = scene->GetComponent<TransformComponent>(CamTarget);
            tf.Position = glm::vec3(2.4f,0.4f,0.f);
            tf.Scale = glm::vec3(0.1f);
            //scene->AddComponent<StaticMeshComponent>(CamTarget,"Assets/LowPolySphere.fbx");


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
            //auto & mesh = scene->AddComponent<StaticMeshComponent>(HeadLightR,"Assets/LowPolySphere.fbx");
            //mesh.meshes[0].MeshMaterial.Diffuse = Texture(TextureColor::White);
            //mesh.meshes[0].MeshMaterial.UpdateDescriptorSet();

            light.intensity = 10.f;

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

            light.intensity = 10.f;

            auto &transform = scene->GetComponent<TransformComponent>(HeadLightL);
            transform.Position = glm::vec3(1.3f, 0.35f, 0.4f);
            transform.Scale = glm::vec3(0.1f);
        }

    }
    {
        Wheel_fr = scene->CreateEntity("Wheel Front Right");

        auto& sound = scene->AddComponent<SoundSourceComponent>(Wheel_fr, "Vehicles_idle2.wav");
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
        transform.Position = glm::vec3(5.5f, -0.5f, -2.5f) + SpawnLocation;
        transform.Scale = glm::vec3(2.5f);
        transform.Rotation = glm::vec3(glm::pi<float>() / 2.f, 0.f, 0.f);
        glm::vec3 scale = glm::vec3(1.3f, 0.6f, 1.3f);
        auto &body = scene->AddComponent<RigidBodyComponent>(Wheel_fr, transform.Position, scale, transform.Rotation, 300.f, bt::CollisionPrimitive::Cylinder);
        auto &RigidBody = body.RigidBody;
        RigidBody->setAnisotropicFriction(btVector3(1.f,0.5f,0.5f));
        RigidBody->setFriction(engine.WheelFriction);
        RigidBody->setRollingFriction(1);
        RigidBody->setSpinningFriction(1);
    }
    {
        Wheel_fl = scene->CreateEntity("Wheel Front Left");
        auto& sound = scene->AddComponent<SoundSourceComponent>(Wheel_fl, "Vehicles_idle2.wav");
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
        transform.Position = glm::vec3(5.5f, -0.5f, 2.5f)+ SpawnLocation;
        transform.Scale = glm::vec3(2.5f);
        transform.Rotation = glm::vec3(glm::pi<float>() / 2.f, 0.f, 0.f);
        glm::vec3 scale = glm::vec3(1.3f, 0.6f, 1.3f);
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
        transform.Position = glm::vec3(-5.0f, -0.5f, -2.5f) + SpawnLocation;
        transform.Scale = glm::vec3(2.5f);
        transform.Rotation = glm::vec3(glm::pi<float>() / 2.f, 0.f, 0.f);
        glm::vec3 scale = glm::vec3(1.3f, 0.6f, 1.3f);
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
        transform.Position = glm::vec3(-5.0f, -0.5f, 2.5f) + SpawnLocation;
        transform.Scale = glm::vec3(2.5f);
        transform.Rotation = glm::vec3(glm::pi<float>() / 2.f, 0.f, 0.f);
        glm::vec3 scale = glm::vec3(1.3f, 0.6f, 1.3f);
        auto &body = scene->AddComponent<RigidBodyComponent>(Wheel_bl, transform.Position, scale, transform.Rotation, 300.f, bt::CollisionPrimitive::Cylinder);
        auto &RigidBody = body.RigidBody;
        RigidBody->setAnisotropicFriction(btVector3(1.f,0.5f,0.5f));
        RigidBody->setFriction(engine.WheelFriction);
        RigidBody->setRollingFriction(1);
        RigidBody->setSpinningFriction(1);
    }
    //make gears
    {
        engine.Gears.emplace_back(5.f,3.f);
        engine.Gears.emplace_back(15.f,2.f);
        engine.Gears.emplace_back(25.f,1.5f);
        engine.Gears.emplace_back(50.f,1.f);
        engine.Gears.emplace_back(70.f,0.5f);
        engine.Gears.emplace_back(90.f,0.25f);

        engine.maxVelocity = engine.Gears[engine.CurrentGear].first;
    }
    //hinge car togheter
    {

        auto &frameBody = scene->GetComponent<RigidBodyComponent>(frame);
        auto &WheelfrBody = scene->GetComponent<RigidBodyComponent>(Wheel_fr);
        auto &WheelflBody = scene->GetComponent<RigidBodyComponent>(Wheel_fl);
        auto &WheelbrBody = scene->GetComponent<RigidBodyComponent>(Wheel_br);
        auto &WheelblBody = scene->GetComponent<RigidBodyComponent>(Wheel_bl);

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
            hinge->setServo(5, true);
            hinge->setMaxMotorForce(5, 8000.f);
            hinge->setTargetVelocity(5, 10.f);

            hinge->setParam(BT_CONSTRAINT_CFM, 0.15f, 1);
            hinge->setParam(BT_CONSTRAINT_ERP, 0.35f, 1);

            hinge->setLimit(2, 0, engine.suspensionRestLength);

            hinge->setDamping(2, engine.suspensionDamping);
            hinge->setStiffness(2, engine.suspensionStiffness);

            hinge->setLowerLimit(-SIMD_PI * 0.2f);
            hinge->setUpperLimit(SIMD_PI * 0.2f);

            hinge->setBreakingImpulseThreshold(40000.f);


            //back wheels
             if (hinge == Hinge3 || hinge == Hinge4) {
                hinge->setLowerLimit(-SIMD_HALF_PI * 0.1f);
               hinge->setUpperLimit(SIMD_HALF_PI * 0.1f);
            }

            hinge->setDbgDrawSize(btScalar(1.5f));
        }
    }

    //camera locations
    {
        CamLocations.emplace_back("Third Person",glm::vec3(-2.5f,1.2f,0.f),glm::vec3(2.f,0.4f,0.f));
        CamLocations.emplace_back("Close Third Person",glm::vec3(-1.5f,0.8f,0.f),glm::vec3(1.f,0.1f,0.f));
        CamLocations.emplace_back("First Person",glm::vec3(0.1f,0.4f,-0.15f),glm::vec3(2.4f,0.4f,0.f));
        CamLocations.emplace_back("Cinematic",glm::vec3(-2.f,1.f,-1.f),glm::vec3(2.f,0.4f,0.f));
        CamLocations.emplace_back("G T FUCKING A ",glm::vec3(0.2f,0.0f,-0.5f),glm::vec3(4.2f,0.5f,0.f));
    }


}

void FLOOF::MonsterTruckScript::OnUpdate(float deltaTime) {
    NativeScript::OnUpdate(deltaTime);

    auto* scene = m_Scene;
    auto &car = scene->GetComponent<RigidBodyComponent>(frame);
    bool windowIsActive = scene->IsActiveScene();

    bool turnKeyPressed{false};
    bool GasKeyPressed{false};

    auto* controller = m_Scene->TryGetComponent<PlayerControllerComponent>(frame);
    if(controller && controller->mPlayer == m_Scene->ActivePlayer){

        CameraUi();
        EngineUi();

        if (Input::Key(ImGuiKey_RightArrow) && windowIsActive) {
            turnKeyPressed |= true;
            engine.servoTarget -= engine.steeringIncrement;
            if(engine.servoTarget <= -engine.steeringClamp){
                engine.servoTarget = -engine.steeringClamp;
            }
        }
        if (Input::Key(ImGuiKey_LeftArrow) && windowIsActive) {
            turnKeyPressed |= true;
            engine.servoTarget += engine.steeringIncrement;
            if(engine.servoTarget >= engine.steeringClamp){
                engine.servoTarget = engine.steeringClamp;
            }
        }
        if (Input::Key(ImGuiKey_UpArrow) && windowIsActive) {
            GasKeyPressed |= true;
            engine.targetVelocity = engine.maxVelocity;
            engine.breakingForce = 0.f;
        }
        if (Input::Key(ImGuiKey_DownArrow) && windowIsActive) {
            GasKeyPressed |= true;
            engine.targetVelocity = -engine.maxVelocity;
            engine.breakingForce = 0.f;
        }
        if (Input::Key(ImGuiKey_Space) && windowIsActive) {
            auto &bl = scene->GetComponent<PointLightComponent>(BreakLight);
            bl.intensity = 100.f;
            //todo brake
            engine.breakingForce = engine.maxBreakingForce;
        } else {
            //reset lights
            auto &bl = scene->GetComponent<PointLightComponent>(BreakLight);

            bl.intensity = 20.f;
        }
        if (ImGui::IsKeyPressed(ImGuiKey_F, false) && windowIsActive) {
            auto &fhr = scene->GetComponent<PointLightComponent>(HeadLightR);
            auto &fhl = scene->GetComponent<PointLightComponent>(HeadLightL);

            if (fhr.intensity > 50) {
                fhr.intensity = 10.f;
                fhl.intensity = 10.f;
            } else {
                fhr.intensity = 100.f;
                fhl.intensity = 100.f;
            }
        }
        if(ImGui::IsKeyPressed(ImGuiKey_E, false) && windowIsActive){
            engine.CurrentGear++;
            if(engine.Gears.size() <= engine.CurrentGear){
                engine.CurrentGear = engine.Gears.size()-1;
            }
            engine.maxVelocity = engine.Gears[engine.CurrentGear].first;
            //engine.maxEngineForce = engine.Gears[engine.CurrentGear].second;
        }
        if(ImGui::IsKeyPressed(ImGuiKey_Q, false) && windowIsActive){
            engine.CurrentGear--;
            if(0 >= engine.CurrentGear){
                engine.CurrentGear = 0;
            }
            engine.maxVelocity = engine.Gears[engine.CurrentGear].first;
            //engine.maxEngineForce = engine.Gears[engine.CurrentGear].second;
        }
    }


    for (auto &axle: axles) {

        axle->setMaxMotorForce(3, engine.getEngineForce(abs(axle->getRigidBodyB().getLinearVelocity().length())));

        if(engine.breakingForce <= 0){
            axle->setTargetVelocity(3, engine.targetVelocity);
        }
        else {
            axle->setTargetVelocity(3, 0);
        }

    }
    //turning
    {
        //front wheels
        axles[0]->setServoTarget(5,engine.servoTarget);
        axles[1]->setServoTarget(5, engine.servoTarget);

        //back axles
        //turn other way if fast car
        if(axles[2]->getRigidBodyB().getLinearVelocity().length() > 15.f){
            //axles[2]->setServoTarget(5,engine.servoTarget/2.f);
            //axles[3]->setServoTarget(5, engine.servoTarget/2.f);

            //smaller sterring radius in big speed
            axles[0]->setLowerLimit(-SIMD_PI * 0.1f);
            axles[0]->setUpperLimit(SIMD_PI * 0.1f);
        }
        else{
            axles[2]->setServoTarget(5,-engine.servoTarget/2.f);
            axles[3]->setServoTarget(5, -engine.servoTarget/2.f);

            //bigger sterring radius in big speed
            axles[0]->setLowerLimit(-SIMD_PI * 0.2f);
            axles[0]->setUpperLimit(SIMD_PI * 0.2f);
        }

    }
    if(graphnumb > 999)
        graphnumb = 0;
    engine.velocityGraph[graphnumb] = car.RigidBody->getLinearVelocity().length();
    engine.TorqueGraph[graphnumb] = engine.getEngineForce(car.RigidBody->getLinearVelocity().length());
    if(car.RigidBody->getLinearVelocity().length() <= 0.f){
        engine.TorqueGraph[graphnumb] = 0.f;
    }
    engine.GraphOffset = graphnumb;
    graphnumb++;

    //makes you need to hold button for power
    engine.targetVelocity = 0.f;

    //reset steering
    if(!turnKeyPressed){
       engine.servoTarget = 0.f;
    }

    //audio todo fix sound
    auto& sound = m_Scene->GetComponent<SoundSourceComponent>(frame);


}

void FLOOF::MonsterTruckScript::CameraUi() {
    ImGui::Begin("Play Camera");

    static int p{0};
    std::vector<const char*> camNames(CamLocations.size());

    for(int i{0}; i < CamLocations.size(); i++) {
        camNames[i] = CamLocations[i].name.c_str();
    }

    if (ImGui::Combo("Camera View", &p, camNames.data(), camNames.size())) {
        auto camTrans = m_Scene->TryGetComponent<TransformComponent>(Camera);
        camTrans->Position = CamLocations[p].CamLoc;
        auto camtargetTrans = m_Scene->TryGetComponent<TransformComponent>(CamTarget);
        camtargetTrans->Position = CamLocations[p].CamTarget;
    }

    ImGui::End();

}

void FLOOF::MonsterTruckScript::EngineUi() {

    ImGui::Begin("engine Panel");
    ImGui::Text("Press 'F' to Toggle Headlights");
    ImGui::Text("Press 'Space' to Break");
    ImGui::Text("Press 'E' 'Q' to change Gear");
    auto* controller = m_Scene->TryGetComponent<PlayerControllerComponent>(frame);
    if(controller){
        std::string txt = "Player : ";
        txt +=std::to_string(controller->mPlayer);
        ImGui::Text(txt.c_str());
    }

        if (ImGui::CollapsingHeader("engine Graph")) {
            ImGui::PlotLines("Velocity", engine.velocityGraph.data(), engine.velocityGraph.size(), engine.GraphOffset, "m/s", 0.0f, 90.f, ImVec2(200, 100.0f));
            ImGui::PlotLines("Torque", engine.TorqueGraph.data(), engine.TorqueGraph.size(), engine.GraphOffset, "N", 1000.0f, 20000.f, ImVec2(200, 100.0f));
        }
        bool transformChanged{false};
        if (ImGui::CollapsingHeader("engine Controls")) {

            transformChanged |= ImGui::DragFloat("Max Velocity", &engine.maxVelocity);
            transformChanged |= ImGui::DragFloat("Max Torque", &engine.maxEngineForce);
            transformChanged |= ImGui::DragFloat("Max Breaking Force", &engine.maxBreakingForce);

        }
        if (ImGui::CollapsingHeader("Suspension Control")) {
            transformChanged |= ImGui::DragFloat("Suspension Stiffness", &engine.suspensionStiffness);
            transformChanged |= ImGui::DragFloat("Suspension Damping", &engine.suspensionDamping);
            transformChanged |= ImGui::DragFloat("Suspension Length", &engine.suspensionRestLength, 0.01, 0, 3);
        }
        if (ImGui::CollapsingHeader("Wheel Controls")) {
            transformChanged |= ImGui::DragFloat("Wheel Friction", &engine.WheelFriction, 0.1, 0.1, 100);
        }
        if (ImGui::CollapsingHeader("Gearing")) {
            std::string gear = "Current Gear : ";
            gear += std::to_string(engine.CurrentGear + 1);
            ImGui::Text(gear.c_str());

            int i = 1;
            for(auto& [speed, torque] : engine.Gears){
                std::string stats = "Gear " + std::to_string(i)  + " : " + std::to_string(speed) + " m/s " + std::to_string(torque) + "Gear Ratio";
                ImGui::Text(stats.c_str());
                i++;
            }


        if (transformChanged) {
            //remake stuff
            for (auto &hinge: engine.axles) {
                // Drive engine.
                hinge->setMaxMotorForce(3, engine.maxEngineForce);

                //suspension
                hinge->setDamping(2, engine.suspensionDamping);
                hinge->setStiffness(2, engine.suspensionStiffness);
                hinge->setLimit(2, 0, engine.suspensionRestLength);

                hinge->getRigidBodyB().setFriction(engine.WheelFriction);

            }

        }
    }

    ImGui::End();


}

void FLOOF::MonsterTruckScript::LastUpdate(float deltaTime) {
    NativeScript::LastUpdate(deltaTime);
    //set camera
    auto camTrans = m_Scene->TryGetComponent<TransformComponent>(Camera);
    auto camtargetTrans = m_Scene->TryGetComponent<TransformComponent>(CamTarget);

    auto cam = m_Scene->TryGetComponent<CameraComponent>(Camera);
    cam->Lookat(camTrans->GetWorldPosition(),camtargetTrans->GetWorldPosition());

}

void FLOOF::MonsterTruckScript::SetTransformData(FLOOF::CarData data) {

    auto &frameBody = m_Scene->GetComponent<RigidBodyComponent>(frame);
    auto &WheelfrBody = m_Scene->GetComponent<RigidBodyComponent>(Wheel_fr);
    auto &WheelflBody = m_Scene->GetComponent<RigidBodyComponent>(Wheel_fl);
    auto &WheelbrBody = m_Scene->GetComponent<RigidBodyComponent>(Wheel_br);
    auto &WheelblBody = m_Scene->GetComponent<RigidBodyComponent>(Wheel_bl);

    auto &frametf = m_Scene->GetComponent<TransformComponent>(frame);
    auto &Wheelfrtf = m_Scene->GetComponent<TransformComponent>(Wheel_fr);
    auto &Wheelfltf = m_Scene->GetComponent<TransformComponent>(Wheel_fl);
    auto &Wheelbrtf = m_Scene->GetComponent<TransformComponent>(Wheel_br);
    auto &Wheelbltf = m_Scene->GetComponent<TransformComponent>(Wheel_bl);

    frametf = data.MainTform;
    Wheelbltf = data.WheelTformBL;
    Wheelbrtf = data.WheelTformBR;
    Wheelfltf = data.WheelTformFL;
    Wheelfrtf = data.WheelTformFR;

    frameBody.transform(data.MainTform.Position,data.MainTform.Rotation,data.MainTform.Scale);
    WheelblBody.transform(data.WheelTformBL.Position,data.WheelTformBL.Rotation,data.WheelTformBL.Scale);
    WheelbrBody.transform(data.WheelTformBR.Position,data.WheelTformBR.Rotation,data.WheelTformBR.Scale);
    WheelflBody.transform(data.WheelTformFL.Position,data.WheelTformFL.Rotation,data.WheelTformFL.Scale);
    WheelfrBody.transform(data.WheelTformFR.Position,data.WheelTformFR.Rotation,data.WheelTformFR.Scale);
}

FLOOF::CarData FLOOF::MonsterTruckScript::GetTransformData() {
    CarData data;
    auto &frametf = m_Scene->GetComponent<TransformComponent>(frame);
    auto &Wheelfrtf = m_Scene->GetComponent<TransformComponent>(Wheel_fr);
    auto &Wheelfltf = m_Scene->GetComponent<TransformComponent>(Wheel_fl);
    auto &Wheelbrtf = m_Scene->GetComponent<TransformComponent>(Wheel_br);
    auto &Wheelbltf = m_Scene->GetComponent<TransformComponent>(Wheel_bl);

    data.MainTform =  frametf ;
    data.WheelTformBL= Wheelbltf;
    data.WheelTformBR = Wheelbrtf;
    data.WheelTformFL = Wheelfltf;
    data.WheelTformFR = Wheelfrtf;

    return data;
}

void FLOOF::MonsterTruckScript::AddToPhysicsWorld() {

    auto &frameBody = m_Scene->GetComponent<RigidBodyComponent>(frame);
    auto &WheelfrBody = m_Scene->GetComponent<RigidBodyComponent>(Wheel_fr);
    auto &WheelflBody = m_Scene->GetComponent<RigidBodyComponent>(Wheel_fl);
    auto &WheelbrBody = m_Scene->GetComponent<RigidBodyComponent>(Wheel_br);
    auto &WheelblBody = m_Scene->GetComponent<RigidBodyComponent>(Wheel_bl);

    auto physics = m_Scene->GetPhysicSystem();

    physics->AddRigidBody(frameBody.RigidBody.get());
    physics->AddRigidBody(WheelfrBody.RigidBody.get());
    physics->AddRigidBody(WheelflBody.RigidBody.get());
    physics->AddRigidBody(WheelbrBody.RigidBody.get());
    physics->AddRigidBody(WheelblBody.RigidBody.get());

}


void FLOOF::MonsterTruckScript::TruckCollisionCallback::onBeginOverlap(void *obj1, void *obj2) {
    std::cout << "On Begin Overlap" << std::endl;

}

void FLOOF::MonsterTruckScript::TruckCollisionCallback::onOverlap(void *obj1, void *obj2) {
    CollisionDispatcher::onOverlap(obj1, obj2);
}

void FLOOF::MonsterTruckScript::TruckCollisionCallback::onEndOverlap(void *obj) {
    CollisionDispatcher::onEndOverlap(obj);
    std::cout << "On End Overlap" << std::endl;
}
