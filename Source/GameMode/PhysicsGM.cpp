#include "PhysicsGM.h"
#include "../Components.h"
#include "../Application.h"
#include "btBulletDynamicsCommon.h"
#include "BulletSoftBody/btSoftBodyHelpers.h"
#include "../ObjLoader.h"

void FLOOF::PhysicsGM::OnCreate()
{
    //scene is created in application
}

void FLOOF::PhysicsGM::OnUpdateEditor(float deltaTime)
{
    ImGui::Begin("Physics");
    if (ImGui::Button("Spawn ball")) {
        auto& reg = m_Scene.GetCulledScene();
        auto* camera = Application::Get().GetRenderCamera();
       auto ball = SpawnBall(camera->Position, 2.f, 200.f, 0.9f, "Assets/BallTexture.png");
       auto& body = m_Scene.GetCulledScene().get<RigidBodyComponent>(ball);
      if(m_Scene.GetPhysicSystem())
          m_Scene.GetPhysicSystem()->AddRigidBody(body.RigidBody.get());
    }
    if (ImGui::Button("Spawn Cube")) {
        auto& reg = m_Scene.GetCulledScene();
        auto* camera = Application::Get().GetRenderCamera();
        auto cube = SpawnCube(camera->Position, glm::vec3(2.f),100.f, "Assets/BallTexture.png");
        auto& body = m_Scene.GetCulledScene().get<RigidBodyComponent>(cube);
        if(m_Scene.GetPhysicSystem())
            m_Scene.GetPhysicSystem()->AddRigidBody(body.RigidBody.get());
    }
    ImGui::End();

    static int debugMode{btIDebugDraw::DBG_NoDebug};
    static const char* DebugTypeStrings[] =
            {
                    "DBG_NoDebug" ,
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
            (1<<11),
            (1<<12),
            (1<<13),
            (1<<14),
            (1<<15)
    };

    ImGui::Begin("DebugDraw");
    if (ImGui::Combo("DebugDrawMode",
                     &debugMode,
                     DebugTypeStrings,
                     IM_ARRAYSIZE(DebugTypeStrings)))
    {
        m_Scene.GetPhysicsDebugDrawer()->setDebugMode(DebugTypeEnum[debugMode]);
    }
    ImGui::End();


}

const entt::entity FLOOF::PhysicsGM::SpawnBall(glm::vec3 location, const float radius, const float mass, const float elasticity, const std::string& texture)
{


    const auto ballEntity = m_Scene.CreateEntity("Simulated Ball");
    auto& collision = m_Scene.AddComponent<RigidBodyComponent>(ballEntity,location,radius,mass);
    m_Scene.AddComponent<MeshComponent>(ballEntity, "Assets/Ball.obj");
    m_Scene.AddComponent<TextureComponent>(ballEntity, texture);
    auto& transform = m_Scene.GetComponent<TransformComponent>(ballEntity);

    transform.Position = glm::vec3(collision.Transform.getOrigin().getX(),collision.Transform.getOrigin().getY(),collision.Transform.getOrigin().getZ());
    transform.Scale = glm::vec3(radius);

    return ballEntity;
}

const entt::entity FLOOF::PhysicsGM::SpawnSoftBall(glm::vec3 location, const float radius, const float mass, const std::string &texture) {

    auto& m_Registry = m_Scene.GetCulledScene();

    const auto ballEntity = m_Registry.create();
    auto& textureComponent = m_Registry.emplace<TextureComponent>(ballEntity, texture);
    auto& mesh = m_Registry.emplace<MeshComponent>(ballEntity, "Assets/Ball.obj");
    auto& collision = m_Registry.emplace<SoftBodyComponent>(ballEntity);
    auto& transform = m_Scene.GetComponent<TransformComponent>(ballEntity);
    //auto obj = ObjLoader("Assets/Ball.obj");
    //auto data = obj.GetIndexedData();

    collision.CollisionShape = std::make_shared<btSphereShape>(radius);

    //btSoftBodyHelpers::CreateFromTriMesh(*m_Scene.GetPhysicSystem()->getSoftBodyWorldInfo(), data.second.size(),data.second,mesh.Data.IndexCount);

    //transform.Position = glm::vec3(collision.Transform.getOrigin().getX(),collision.Transform.getOrigin().getY(),collision.Transform.getOrigin().getZ());
    transform.Scale = glm::vec3(radius);

    return ballEntity;
}

const entt::entity FLOOF::PhysicsGM::SpawnCube(glm::vec3 location, glm::vec3 extents, const float mass, const std::string &texture) {

    const auto CubeEntity = m_Scene.CreateEntity("Simulated Cube");
    auto& collision = m_Scene.AddComponent<RigidBodyComponent>(CubeEntity,location,extents,mass);
    m_Scene.AddComponent<MeshComponent>(CubeEntity, "Assets/IdentityCube.obj");
    m_Scene.AddComponent<TextureComponent>(CubeEntity, texture);
     auto& transform = m_Scene.GetComponent<TransformComponent>(CubeEntity);

    transform.Position = glm::vec3(collision.Transform.getOrigin().getX(),collision.Transform.getOrigin().getY(),collision.Transform.getOrigin().getZ());
    transform.Scale = extents;

    return CubeEntity;
}
