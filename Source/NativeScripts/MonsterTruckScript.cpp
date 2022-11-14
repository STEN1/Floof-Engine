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
        auto& mesh = scene->AddComponent<StaticMeshComponent>(frame, "Assets/MonsterTruck/MonstertruckFrameRotated.fbx");
        for (auto& mesh : mesh.meshes) {
            mesh.MeshMaterial.Diffuse = Texture("Assets/MonsterTruck/texturesFrame/HarmaaRunko_albedo.jpg");
            mesh.MeshMaterial.Metallic = Texture("Assets/MonsterTruck/texturesFrame/HarmaaRunko_metallic.jpg");
            mesh.MeshMaterial.AO = Texture("Assets/MonsterTruck/texturesFrame/HarmaaRunko_AO.jpg");
            mesh.MeshMaterial.Normals = Texture("Assets/MonsterTruck/texturesFrame/HarmaaRunko_normal.jpg");
            mesh.MeshMaterial.Roughness = Texture("Assets/MonsterTruck/texturesFrame/HarmaaRunko_roughness.jpg");
            mesh.MeshMaterial.UpdateDescriptorSet();
        }

        auto& engine = scene->AddComponent<EngineComponent>(frame);

        auto& transform = scene->GetComponent<TransformComponent>(frame);
        transform.Scale = glm::vec3(5.f);
        //transform.Rotation = glm::vec3(1.5f, 0.f, 0.f);

        auto & body = scene->AddComponent<RigidBodyComponent>(frame, transform.Position, transform.Scale,transform.Rotation,500.f,"Assets/MonsterTruck/MonstertruckFrameRotated.fbx");
    }
    {
        Wheel_fr = scene->CreateEntity("Wheel Front left");
        auto& mesh = scene->AddComponent<StaticMeshComponent>(Wheel_fr, "Assets/MonsterTruck/LPWheelFixed.fbx");
        for (auto& mesh : mesh.meshes) {
            mesh.MeshMaterial.Diffuse = Texture("Assets/MonsterTruck/texturesWheel/lambert1_Base_Color.png");
            mesh.MeshMaterial.Metallic = Texture("Assets/MonsterTruck/texturesWheel/lambert1_Metallic.png");
            mesh.MeshMaterial.AO = Texture("Assets/MonsterTruck/texturesWheel/lambert1_Mixed_AO.png");
            mesh.MeshMaterial.Normals = Texture("Assets/MonsterTruck/texturesWheel/lambert1_Normal_OpenGL.png", false, true);
            mesh.MeshMaterial.Roughness = Texture("Assets/MonsterTruck/texturesWheel/lambert1_Roughness.png");
            mesh.MeshMaterial.UpdateDescriptorSet();
        }

        auto& transform = scene->GetComponent<TransformComponent>(Wheel_fr);
        transform.Position = glm::vec3(35.f, -10.f, -20.f);
        transform.Scale = glm::vec3(6.5f);
        transform.Rotation = glm::vec3(0.f, 1.5f, 0.f);

        auto& frameTrans = scene->GetComponent<TransformComponent>(frame);
        //scene->AddComponent<RigidBodyComponent>(Wheel_fr, transform.Position-frameTrans.Position, transform.Scale+frameTrans.Scale,500.f,"Assets/MonsterTruck/LPWheelFixed.fbx");
        auto& body = scene->AddComponent<RigidBodyComponent>(Wheel_fr, transform.Position, transform.Scale,transform.Rotation,50.f,"Assets/MonsterTruck/LPWheelFixed.fbx");
        auto& RigidBody = body.RigidBody;
        RigidBody->setFriction(WheelFriction);
        RigidBody->setRollingFriction(RollingFriction);
        RigidBody->setSpinningFriction(SpinningFriction);
    }
    {
        Wheel_fl = scene->CreateEntity("Wheel Front right");
        auto& mesh = scene->AddComponent<StaticMeshComponent>(Wheel_fl, "Assets/MonsterTruck/LPWheelFixed.fbx");
        for (auto& mesh : mesh.meshes) {
            mesh.MeshMaterial.Diffuse = Texture("Assets/MonsterTruck/texturesWheel/lambert1_Base_Color.png");
            mesh.MeshMaterial.Metallic = Texture("Assets/MonsterTruck/texturesWheel/lambert1_Metallic.png");
            mesh.MeshMaterial.AO = Texture("Assets/MonsterTruck/texturesWheel/lambert1_Mixed_AO.png");
            mesh.MeshMaterial.Normals = Texture("Assets/MonsterTruck/texturesWheel/lambert1_Normal_OpenGL.png", false, true);
            mesh.MeshMaterial.Roughness = Texture("Assets/MonsterTruck/texturesWheel/lambert1_Roughness.png");
            mesh.MeshMaterial.UpdateDescriptorSet();
        }

        auto& transform = scene->GetComponent<TransformComponent>(Wheel_fl);
        transform.Position = glm::vec3(35.f, -10.f, 20.f);
        transform.Scale = glm::vec3(6.5f);
        transform.Rotation = glm::vec3(0.f, -1.5f, 0.f);

        auto& frameTrans = scene->GetComponent<TransformComponent>(frame);
        //scene->AddComponent<RigidBodyComponent>(Wheel_fl, transform.Position-frameTrans.Position, transform.Scale+frameTrans.Scale,500.f,"Assets/MonsterTruck/LPWheelFixed.fbx");
        auto& body = scene->AddComponent<RigidBodyComponent>(Wheel_fl, transform.Position, transform.Scale,transform.Rotation,50.f,"Assets/MonsterTruck/LPWheelFixed.fbx");
        auto& RigidBody = body.RigidBody;
        RigidBody->setFriction(WheelFriction);
        RigidBody->setRollingFriction(RollingFriction);
        RigidBody->setSpinningFriction(SpinningFriction);
    }
    {
        Wheel_br = scene->CreateEntity("Wheel Back right");
        auto& mesh = scene->AddComponent<StaticMeshComponent>(Wheel_br, "Assets/MonsterTruck/LPWheelFixed.fbx");
        for (auto& mesh : mesh.meshes) {
            mesh.MeshMaterial.Diffuse = Texture("Assets/MonsterTruck/texturesWheel/lambert1_Base_Color.png");
            mesh.MeshMaterial.Metallic = Texture("Assets/MonsterTruck/texturesWheel/lambert1_Metallic.png");
            mesh.MeshMaterial.AO = Texture("Assets/MonsterTruck/texturesWheel/lambert1_Mixed_AO.png");
            mesh.MeshMaterial.Normals = Texture("Assets/MonsterTruck/texturesWheel/lambert1_Normal_OpenGL.png", false, true);
            mesh.MeshMaterial.Roughness = Texture("Assets/MonsterTruck/texturesWheel/lambert1_Roughness.png");
            mesh.MeshMaterial.UpdateDescriptorSet();
        }

        auto& transform = scene->GetComponent<TransformComponent>(Wheel_br);
        transform.Position = glm::vec3(-35.f, -10.f, -20.f);
        transform.Scale = glm::vec3(6.5f);
        transform.Rotation = glm::vec3(0.f, 1.5f, 0.f);

        auto& frameTrans = scene->GetComponent<TransformComponent>(frame);
        //scene->AddComponent<RigidBodyComponent>(Wheel_br, transform.Position-frameTrans.Position, transform.Scale+frameTrans.Scale,500.f,"Assets/MonsterTruck/LPWheelFixed.fbx");
        auto & body =scene->AddComponent<RigidBodyComponent>(Wheel_br, transform.Position, transform.Scale,transform.Rotation,50.f,"Assets/MonsterTruck/LPWheelFixed.fbx");
        body.RigidBody->setFriction(1000.f);
    }
    {
        Wheel_bl = scene->CreateEntity("Wheel back left");
        auto& mesh = scene->AddComponent<StaticMeshComponent>(Wheel_bl, "Assets/MonsterTruck/LPWheelFixed.fbx");
        for (auto& mesh : mesh.meshes) {
            mesh.MeshMaterial.Diffuse = Texture("Assets/MonsterTruck/texturesWheel/lambert1_Base_Color.png");
            mesh.MeshMaterial.Metallic = Texture("Assets/MonsterTruck/texturesWheel/lambert1_Metallic.png");
            mesh.MeshMaterial.AO = Texture("Assets/MonsterTruck/texturesWheel/lambert1_Mixed_AO.png");
            mesh.MeshMaterial.Normals = Texture("Assets/MonsterTruck/texturesWheel/lambert1_Normal_OpenGL.png", false, true);
            mesh.MeshMaterial.Roughness = Texture("Assets/MonsterTruck/texturesWheel/lambert1_Roughness.png");
            mesh.MeshMaterial.UpdateDescriptorSet();
        }

        auto& transform =scene->GetComponent<TransformComponent>(Wheel_bl);
        transform.Position = glm::vec3(-35.f,-10.f,20.f);
        transform.Scale = glm::vec3(6.5f);
        transform.Rotation = glm::vec3(0.f,-1.5f,0.f);

        auto& frameTrans = scene->GetComponent<TransformComponent>(frame);
        //scene->AddComponent<RigidBodyComponent>(Wheel_bl, transform.Position-frameTrans.Position, transform.Scale+frameTrans.Scale,500.f,"Assets/MonsterTruck/LPWheelFixed.fbx");
        auto & body= scene->AddComponent<RigidBodyComponent>(Wheel_bl, transform.Position, transform.Scale,transform.Rotation,50.f,"Assets/MonsterTruck/LPWheelFixed.fbx");
        auto& RigidBody = body.RigidBody;
        RigidBody->setFriction(WheelFriction);
        RigidBody->setRollingFriction(RollingFriction);
        RigidBody->setSpinningFriction(SpinningFriction);
    }
    //hinge car togheter
    {

        auto& frameBody = scene->GetComponent<RigidBodyComponent>(frame);
        auto& WheelfrBody = scene->GetComponent<RigidBodyComponent>(Wheel_fr);
        auto& WheelflBody = scene->GetComponent<RigidBodyComponent>(Wheel_fl);
        auto& WheelbrBody = scene->GetComponent<RigidBodyComponent>(Wheel_br);
        auto& WheelblBody = scene->GetComponent<RigidBodyComponent>(Wheel_bl);

        auto& engine = scene->GetComponent<EngineComponent>(frame);

        frameBody.RigidBody->setActivationState(DISABLE_DEACTIVATION);
        WheelfrBody.RigidBody->setActivationState(DISABLE_DEACTIVATION);
        WheelflBody.RigidBody->setActivationState(DISABLE_DEACTIVATION);
        WheelbrBody.RigidBody->setActivationState(DISABLE_DEACTIVATION);
        WheelblBody.RigidBody->setActivationState(DISABLE_DEACTIVATION);


        btVector3 parentAxis(0.f, 1.f, 0.f);
        btVector3 childAxis(0.f, 0.f, 1.f);

        std::vector<btHinge2Constraint*> hinges;

        btVector3 anchor = WheelfrBody.RigidBody->getWorldTransform().getOrigin();
        btHinge2Constraint* Hinge1 = new btHinge2Constraint(*frameBody.RigidBody.get(), *WheelfrBody.RigidBody.get(), anchor, parentAxis, childAxis);
        scene->GetPhysicSystem()->AddConstraint(Hinge1, true);

        anchor = WheelflBody.RigidBody->getWorldTransform().getOrigin();
        btHinge2Constraint* Hinge2 = new btHinge2Constraint(*frameBody.RigidBody.get(), *WheelflBody.RigidBody.get(), anchor, parentAxis, childAxis);
        scene->GetPhysicSystem()->AddConstraint(Hinge2, true);

        anchor = WheelbrBody.RigidBody->getWorldTransform().getOrigin();
        btHinge2Constraint* Hinge3 = new btHinge2Constraint(*frameBody.RigidBody.get(), *WheelbrBody.RigidBody.get(), anchor, parentAxis, childAxis);
        scene->GetPhysicSystem()->AddConstraint(Hinge3, true);

        anchor = WheelblBody.RigidBody->getWorldTransform().getOrigin();
        btHinge2Constraint* Hinge4 = new btHinge2Constraint(*frameBody.RigidBody.get(), *WheelblBody.RigidBody.get(), anchor, parentAxis, childAxis);
        scene->GetPhysicSystem()->AddConstraint(Hinge4, true);



        hinges.emplace_back(Hinge1);
        hinges.emplace_back(Hinge2);
        hinges.emplace_back(Hinge3);
        hinges.emplace_back(Hinge4);

        for(auto& hinge: hinges){

            // Drive engine.
            hinge->enableMotor(3, true);
            hinge->setMaxMotorForce(3, engine.maxEngineForce/2.f);
            hinge->setTargetVelocity(3, 0);

            // Steering engine.
            hinge->enableMotor(5, true);
            hinge->setMaxMotorForce(5, engine.maxEngineForce/2.f);
            hinge->setTargetVelocity(5, 0);

            hinge->setParam( BT_CONSTRAINT_CFM, 0.15f, 2);
            hinge->setParam( BT_CONSTRAINT_ERP, 0.35f, 2);

            hinge->setDamping( 2, 2.0 );
            hinge->setStiffness( 2, 40.0 );

            hinge->setLimit(0,0.f,0.f);
            hinge->setLimit(1,0.f,1.f);
            hinge->setLimit(2,0.f,0.f);

            hinge->setDbgDrawSize(btScalar(5.f));
        }
    }
}

