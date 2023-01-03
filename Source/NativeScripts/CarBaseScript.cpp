#include "CarBaseScript.h"
#include "../Components.h"
#include "../Renderer/Mesh.h"
#include "../Renderer/ModelManager.h"
#include "../Input.h"
#include "../Application.h"
#include "BulletSoftBody/btSoftBodyHelpers.h"
#include "../Utils.h"
#include "../Network/FloofNetworking.h"

FLOOF::CarBaseScript::CarBaseScript(glm::vec3 Pos) : SpawnLocation(Pos) {

}

void FLOOF::CarBaseScript::OnCreate(Scene *scene, entt::entity entity) {
    NativeScript::OnCreate(scene, entity);
    {

        //camera
        {
            Camera = scene->CreateEntity("camera", frame);
            scene->AddComponent<CameraComponent>(Camera);

            auto &tf = scene->GetComponent<TransformComponent>(Camera);
            tf.Position = glm::vec3(-3.f, 1.7f, 0.f);
            tf.Scale = glm::vec3(0.1f);
            //scene->AddComponent<StaticMeshComponent>(Camera,"Assets/LowPolySphere.fbx");
        }
        {
            CamTarget = scene->CreateEntity("CameraTarget", frame);
            auto &tf = scene->GetComponent<TransformComponent>(CamTarget);
            tf.Position = glm::vec3(2.4f, 0.4f, 0.f);
            tf.Scale = glm::vec3(0.1f);
            //scene->AddComponent<StaticMeshComponent>(CamTarget,"Assets/LowPolySphere.fbx");


        }

    }

    //sound
    {
        auto &sound = scene->AddComponent<SoundComponent>(frame, "Vehicles_idle2.wav");
        sound.GetClip("Vehicles_idle2.wav")->Looping(true);
        sound.AddClip("pinchcliffe.wav");
        TruckCallback->SetImpactSound(sound.AddClip("metal_impact_mono.wav"));
        sound.AddClip("honk.wav");

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
            hinge->setMaxMotorForce(5, engine.maxEngineForce * 2.f);
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

    auto camTrans = m_Scene->TryGetComponent<TransformComponent>(Camera);
    camTrans->Position = CamLocations[0].CamLoc;
    auto camtargetTrans = m_Scene->TryGetComponent<TransformComponent>(CamTarget);
    camtargetTrans->Position = CamLocations[0].CamTarget;
}

void FLOOF::CarBaseScript::OnUpdate(float deltaTime) {
    NativeScript::OnUpdate(deltaTime);

    auto *scene = m_Scene;
    auto &car = scene->GetComponent<RigidBodyComponent>(frame);
    bool windowIsActive = scene->IsActiveScene();

    bool turnKeyPressed{false};
    bool GasKeyPressed{false};

    auto *controller = m_Scene->TryGetComponent<PlayerControllerComponent>(frame);
    if (controller && controller->mPlayer == m_Scene->ActivePlayer) {

        CameraUi();
        EngineUi();

        if (Input::Key(ImGuiKey_RightArrow) && windowIsActive) {
            turnKeyPressed |= true;
            engine.servoTarget -= engine.steeringIncrement;
            if (engine.servoTarget <= -engine.steeringClamp) {
                engine.servoTarget = -engine.steeringClamp;
            }
        }
        if (Input::Key(ImGuiKey_LeftArrow) && windowIsActive) {
            turnKeyPressed |= true;
            engine.servoTarget += engine.steeringIncrement;
            if (engine.servoTarget >= engine.steeringClamp) {
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
        if (ImGui::IsKeyPressed(ImGuiKey_E, false) && windowIsActive) {
            engine.CurrentGear++;
            if (engine.Gears.size() <= engine.CurrentGear) {
                engine.CurrentGear = engine.Gears.size() - 1;
            }
            engine.maxVelocity = engine.Gears[engine.CurrentGear].first;
            //engine.maxEngineForce = engine.Gears[engine.CurrentGear].second;
            
        }
        if (ImGui::IsKeyPressed(ImGuiKey_W, false) && windowIsActive) { 
            scene->GetComponent<SoundComponent>(frame).GetClip("honk.wav")->Play();
        }
        if (ImGui::IsKeyPressed(ImGuiKey_Q, false) && windowIsActive) {
            engine.CurrentGear--;
            if (0 >= engine.CurrentGear) {
                engine.CurrentGear = 0;
            }
            engine.maxVelocity = engine.Gears[engine.CurrentGear].first;
            //engine.maxEngineForce = engine.Gears[engine.CurrentGear].second;
        }
    }


    for (auto &axle: axles) {

        axle->setMaxMotorForce(3, engine.getEngineForce(abs(axle->getRigidBodyB().getLinearVelocity().length())));

        if (engine.breakingForce <= 0) {
            axle->setTargetVelocity(3, engine.targetVelocity);
        } else {
            axle->setTargetVelocity(3, 0);
        }

    }
    //turning
    {
        //front wheels
        axles[0]->setServoTarget(5, engine.servoTarget);
        axles[1]->setServoTarget(5, engine.servoTarget);

        //back axles
        //turn other way if fast car
        if (axles[2]->getRigidBodyB().getLinearVelocity().length() > 15.f) {
            //smaller sterring radius in big speed
            axles[0]->setLowerLimit(-engine.TurnHighSpeed);
            axles[0]->setUpperLimit(engine.TurnHighSpeed);
            axles[1]->setLowerLimit(-engine.TurnHighSpeed);
            axles[1]->setUpperLimit(engine.TurnHighSpeed);
        } else {
            if (BackWheelTurn) {
                axles[2]->setServoTarget(5, -engine.servoTarget / 2.f);
                axles[3]->setServoTarget(5, -engine.servoTarget / 2.f);
            }
            //bigger sterring radius in big speed
            axles[0]->setLowerLimit(-engine.TurnLowSpeed);
            axles[0]->setUpperLimit(engine.TurnLowSpeed);
            axles[1]->setLowerLimit(-engine.TurnLowSpeed);
            axles[1]->setUpperLimit(engine.TurnLowSpeed);
        }

    }
    if (graphnumb > 999)
        graphnumb = 0;
    engine.velocityGraph[graphnumb] = car.RigidBody->getLinearVelocity().length();
    engine.TorqueGraph[graphnumb] = engine.getEngineForce(car.RigidBody->getLinearVelocity().length());
    if (car.RigidBody->getLinearVelocity().length() <= 0.f) {
        engine.TorqueGraph[graphnumb] = 0.f;
    }
    engine.GraphOffset = graphnumb;
    graphnumb++;

    //makes you need to hold button for power
    engine.targetVelocity = 0.f;

    //reset steering
    if (!turnKeyPressed) {
        engine.servoTarget = 0.f;
    }

    //audio todo fix sound
    auto &sound = m_Scene->GetComponent<SoundComponent>(frame);


}

void FLOOF::CarBaseScript::CameraUi() {
    ImGui::Begin("Play Camera");

    static int p{0};
    std::vector<const char *> camNames(CamLocations.size());

    for (int i{0}; i < CamLocations.size(); i++) {
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

void FLOOF::CarBaseScript::EngineUi() {

    ImGui::Begin("engine Panel");
    ImGui::Text("Press 'F' to Toggle Headlights");
    ImGui::Text("Press 'Space' to Break");
    ImGui::Text("Press 'E' 'Q' to change Gear");
    auto *controller = m_Scene->TryGetComponent<PlayerControllerComponent>(frame);
    if (controller) {
        std::string txt = "Player : ";
        txt += std::to_string(controller->mPlayer);
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
        for (auto &[speed, torque]: engine.Gears) {
            std::string stats = "Gear " + std::to_string(i) + " : " + std::to_string(speed) + " m/s " + std::to_string(torque) + "Gear Ratio";
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

void FLOOF::CarBaseScript::LastUpdate(float deltaTime) {
    NativeScript::LastUpdate(deltaTime);
    //set camera
    auto camTrans = m_Scene->TryGetComponent<TransformComponent>(Camera);
    auto camtargetTrans = m_Scene->TryGetComponent<TransformComponent>(CamTarget);

    auto cam = m_Scene->TryGetComponent<CameraComponent>(Camera);
    cam->Lookat(camTrans->GetWorldPosition(), camtargetTrans->GetWorldPosition());

}

void FLOOF::CarBaseScript::SetTransformData(FLOOF::CarData data) {

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

    frametf.Position = data.MainTform.Position;
    frametf.Rotation = data.MainTform.Rotation;
    //if(data.MainTform.Scale != glm::vec3(1.f))
    //  frametf.Scale = data.MainTform.Scale;

    Wheelbltf.Position = data.WheelTformBL.Position;
    Wheelbltf.Rotation = data.WheelTformBL.Rotation;
    //if(data.WheelTformBL.Scale != glm::vec3(1.f))
    //Wheelbltf.Scale = data.WheelTformBL.Scale;

    Wheelbrtf.Position = data.WheelTformBR.Position;
    Wheelbrtf.Rotation = data.WheelTformBR.Rotation;
    //if(data.WheelTformBR.Scale != glm::vec3(1.f))
    //Wheelbrtf.Scale = data.WheelTformBR.Scale;

    Wheelfltf.Rotation = data.WheelTformFL.Rotation;
    Wheelfltf.Position = data.WheelTformFL.Position;
    // if(data.WheelTformFL.Scale != glm::vec3(1.f))
    //Wheelfltf.Scale = data.WheelTformFL.Scale;

    Wheelfrtf.Position = data.WheelTformFR.Position;
    Wheelfrtf.Rotation = data.WheelTformFR.Rotation;
    //if(data.WheelTformFR.Scale != glm::vec3(1.f))
    //Wheelfrtf.Scale = data.WheelTformFR.Scale;


    frameBody.transform(data.MainTform.Position, data.MainTform.Rotation, data.MainTform.Scale);
    WheelblBody.transform(data.WheelTformBL.Position, data.WheelTformBL.Rotation, data.WheelTformBL.Scale);
    WheelbrBody.transform(data.WheelTformBR.Position, data.WheelTformBR.Rotation, data.WheelTformBR.Scale);
    WheelflBody.transform(data.WheelTformFL.Position, data.WheelTformFL.Rotation, data.WheelTformFL.Scale);
    WheelfrBody.transform(data.WheelTformFR.Position, data.WheelTformFR.Rotation, data.WheelTformFR.Scale);

    frameBody.RigidBody->setAngularVelocity(data.AvFrame);
    WheelblBody.RigidBody->setAngularVelocity(data.AvWheelBL);
    WheelbrBody.RigidBody->setAngularVelocity(data.AvWheelBR);
    WheelflBody.RigidBody->setAngularVelocity(data.AvWheelFL);
    WheelfrBody.RigidBody->setAngularVelocity(data.AvWheelFR);

}

FLOOF::CarData FLOOF::CarBaseScript::GetTransformData() {
    CarData data;

    data.CarType = CarType;

    auto &frametf = m_Scene->GetComponent<TransformComponent>(frame);
    auto &Wheelfrtf = m_Scene->GetComponent<TransformComponent>(Wheel_fr);
    auto &Wheelfltf = m_Scene->GetComponent<TransformComponent>(Wheel_fl);
    auto &Wheelbrtf = m_Scene->GetComponent<TransformComponent>(Wheel_br);
    auto &Wheelbltf = m_Scene->GetComponent<TransformComponent>(Wheel_bl);

    data.MainTform.Position = frametf.Position;
    data.MainTform.Rotation = frametf.Rotation;
    //data.MainTform.Scale = frametf.Scale;

    data.WheelTformBL.Position = Wheelbltf.Position;
    data.WheelTformBL.Rotation = Wheelbltf.Rotation;
    //data.WheelTformBL.Scale = Wheelbltf.Scale;

    data.WheelTformBR.Position = Wheelbrtf.Position;
    data.WheelTformBR.Rotation = Wheelbrtf.Rotation;
    //data.WheelTformBR.Scale = Wheelbrtf.Scale;

    data.WheelTformFL.Rotation = Wheelfltf.Rotation;
    data.WheelTformFL.Position = Wheelfltf.Position;
    //data.WheelTformFL.Scale = Wheelfltf.Scale;

    data.WheelTformFR.Position = Wheelfrtf.Position;
    data.WheelTformFR.Rotation = Wheelfrtf.Rotation;
    //data.WheelTformFR.Scale = Wheelfrtf.Scale;

    auto &frameBody = m_Scene->GetComponent<RigidBodyComponent>(frame);
    auto &WheelfrBody = m_Scene->GetComponent<RigidBodyComponent>(Wheel_fr);
    auto &WheelflBody = m_Scene->GetComponent<RigidBodyComponent>(Wheel_fl);
    auto &WheelbrBody = m_Scene->GetComponent<RigidBodyComponent>(Wheel_br);
    auto &WheelblBody = m_Scene->GetComponent<RigidBodyComponent>(Wheel_bl);

    data.AvFrame = frameBody.RigidBody->getAngularVelocity();
    data.AvWheelBL = WheelblBody.RigidBody->getAngularVelocity();
    data.AvWheelBR = WheelbrBody.RigidBody->getAngularVelocity();
    data.AvWheelFL = WheelflBody.RigidBody->getAngularVelocity();
    data.AvWheelFR = WheelfrBody.RigidBody->getAngularVelocity();

    return data;
}

void FLOOF::CarBaseScript::AddToPhysicsWorld() {

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

FLOOF::CarBaseScript::~CarBaseScript() {

    m_Scene->DestroyEntity(Wheel_bl);
    m_Scene->DestroyEntity(Wheel_br);
    m_Scene->DestroyEntity(Wheel_fl);
    m_Scene->DestroyEntity(Wheel_fr);

}


void FLOOF::CarBaseScript::TruckCollisionCallback::onBeginOverlap(void *obj1, void *obj2) {
    std::cout << "On Begin Overlap" << std::endl;
    if (ImpactSound) { ImpactSound->Play(); }
}

void FLOOF::CarBaseScript::TruckCollisionCallback::onOverlap(void *obj1, void *obj2) {
    CollisionDispatcher::onOverlap(obj1, obj2);
}

void FLOOF::CarBaseScript::TruckCollisionCallback::onEndOverlap(void *obj) {
    CollisionDispatcher::onEndOverlap(obj);
    std::cout << "On End Overlap" << std::endl;
}
