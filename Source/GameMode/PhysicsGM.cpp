#include "PhysicsGM.h"
#include "../Components.h"
#include "../Application.h"
#include "btBulletDynamicsCommon.h"
#include "BulletSoftBody/btSoftBodyHelpers.h"
#include "../ObjLoader.h"
#include "../Renderer/ModelManager.h"


namespace FLOOF {
    using namespace bt;

    void PhysicsGM::OnCreate() {


    }

    void PhysicsGM::OnUpdateEditor(float deltaTime) {

        ImGui::Begin("Spawn Rigid Bodies");
        if (ImGui::Button("Ball")) {
            auto &reg = m_Scene.GetCulledScene();
            auto *camera = Application::Get().GetRenderCamera();
            auto ent = SpawnRigidMesh(camera->Position, glm::vec3(2.f), 200.f, "Assets/Ball.obj","Assets/BallTexture.png",CollisionPrimitive::Sphere);
            auto &body = m_Scene.GetCulledScene().get<RigidBodyComponent>(ent);
            if (m_Scene.GetPhysicSystem())
                m_Scene.GetPhysicSystem()->AddRigidBody(body.RigidBody.get());
        }
        if (ImGui::Button("Cube")) {
            auto &reg = m_Scene.GetCulledScene();
            auto *camera = Application::Get().GetRenderCamera();
            auto ent = SpawnRigidMesh(camera->Position, glm::vec3(2.f), 100.f, "Assets/IdentityCube.obj","Assets/BallTexture.png",CollisionPrimitive::Box);
            auto &body = m_Scene.GetCulledScene().get<RigidBodyComponent>(ent);
            if (m_Scene.GetPhysicSystem())
                m_Scene.GetPhysicSystem()->AddRigidBody(body.RigidBody.get());
        }
        if (ImGui::Button("Cone")) {
            auto &reg = m_Scene.GetCulledScene();
            auto *camera = Application::Get().GetRenderCamera();
            auto ent = SpawnRigidMesh(camera->Position, glm::vec3(2.f), 200.f, "Assets/LowPolyCone.fbx","Assets/BallTexture.png",CollisionPrimitive::Cone);
            auto &body = m_Scene.GetCulledScene().get<RigidBodyComponent>(ent);
            if (m_Scene.GetPhysicSystem())
                m_Scene.GetPhysicSystem()->AddRigidBody(body.RigidBody.get());
        }
        if (ImGui::Button("Cylinder")) {
            auto &reg = m_Scene.GetCulledScene();
            auto *camera = Application::Get().GetRenderCamera();
            auto ent = SpawnRigidMesh(camera->Position, glm::vec3(2.0f), 200.f, "Assets/LowPolyCylinder.fbx","Assets/BallTexture.png",  CollisionPrimitive::Cylinder);
            auto &body = m_Scene.GetCulledScene().get<RigidBodyComponent>(ent);
            if (m_Scene.GetPhysicSystem())
                m_Scene.GetPhysicSystem()->AddRigidBody(body.RigidBody.get());
        }
        if (ImGui::Button("Convex Torus")) {
            auto &reg = m_Scene.GetCulledScene();
            auto *camera = Application::Get().GetRenderCamera();
            auto ent = SpawnRigidMesh(camera->Position, glm::vec3(2.f), 200.f, "Assets/LowPolyTorus.fbx","Assets/BallTexture.png");
            auto &body = m_Scene.GetCulledScene().get<RigidBodyComponent>(ent);
            if (m_Scene.GetPhysicSystem())
                m_Scene.GetPhysicSystem()->AddRigidBody(body.RigidBody.get());
        }

        if (ImGui::Button("Convex Statue")) {
            auto &reg = m_Scene.GetCulledScene();
            auto *camera = Application::Get().GetRenderCamera();
            auto ent = SpawnRigidMesh(camera->Position, glm::vec3(1 / 100.f), 400.f,"Assets/statue/source/statue1.fbx","Assets/statue/textures/staue1Color.png");
            auto &body = m_Scene.GetCulledScene().get<RigidBodyComponent>(ent);
            if (m_Scene.GetPhysicSystem())
                m_Scene.GetPhysicSystem()->AddRigidBody(body.RigidBody.get());
        }
        if (ImGui::Button("Bigger Statue")) {
            auto &reg = m_Scene.GetCulledScene();
            auto *camera = Application::Get().GetRenderCamera();
            auto ent = SpawnRigidMesh(camera->Position, glm::vec3(1 / 50.f), 600.f, "Assets/statue/source/statue1.fbx","Assets/statue/textures/staue1Color.png");
            auto &body = m_Scene.GetCulledScene().get<RigidBodyComponent>(ent);
            if (m_Scene.GetPhysicSystem())
                m_Scene.GetPhysicSystem()->AddRigidBody(body.RigidBody.get());
        }
        ImGui::End();

        ImGui::Begin("Spawn Soft Bodies");
        if (ImGui::Button("Soft Ball")) {
            auto &reg = m_Scene.GetCulledScene();
            auto *camera = Application::Get().GetRenderCamera();
            auto ent = SpawnSoftMesh(camera->Position, glm::vec3(5.f), 800.f, "Assets/LowPolySphere.fbx","Assets/BallTexture.png");
            auto &body = m_Scene.GetCulledScene().get<SoftBodyComponent>(ent);
            if (m_Scene.GetPhysicSystem())
                m_Scene.GetPhysicSystem()->AddSoftBody(body.SoftBody);
        }
        if (ImGui::Button("Soft Cylinder")) {
            auto &reg = m_Scene.GetCulledScene();
            auto *camera = Application::Get().GetRenderCamera();
            auto ent = SpawnSoftMesh(camera->Position, glm::vec3(5.f), 800.f, "Assets/LowPolyCylinder.fbx","Assets/BallTexture.png");
            auto &body = m_Scene.GetCulledScene().get<SoftBodyComponent>(ent);
            if (m_Scene.GetPhysicSystem())
                m_Scene.GetPhysicSystem()->AddSoftBody(body.SoftBody);
        }
        if (ImGui::Button("Soft Torus")) {
            auto &reg = m_Scene.GetCulledScene();
            auto *camera = Application::Get().GetRenderCamera();
            auto ent = SpawnSoftMesh(camera->Position, glm::vec3(5.f), 800.f, "Assets/LowPolyTorus.fbx","Assets/BallTexture.png");
            auto &body = m_Scene.GetCulledScene().get<SoftBodyComponent>(ent);
            if (m_Scene.GetPhysicSystem())
                m_Scene.GetPhysicSystem()->AddSoftBody(body.SoftBody);
        }
        if (ImGui::Button("Soft Dense Ball")) {
            auto &reg = m_Scene.GetCulledScene();
            auto *camera = Application::Get().GetRenderCamera();
            auto ent = SpawnSoftMesh(camera->Position, glm::vec3(10.f), 1000.f, "Assets/Ball.obj","Assets/BallTexture.png");
            auto &body = m_Scene.GetCulledScene().get<SoftBodyComponent>(ent);
            if (m_Scene.GetPhysicSystem())
                m_Scene.GetPhysicSystem()->AddSoftBody(body.SoftBody);
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

        ImGui::Begin("DebugDraw");
        if (ImGui::Combo("DebugDrawMode",
                         &debugMode,
                         DebugTypeStrings,
                         IM_ARRAYSIZE(DebugTypeStrings))) {
            m_Scene.GetPhysicsDebugDrawer()->setDebugMode(DebugTypeEnum[debugMode]);
        }
        ImGui::End();

    }

    const entt::entity PhysicsGM::SpawnSoftMesh(glm::vec3 Location, glm::vec3 Scale, const float mass, const std::string FilePath, const std::string Texture) {
        auto& app = Application::Get();
        auto m_Scene = app.m_Scene;

        const auto entity = m_Scene->CreateEntity("Softbody");
        auto &sm = m_Scene->AddComponent<StaticMeshComponent>(entity);
        m_Scene->AddComponent<TextureComponent>(entity, Texture);
        sm.meshes = ModelManager::Get().LoadModelMesh(FilePath);

        //test python script
        auto &script = m_Scene->AddComponent<ScriptComponent>(entity,"Scripts/HelloWorld.py");

        auto &transform = m_Scene->GetComponent<TransformComponent>(entity);

        //create softbody
        auto btvert = ModelManager::Get().LoadbtModel(FilePath, Scale);
        btSoftBody *psb = btSoftBodyHelpers::CreateFromConvexHull(*m_Scene->GetPhysicSystem()->getSoftBodyWorldInfo(),
                                                                  &btvert.btVertices[0], btvert.VertCount, true);
        psb->translate(btVector3(Location.x, Location.y, Location.z));
        auto &collision = m_Scene->AddComponent<SoftBodyComponent>(entity, 0.7, 0.7, mass, psb);

        transform.Position = Location;
        transform.Scale = Scale;

        return entity;
    }

    const entt::entity PhysicsGM::SpawnRigidMesh(glm::vec3 Location, glm::vec3 Scale, const float mass, const std::string FilePath, const std::string Texture,
                                     CollisionPrimitive shape) {
        auto& app = Application::Get();
        auto m_Scene = app.m_Scene;

        const auto entity = m_Scene->CreateEntity("Rigid Mesh");

        if (shape == CollisionPrimitive::ConvexHull)
            auto &collision = m_Scene->AddComponent<RigidBodyComponent>(entity, Location, Scale, mass, FilePath);
        else
            auto &collision = m_Scene->AddComponent<RigidBodyComponent>(entity, Location, Scale, mass, shape);

        auto &sm = m_Scene->AddComponent<StaticMeshComponent>(entity);
        m_Scene->AddComponent<TextureComponent>(entity, Texture);
        sm.meshes = ModelManager::Get().LoadModelMesh(FilePath);

        auto &transform = m_Scene->GetComponent<TransformComponent>(entity);

        //test python script
        auto &script = m_Scene->AddComponent<ScriptComponent>(entity,"Scripts/HelloWorld.py");

        transform.Position = Location;
        transform.Scale = Scale;

        return entity;
    }

}