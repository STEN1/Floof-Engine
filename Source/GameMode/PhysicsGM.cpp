#include "PhysicsGM.h"
#include "../Components.h"
#include "../Application.h"
#include "btBulletDynamicsCommon.h"
#include "BulletSoftBody/btSoftBodyHelpers.h"
#include "../ObjLoader.h"
#include "../Renderer/ModelManager.h"

void FLOOF::PhysicsGM::OnCreate()
{

}

void FLOOF::PhysicsGM::OnUpdateEditor(float deltaTime)
{
    ImGui::Begin("Spawn Rigid Bodies");
    if (ImGui::Button("Ball")) {
        auto& reg = m_Scene.GetCulledScene();
        auto* camera = Application::Get().GetRenderCamera();
       auto ent = SpawnBall(camera->Position, 2.f, 200.f, "Assets/BallTexture.png");
       auto& body = m_Scene.GetCulledScene().get<RigidBodyComponent>(ent);
      if(m_Scene.GetPhysicSystem())
          m_Scene.GetPhysicSystem()->AddRigidBody(body.RigidBody.get());
    }
    if (ImGui::Button("Cube")) {
        auto& reg = m_Scene.GetCulledScene();
        auto* camera = Application::Get().GetRenderCamera();
        auto ent = SpawnCube(camera->Position, glm::vec3(2.f),100.f, "Assets/BallTexture.png");
        auto& body = m_Scene.GetCulledScene().get<RigidBodyComponent>(ent);
        if(m_Scene.GetPhysicSystem())
            m_Scene.GetPhysicSystem()->AddRigidBody(body.RigidBody.get());
    }
    if (ImGui::Button("Convex Cone")) {
        auto& reg = m_Scene.GetCulledScene();
        auto* camera = Application::Get().GetRenderCamera();
        auto ent= SpawnRigidMesh(camera->Position, glm::vec3(.05f),200.f,"Assets/LowPolyCone.fbx");
        auto& body = m_Scene.GetCulledScene().get<RigidBodyComponent>(ent);
        if(m_Scene.GetPhysicSystem())
            m_Scene.GetPhysicSystem()->AddRigidBody(body.RigidBody.get());
    }
    if (ImGui::Button("Convex Cylinder")) {
        auto& reg = m_Scene.GetCulledScene();
        auto* camera = Application::Get().GetRenderCamera();
        auto ent= SpawnRigidMesh(camera->Position, glm::vec3(.05f),200.f,"Assets/LowPolyCylinder.fbx");
        auto& body = m_Scene.GetCulledScene().get<RigidBodyComponent>(ent);
        if(m_Scene.GetPhysicSystem())
            m_Scene.GetPhysicSystem()->AddRigidBody(body.RigidBody.get());
    }
    if (ImGui::Button("Convex Torus")) {
        auto& reg = m_Scene.GetCulledScene();
        auto* camera = Application::Get().GetRenderCamera();
        auto ent= SpawnRigidMesh(camera->Position, glm::vec3(.05f),200.f,"Assets/LowPolyTorus.fbx");
        auto& body = m_Scene.GetCulledScene().get<RigidBodyComponent>(ent);
        if(m_Scene.GetPhysicSystem())
            m_Scene.GetPhysicSystem()->AddRigidBody(body.RigidBody.get());
    }

    if (ImGui::Button("Convex Statue")) {
        auto& reg = m_Scene.GetCulledScene();
        auto* camera = Application::Get().GetRenderCamera();
        auto ent= SpawnRigidMesh(camera->Position, glm::vec3(1/100.f),400.f,"Assets/statue/source/statue1.fbx");
        auto& body = m_Scene.GetCulledScene().get<RigidBodyComponent>(ent);
        if(m_Scene.GetPhysicSystem())
            m_Scene.GetPhysicSystem()->AddRigidBody(body.RigidBody.get());
    }
    if (ImGui::Button("Bigger Statue")) {
        auto& reg = m_Scene.GetCulledScene();
        auto* camera = Application::Get().GetRenderCamera();
        auto ent= SpawnRigidMesh(camera->Position, glm::vec3(1/50.f),600.f,"Assets/statue/source/statue1.fbx");
        auto& body = m_Scene.GetCulledScene().get<RigidBodyComponent>(ent);
        if(m_Scene.GetPhysicSystem())
            m_Scene.GetPhysicSystem()->AddRigidBody(body.RigidBody.get());
    }
    ImGui::End();

    ImGui::Begin("Spawn Soft Bodies");
    if (ImGui::Button("Soft Ball")) {
        auto& reg = m_Scene.GetCulledScene();
        auto* camera = Application::Get().GetRenderCamera();
        auto ent = SpawnSoftMesh(camera->Position, glm::vec3(0.1f),800.f,"Assets/LowPolySphere.fbx");
        auto& body = m_Scene.GetCulledScene().get<SoftBodyComponent>(ent);
        if(m_Scene.GetPhysicSystem())
            m_Scene.GetPhysicSystem()->AddSoftBody(body.SoftBody);
    }
    if (ImGui::Button("Soft Cylinder")) {
        auto& reg = m_Scene.GetCulledScene();
        auto* camera = Application::Get().GetRenderCamera();
        auto ent = SpawnSoftMesh(camera->Position, glm::vec3(0.1f),800.f,"Assets/LowPolyCylinder.fbx");
        auto& body = m_Scene.GetCulledScene().get<SoftBodyComponent>(ent);
        if(m_Scene.GetPhysicSystem())
            m_Scene.GetPhysicSystem()->AddSoftBody(body.SoftBody);
    }
    if (ImGui::Button("Soft Torus")) {
        auto& reg = m_Scene.GetCulledScene();
        auto* camera = Application::Get().GetRenderCamera();
        auto ent = SpawnSoftMesh(camera->Position, glm::vec3(0.1f),800.f,"Assets/LowPolyTorus.fbx");
        auto& body = m_Scene.GetCulledScene().get<SoftBodyComponent>(ent);
        if(m_Scene.GetPhysicSystem())
            m_Scene.GetPhysicSystem()->AddSoftBody(body.SoftBody);
    }
    if (ImGui::Button("Soft Dense Ball")) {
        auto& reg = m_Scene.GetCulledScene();
        auto* camera = Application::Get().GetRenderCamera();
        auto ent = SpawnSoftMesh(camera->Position, glm::vec3(10.f),1000.f, "Assets/Ball.obj");
        auto& body = m_Scene.GetCulledScene().get<SoftBodyComponent>(ent);
        if(m_Scene.GetPhysicSystem())
            m_Scene.GetPhysicSystem()->AddSoftBody(body.SoftBody);
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

const entt::entity FLOOF::PhysicsGM::SpawnBall(glm::vec3 location, const float radius, const float mass, const std::string& texture)
{

    const auto ballEntity = m_Scene.CreateEntity("Simulated Ball");
    auto& collision = m_Scene.AddComponent<RigidBodyComponent>(ballEntity,location,glm::vec3(radius),mass,CollisionPrimitive::Sphere);
    m_Scene.AddComponent<MeshComponent>(ballEntity, "Assets/Ball.obj");
    m_Scene.AddComponent<TextureComponent>(ballEntity, texture);
    auto& transform = m_Scene.GetComponent<TransformComponent>(ballEntity);

    transform.Position = glm::vec3(collision.Transform.getOrigin().getX(),collision.Transform.getOrigin().getY(),collision.Transform.getOrigin().getZ());
    transform.Scale = glm::vec3(radius);

    return ballEntity;
}

const entt::entity FLOOF::PhysicsGM::SpawnCube(glm::vec3 location, glm::vec3 extents, const float mass, const std::string &texture) {

    const auto CubeEntity = m_Scene.CreateEntity("Simulated Cube");
    auto& collision = m_Scene.AddComponent<RigidBodyComponent>(CubeEntity,location,extents,mass,CollisionPrimitive::Box);
    m_Scene.AddComponent<MeshComponent>(CubeEntity, "Assets/IdentityCube.obj");
    m_Scene.AddComponent<TextureComponent>(CubeEntity, texture);
     auto& transform = m_Scene.GetComponent<TransformComponent>(CubeEntity);

    transform.Position = glm::vec3(collision.Transform.getOrigin().getX(),collision.Transform.getOrigin().getY(),collision.Transform.getOrigin().getZ());
    transform.Scale = extents;

    return CubeEntity;
}

const entt::entity FLOOF::PhysicsGM::SpawnSoftMesh(glm::vec3 Location, glm::vec3 Scale, const float mass, const std::string FilePath) {

    const auto entity = m_Scene.CreateEntity("Softbody");
    auto& sm = m_Scene.AddComponent<StaticMeshComponent>(entity);
    m_Scene.AddComponent<TextureComponent>(entity, "Assets/BallTexture.png");
    sm.meshes = ModelManager::Get().LoadModelMesh(FilePath).meshes;

    auto& transform = m_Scene.GetComponent<TransformComponent>(entity);

    //create softbody
    auto btvert = ModelManager::Get().LoadbtModel(FilePath,Scale);
    btSoftBody* psb = btSoftBodyHelpers::CreateFromConvexHull(*m_Scene.GetPhysicSystem()->getSoftBodyWorldInfo(),&btvert.btVertices[0] ,btvert.VertCount, true);
    psb->translate(btVector3(Location.x,Location.y,Location.z));
    auto& collision =  m_Scene.AddComponent<SoftBodyComponent>(entity,0.7,0.7,mass,psb);

    transform.Position = Location;
    transform.Scale = Scale;

    return entity;
}

const entt::entity
FLOOF::PhysicsGM::SpawnRigidMesh(glm::vec3 Location, glm::vec3 Scale, const float mass, const std::string FilePath) {
    const auto entity = m_Scene.CreateEntity("Simulated Statue");
    auto& collision = m_Scene.AddComponent<RigidBodyComponent>(entity,Location,Scale,mass,FilePath);
    auto& sm = m_Scene.AddComponent<StaticMeshComponent>(entity);
    m_Scene.AddComponent<TextureComponent>(entity, "Assets/BallTexture.png");
    sm.meshes = ModelManager::Get().LoadModelMesh(FilePath).meshes;

    auto& transform = m_Scene.GetComponent<TransformComponent>(entity);

    transform.Position = glm::vec3(collision.Transform.getOrigin().getX(),collision.Transform.getOrigin().getY(),collision.Transform.getOrigin().getZ());
    transform.Scale = Scale;  // mesh is giga

    return entity;
}