void FLOOF::MonsterTruckScript::OnUpdate(float deltaTime) {
    NativeScript::OnUpdate(deltaTime);

    auto scene = Application::Get().m_Scene;
    auto& engine = scene->GetComponent<EngineComponent>(frame);

    if (Input::Key(ImGuiKey_LeftArrow)) {
        engine.TurnForce = -engine.MaxTurnForce;
            engine.VehicleSteering += engine.steeringClamp;
            if(engine.VehicleSteering > engine.steeringClamp){
                engine.VehicleSteering = engine.steeringClamp;
            }
    }
    if (Input::Key(ImGuiKey_RightArrow)) {
        engine.TurnForce = engine.MaxTurnForce;
        engine.VehicleSteering -= engine.steeringClamp;
        if(engine.VehicleSteering < -engine.steeringClamp){
            engine.VehicleSteering = -engine.steeringClamp;
        }
    }
    if (Input::Key(ImGuiKey_UpArrow)) {
        engine.EngineForce = engine.maxEngineForce;
        engine.BreakingForce = 0.f;
    }
    if (Input::Key(ImGuiKey_DownArrow)) {
        engine.EngineForce = -engine.maxEngineForce;
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

    //forward
    for(auto wheel: wheels){
        btVector3 torque(0.f,0.f,-1.f); // local direction
        torque *= engine.EngineForce;
        wheel->applyTorque(torque);

    }
    //turning
    {
        btVector3 torque(0.f,1.f,0.f); // local direction
        torque *= engine.TurnForce;
        wfl->applyTorque(torque);
        wfr->applyTorque(torque);

    }

    //makes you need to hold button for power
    engine.EngineForce = 0.f;

}
