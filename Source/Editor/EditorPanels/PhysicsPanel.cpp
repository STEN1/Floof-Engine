#include "PhysicsPanel.h"
#include "../../Application.h"
#include "../../GameMode/PhysicsGM.h"
#include "../../Components.h"
void FLOOF::PhysicsPanel::DrawPanel() {
    auto& app = Application::Get();
    auto m_Scene = app.m_Scene;
    //blank panel
    ImGui::Begin("Physics Panel");

    ImGui::Begin("Spawn Rigid Bodies");
    if (ImGui::Button("Ball")) {
        auto *camera = Application::Get().GetRenderCamera();
        auto ent = PhysicsGM::SpawnRigidMesh(camera->Position, glm::vec3(2.f), 200.f, "Assets/Ball.obj","Assets/BallTexture.png",bt::CollisionPrimitive::Sphere);
        auto &body = m_Scene->GetComponent<RigidBodyComponent>(ent);
        if (m_Scene->GetPhysicSystem())
            m_Scene->GetPhysicSystem()->AddRigidBody(body.RigidBody.get());
    }
    if (ImGui::Button("Cube")) {
        auto *camera = Application::Get().GetRenderCamera();
        auto ent = PhysicsGM::SpawnRigidMesh(camera->Position, glm::vec3(2.f), 100.f, "Assets/IdentityCube.obj","Assets/BallTexture.png",bt::CollisionPrimitive::Box);
        auto &body = m_Scene->GetComponent<RigidBodyComponent>(ent);
        if (m_Scene->GetPhysicSystem())
            m_Scene->GetPhysicSystem()->AddRigidBody(body.RigidBody.get());
    }
    if (ImGui::Button("Cone")) {
        auto *camera = Application::Get().GetRenderCamera();
        auto ent = PhysicsGM::SpawnRigidMesh(camera->Position, glm::vec3(2.f), 200.f, "Assets/LowPolyCone.fbx","Assets/BallTexture.png",bt::CollisionPrimitive::Cone);
        auto &body = m_Scene->GetComponent<RigidBodyComponent>(ent);
        if (m_Scene->GetPhysicSystem())
            m_Scene->GetPhysicSystem()->AddRigidBody(body.RigidBody.get());
    }
    if (ImGui::Button("Cylinder")) {
        auto *camera = Application::Get().GetRenderCamera();
        auto ent = PhysicsGM::SpawnRigidMesh(camera->Position, glm::vec3(2.0f), 200.f, "Assets/LowPolyCylinder.fbx","Assets/BallTexture.png",  bt::CollisionPrimitive::Cylinder);
        auto &body = m_Scene->GetComponent<RigidBodyComponent>(ent);
        if (m_Scene->GetPhysicSystem())
            m_Scene->GetPhysicSystem()->AddRigidBody(body.RigidBody.get());
    }
    if (ImGui::Button("Convex Torus")) {
        auto *camera = Application::Get().GetRenderCamera();
        auto ent = PhysicsGM::SpawnRigidMesh(camera->Position, glm::vec3(2.f), 200.f, "Assets/LowPolyTorus.fbx","Assets/BallTexture.png");
        auto &body = m_Scene->GetComponent<RigidBodyComponent>(ent);
        if (m_Scene->GetPhysicSystem())
            m_Scene->GetPhysicSystem()->AddRigidBody(body.RigidBody.get());
    }

    if (ImGui::Button("Convex Statue")) {
        auto *camera = Application::Get().GetRenderCamera();
        auto ent = PhysicsGM::SpawnRigidMesh(camera->Position, glm::vec3(1 / 100.f), 400.f,"Assets/statue/source/statue1.fbx","Assets/statue/textures/staue1Color.png");
        auto &body = m_Scene->GetComponent<RigidBodyComponent>(ent);
        if (m_Scene->GetPhysicSystem())
            m_Scene->GetPhysicSystem()->AddRigidBody(body.RigidBody.get());
    }
    if (ImGui::Button("Bigger Statue")) {
        auto *camera = Application::Get().GetRenderCamera();
        auto ent = PhysicsGM::SpawnRigidMesh(camera->Position, glm::vec3(1 / 50.f), 600.f, "Assets/statue/source/statue1.fbx","Assets/statue/textures/staue1Color.png");
        auto &body = m_Scene->GetComponent<RigidBodyComponent>(ent);
        if (m_Scene->GetPhysicSystem())
            m_Scene->GetPhysicSystem()->AddRigidBody(body.RigidBody.get());
    }
    ImGui::End();

    ImGui::Begin("Spawn Soft Bodies");
    if (ImGui::Button("Soft Ball")) {
        auto *camera = Application::Get().GetRenderCamera();
        auto ent = PhysicsGM::SpawnSoftMesh(camera->Position, glm::vec3(5.f), 800.f, "Assets/LowPolySphere.fbx","Assets/BallTexture.png");
        auto &body = m_Scene->GetComponent<SoftBodyComponent>(ent);
        if (m_Scene->GetPhysicSystem())
            m_Scene->GetPhysicSystem()->AddSoftBody(body.SoftBody);
    }
    if (ImGui::Button("Soft Cylinder")) {
        auto *camera = Application::Get().GetRenderCamera();
        auto ent = PhysicsGM::SpawnSoftMesh(camera->Position, glm::vec3(5.f), 800.f, "Assets/LowPolyCylinder.fbx","Assets/BallTexture.png");
        auto &body = m_Scene->GetComponent<SoftBodyComponent>(ent);
        if (m_Scene->GetPhysicSystem())
            m_Scene->GetPhysicSystem()->AddSoftBody(body.SoftBody);
    }
    if (ImGui::Button("Soft Torus")) {
        auto *camera = Application::Get().GetRenderCamera();
        auto ent = PhysicsGM::SpawnSoftMesh(camera->Position, glm::vec3(5.f), 800.f, "Assets/LowPolyTorus.fbx","Assets/BallTexture.png");
        auto &body = m_Scene->GetComponent<SoftBodyComponent>(ent);
        if (m_Scene->GetPhysicSystem())
            m_Scene->GetPhysicSystem()->AddSoftBody(body.SoftBody);
    }
    if (ImGui::Button("Soft Dense Ball")) {
        auto *camera = Application::Get().GetRenderCamera();
        auto ent = PhysicsGM::SpawnSoftMesh(camera->Position, glm::vec3(10.f), 1000.f, "Assets/Ball.obj","Assets/BallTexture.png");
        auto &body = m_Scene->GetComponent<SoftBodyComponent>(ent);
        if (m_Scene->GetPhysicSystem())
            m_Scene->GetPhysicSystem()->AddSoftBody(body.SoftBody);
    }
    ImGui::End();

    static int debugMode{btIDebugDraw::DBG_NoDebug};
    static const char *DebugTypeStrings[] =
            {
                    "DBG_NoDebug",
                    "DBG_DrawWireframe",
                    "DBG_DrawAabb",
                    "DBG_DrawFeaturesText",
                    "DBG_DrawContactPoints",
                    "DBG_NoDeactivation",
                    "DBG_NoHelpText",
                    "DBG_DrawText",
                    "DBG_ProfileTimings",
                    "DBG_EnableSatComparison",
                    "DBG_DisableBulletLCP",
                    "DBG_EnableCCD",
                    "DBG_DrawConstraints",
                    "DBG_DrawConstraintLimits",
                    "DBG_FastWireframe",
                    "DBG_DrawNormals",
                    "DBG_DrawFrames",
            };
    static const int DebugTypeEnum[] = {
            0,
            1,
            2,
            4,
            8,
            16,
            32,
            64,
            128,
            256,
            512,
            1024,
            (1 << 11),
            (1 << 12),
            (1 << 13),
            (1 << 14),
            (1 << 15)
    };

    ImGui::BeginChild("DebugDraw");
    if (ImGui::Combo("DebugDrawMode",
                     &debugMode,
                     DebugTypeStrings,
                     IM_ARRAYSIZE(DebugTypeStrings))) {
        m_Scene->GetPhysicsDebugDrawer()->setDebugMode(DebugTypeEnum[debugMode]);
    }
    ImGui::EndChild();

    ImGui::End();
}
