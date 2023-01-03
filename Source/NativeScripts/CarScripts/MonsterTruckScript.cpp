#include "MonsterTruckScript.h"
#include "../../SoundComponent.h"

void FLOOF::MonsterTruckScript::OnCreate(FLOOF::Scene *scene, entt::entity entity) {
    frame = entity;
    TruckCallback = std::make_shared<TruckCollisionCallback>(scene, entity);
    CarType = 2;

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
                    if (textureorder[i] == CyberTruckTexture::glass)
                        mesh.meshes[i].MeshMaterial.HasOpacity = true;

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

    {
        auto &transform = scene->GetComponent<TransformComponent>(frame);
        transform.Scale = glm::vec3(8.f);
        transform.Position = SpawnLocation;

        auto &body = scene->AddComponent<RigidBodyComponent>(frame, transform.Position, transform.Scale, transform.Rotation, 3000.f, "Assets/Wheels/tesla-cybertruck-technic-animation-studios/source/Cybertruck_Frame.fbx");
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


    {
        Wheel_fr = scene->CreateEntity("Wheel Front Right");

        auto& sound = scene->AddComponent<SoundComponent>(Wheel_fr, "Vehicles_idle2.wav");
        auto &mesh = scene->AddComponent<StaticMeshComponent>(Wheel_fr, "Assets/Wheels/tesla-cybertruck-technic-animation-studios/source/Cybertruck_WheelRight.fbx");
        mesh.meshes[1].MeshMaterial.Diffuse = Texture(TextureColor::DarkGrey);
        mesh.meshes[1].MeshMaterial.Metallic = Texture("Assets/Wheels/tesla-cybertruck-technic-animation-studios/textures/ring_plus_metallicRoughness_png@channels=B.png");
        mesh.meshes[1].MeshMaterial.Normals = Texture("Assets/Wheels/tesla-cybertruck-technic-animation-studios/textures/ring_plus_normal.png");
        mesh.meshes[1].MeshMaterial.Roughness = Texture("Assets/Wheels/tesla-cybertruck-technic-animation-studios/textures/ring_plus_metallicRoughness_png@channels=G.png");
        mesh.meshes[1].MeshMaterial.UpdateDescriptorSet();

        mesh.meshes[0].MeshMaterial.Diffuse = Texture(TextureColor::Black);
        mesh.meshes[0].MeshMaterial.Metallic = Texture("Assets/Wheels/tesla-cybertruck-technic-animation-studios/textures/ring_plus_metallicRoughness_png@channels=B.png");
        mesh.meshes[0].MeshMaterial.Normals = Texture("Assets/Wheels/tesla-cybertruck-technic-animation-studios/textures/ring_plus_normal.png");
        mesh.meshes[0].MeshMaterial.Roughness = Texture("Assets/Wheels/tesla-cybertruck-technic-animation-studios/textures/ring_plus_metallicRoughness_png@channels=G.png");
        mesh.meshes[0].MeshMaterial.UpdateDescriptorSet();

        mesh.meshes[2].MeshMaterial.Diffuse = Texture(TextureColor::Black);
        mesh.meshes[2].MeshMaterial.Metallic = Texture("Assets/Wheels/tesla-cybertruck-technic-animation-studios/textures/lastik_metallicRoughness_png@channels=B.png");
        mesh.meshes[2].MeshMaterial.Roughness = Texture("Assets/Wheels/tesla-cybertruck-technic-animation-studios/textures/lastik_metallicRoughness_png@channels=G.png");
        mesh.meshes[2].MeshMaterial.UpdateDescriptorSet();


        auto &transform = scene->GetComponent<TransformComponent>(Wheel_fr);
        transform.Position = glm::vec3(5.5f, -0.5f, -2.5f) + SpawnLocation;
        transform.Scale = glm::vec3(8.f);
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
        auto& sound = scene->AddComponent<SoundComponent>(Wheel_fl, "Vehicles_idle2.wav");
        auto &mesh = scene->AddComponent<StaticMeshComponent>(Wheel_fl, "Assets/Wheels/tesla-cybertruck-technic-animation-studios/source/Cybertruck_WheelLeft.fbx");
        mesh.meshes[1].MeshMaterial.Diffuse = Texture(TextureColor::DarkGrey);
        mesh.meshes[1].MeshMaterial.Metallic = Texture("Assets/Wheels/tesla-cybertruck-technic-animation-studios/textures/ring_plus_metallicRoughness_png@channels=B.png");
        mesh.meshes[1].MeshMaterial.Normals = Texture("Assets/Wheels/tesla-cybertruck-technic-animation-studios/textures/ring_plus_normal.png");
        mesh.meshes[1].MeshMaterial.Roughness = Texture("Assets/Wheels/tesla-cybertruck-technic-animation-studios/textures/ring_plus_metallicRoughness_png@channels=G.png");
        mesh.meshes[1].MeshMaterial.UpdateDescriptorSet();

        mesh.meshes[0].MeshMaterial.Diffuse = Texture(TextureColor::Black);
        mesh.meshes[0].MeshMaterial.Metallic = Texture("Assets/Wheels/tesla-cybertruck-technic-animation-studios/textures/ring_plus_metallicRoughness_png@channels=B.png");
        mesh.meshes[0].MeshMaterial.Normals = Texture("Assets/Wheels/tesla-cybertruck-technic-animation-studios/textures/ring_plus_normal.png");
        mesh.meshes[0].MeshMaterial.Roughness = Texture("Assets/Wheels/tesla-cybertruck-technic-animation-studios/textures/ring_plus_metallicRoughness_png@channels=G.png");
        mesh.meshes[0].MeshMaterial.UpdateDescriptorSet();

        mesh.meshes[2].MeshMaterial.Diffuse = Texture(TextureColor::Black);
        mesh.meshes[2].MeshMaterial.Metallic = Texture("Assets/Wheels/tesla-cybertruck-technic-animation-studios/textures/lastik_metallicRoughness_png@channels=B.png");
        mesh.meshes[2].MeshMaterial.Roughness = Texture("Assets/Wheels/tesla-cybertruck-technic-animation-studios/textures/lastik_metallicRoughness_png@channels=G.png");
        mesh.meshes[2].MeshMaterial.UpdateDescriptorSet();

        auto &transform = scene->GetComponent<TransformComponent>(Wheel_fl);
        transform.Position = glm::vec3(5.5f, -0.5f, 2.5f)+ SpawnLocation;
        transform.Scale = glm::vec3(8.f);
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
        auto &mesh = scene->AddComponent<StaticMeshComponent>(Wheel_br, "Assets/Wheels/tesla-cybertruck-technic-animation-studios/source/Cybertruck_WheelRight.fbx");
        mesh.meshes[1].MeshMaterial.Diffuse = Texture(TextureColor::DarkGrey);
        mesh.meshes[1].MeshMaterial.Metallic = Texture("Assets/Wheels/tesla-cybertruck-technic-animation-studios/textures/ring_plus_metallicRoughness_png@channels=B.png");
        mesh.meshes[1].MeshMaterial.Normals = Texture("Assets/Wheels/tesla-cybertruck-technic-animation-studios/textures/ring_plus_normal.png");
        mesh.meshes[1].MeshMaterial.Roughness = Texture("Assets/Wheels/tesla-cybertruck-technic-animation-studios/textures/ring_plus_metallicRoughness_png@channels=G.png");
        mesh.meshes[1].MeshMaterial.UpdateDescriptorSet();

        mesh.meshes[0].MeshMaterial.Diffuse = Texture(TextureColor::Black);
        mesh.meshes[0].MeshMaterial.Metallic = Texture("Assets/Wheels/tesla-cybertruck-technic-animation-studios/textures/ring_plus_metallicRoughness_png@channels=B.png");
        mesh.meshes[0].MeshMaterial.Normals = Texture("Assets/Wheels/tesla-cybertruck-technic-animation-studios/textures/ring_plus_normal.png");
        mesh.meshes[0].MeshMaterial.Roughness = Texture("Assets/Wheels/tesla-cybertruck-technic-animation-studios/textures/ring_plus_metallicRoughness_png@channels=G.png");
        mesh.meshes[0].MeshMaterial.UpdateDescriptorSet();

        mesh.meshes[2].MeshMaterial.Diffuse = Texture(TextureColor::Black);
        mesh.meshes[2].MeshMaterial.Metallic = Texture("Assets/Wheels/tesla-cybertruck-technic-animation-studios/textures/lastik_metallicRoughness_png@channels=B.png");
        mesh.meshes[2].MeshMaterial.Roughness = Texture("Assets/Wheels/tesla-cybertruck-technic-animation-studios/textures/lastik_metallicRoughness_png@channels=G.png");
        mesh.meshes[2].MeshMaterial.UpdateDescriptorSet();

        auto &transform = scene->GetComponent<TransformComponent>(Wheel_br);
        transform.Position = glm::vec3(-5.0f, -0.5f, -2.5f) + SpawnLocation;
        transform.Scale = glm::vec3(8.f);
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
        auto &mesh = scene->AddComponent<StaticMeshComponent>(Wheel_bl, "Assets/Wheels/tesla-cybertruck-technic-animation-studios/source/Cybertruck_WheelLeft.fbx");
        mesh.meshes[1].MeshMaterial.Diffuse = Texture(TextureColor::DarkGrey);
        mesh.meshes[1].MeshMaterial.Metallic = Texture("Assets/Wheels/tesla-cybertruck-technic-animation-studios/textures/ring_plus_metallicRoughness_png@channels=B.png");
        mesh.meshes[1].MeshMaterial.Normals = Texture("Assets/Wheels/tesla-cybertruck-technic-animation-studios/textures/ring_plus_normal.png");
        mesh.meshes[1].MeshMaterial.Roughness = Texture("Assets/Wheels/tesla-cybertruck-technic-animation-studios/textures/ring_plus_metallicRoughness_png@channels=G.png");
        mesh.meshes[1].MeshMaterial.UpdateDescriptorSet();

        mesh.meshes[0].MeshMaterial.Diffuse = Texture(TextureColor::Black);
        mesh.meshes[0].MeshMaterial.Metallic = Texture("Assets/Wheels/tesla-cybertruck-technic-animation-studios/textures/ring_plus_metallicRoughness_png@channels=B.png");
        mesh.meshes[0].MeshMaterial.Normals = Texture("Assets/Wheels/tesla-cybertruck-technic-animation-studios/textures/ring_plus_normal.png");
        mesh.meshes[0].MeshMaterial.Roughness = Texture("Assets/Wheels/tesla-cybertruck-technic-animation-studios/textures/ring_plus_metallicRoughness_png@channels=G.png");
        mesh.meshes[0].MeshMaterial.UpdateDescriptorSet();

        mesh.meshes[2].MeshMaterial.Diffuse = Texture(TextureColor::Black);
        mesh.meshes[2].MeshMaterial.Metallic = Texture("Assets/Wheels/tesla-cybertruck-technic-animation-studios/textures/lastik_metallicRoughness_png@channels=B.png");
        mesh.meshes[2].MeshMaterial.Roughness = Texture("Assets/Wheels/tesla-cybertruck-technic-animation-studios/textures/lastik_metallicRoughness_png@channels=G.png");
        mesh.meshes[2].MeshMaterial.UpdateDescriptorSet();

        auto &transform = scene->GetComponent<TransformComponent>(Wheel_bl);
        transform.Position = glm::vec3(-5.0f, -0.5f, 2.5f) + SpawnLocation;
        transform.Scale = glm::vec3(8.f);
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
        engine.Gears.emplace_back(20.f,1.5f);
        engine.Gears.emplace_back(25.f,1.f);
        engine.Gears.emplace_back(30.f,0.5f);
        engine.Gears.emplace_back(40.f,0.25f);

        engine.maxVelocity = engine.Gears[engine.CurrentGear].first;
    }
    //camera locations
    {
        CamLocations.emplace_back("Third Person",glm::vec3(-2.5f,1.2f,0.f),glm::vec3(2.f,0.4f,0.f));
        CamLocations.emplace_back("Close Third Person",glm::vec3(-1.5f,0.8f,0.f),glm::vec3(1.f,0.1f,0.f));
        CamLocations.emplace_back("First Person",glm::vec3(0.1f,0.4f,-0.15f),glm::vec3(2.4f,0.4f,0.f));
        CamLocations.emplace_back("Cinematic",glm::vec3(-2.f,1.f,-1.f),glm::vec3(2.f,0.4f,0.f));
        CamLocations.emplace_back("G T FUCKING A ",glm::vec3(0.2f,0.0f,-0.5f),glm::vec3(4.2f,0.5f,0.f));
    }

    //parent oncreate last since it needs to hinge car togheter
    CarBaseScript::OnCreate(scene, entity);
}

void FLOOF::MonsterTruckScript::OnUpdate(float deltaTime) {
    CarBaseScript::OnUpdate(deltaTime);

}

void FLOOF::MonsterTruckScript::LastUpdate(float deltaTime) {
    CarBaseScript::LastUpdate(deltaTime);

}
