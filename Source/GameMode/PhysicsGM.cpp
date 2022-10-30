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
    ImGui::Begin("Physics");
    if (ImGui::Button("Spawn ball")) {
        auto& reg = m_Scene.GetCulledScene();
        auto* camera = Application::Get().GetRenderCamera();
       auto ent = SpawnBall(camera->Position, 2.f, 200.f, 0.9f, "Assets/BallTexture.png");
       auto& body = m_Scene.GetCulledScene().get<RigidBodyComponent>(ent);
      if(m_Scene.GetPhysicSystem())
          m_Scene.GetPhysicSystem()->AddRigidBody(body.RigidBody.get());
    }
    if (ImGui::Button("Spawn Cube")) {
        auto& reg = m_Scene.GetCulledScene();
        auto* camera = Application::Get().GetRenderCamera();
        auto ent = SpawnCube(camera->Position, glm::vec3(2.f),100.f, "Assets/BallTexture.png");
        auto& body = m_Scene.GetCulledScene().get<RigidBodyComponent>(ent);
        if(m_Scene.GetPhysicSystem())
            m_Scene.GetPhysicSystem()->AddRigidBody(body.RigidBody.get());
    }
    if (ImGui::Button("Spawn Soft Ball")) {
        auto& reg = m_Scene.GetCulledScene();
        auto* camera = Application::Get().GetRenderCamera();
        auto ent = SpawnSoftBall(camera->Position, 10.f,800.f, "Assets/BallTexture.png");
        auto& body = m_Scene.GetCulledScene().get<SoftBodyComponent>(ent);
        if(m_Scene.GetPhysicSystem())
            m_Scene.GetPhysicSystem()->AddSoftBody(body.SoftBody);
    }
    if (ImGui::Button("Spawn Statue")) {
        auto& reg = m_Scene.GetCulledScene();
        auto* camera = Application::Get().GetRenderCamera();
        auto ent= SpawnStatue(camera->Position, glm::vec3(2.f,6.f,2.f),500.f);
        auto& body = m_Scene.GetCulledScene().get<RigidBodyComponent>(ent);
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
    auto& collision = m_Scene.AddComponent<RigidBodyComponent>(ballEntity,location,glm::vec3(radius),mass,CollisionPrimitive::Sphere);
    m_Scene.AddComponent<MeshComponent>(ballEntity, "Assets/Ball.obj");
    m_Scene.AddComponent<TextureComponent>(ballEntity, texture);
    auto& transform = m_Scene.GetComponent<TransformComponent>(ballEntity);

    transform.Position = glm::vec3(collision.Transform.getOrigin().getX(),collision.Transform.getOrigin().getY(),collision.Transform.getOrigin().getZ());
    transform.Scale = glm::vec3(radius);

    return ballEntity;
}

const entt::entity FLOOF::PhysicsGM::SpawnSoftBall(glm::vec3 location, const float radius, const float mass, const std::string &texture) {
    
    const auto entity = m_Scene.CreateEntity("Softbody");
    auto& textureComponent = m_Scene.AddComponent<TextureComponent>(entity, texture);
    auto& mesh =  m_Scene.AddComponent<MeshComponent>(entity, "Assets/Ball.obj");
    auto& collision =  m_Scene.AddComponent<SoftBodyComponent>(entity);
    auto& transform = m_Scene.GetComponent<TransformComponent>(entity);

    /*
    const btVector3 p(location.x,location.y,location.z);
    const btVector3 s(radius,radius,radius);
    const btVector3 h = s * 0.5;
    const btVector3 c[] = {p + h * btVector3(-1, -1, -1),
                           p + h * btVector3(+1, -1, -1),
                           p + h * btVector3(-1, +1, -1),
                           p + h * btVector3(+1, +1, -1),
                           p + h * btVector3(-1, -1, +1),
                           p + h * btVector3(+1, -1, +1),
                           p + h * btVector3(-1, +1, +1),
                           p + h * btVector3(+1, +1, +1)};
    */
    //btSoftBody* psb = btSoftBodyHelpers::CreateFromConvexHull(*m_Scene.GetPhysicSystem()->getSoftBodyWorldInfo(), c, 8);

    btSoftBody* psb = btSoftBodyHelpers::CreateEllipsoid(*m_Scene.GetPhysicSystem()->getSoftBodyWorldInfo(),btVector3(location.x,location.y,location.z),btVector3(radius,radius,radius),50);
    psb->m_cfg.kVC = 0.5; //Konservation coefficient
    psb->m_materials[0]->m_kLST = 0.5; // linear stiffness

    //soft rigid collision and soft soft collision
    psb->generateBendingConstraints(radius/10.f, psb->m_materials[0]);
    psb->m_cfg.piterations = 2;
    psb->m_cfg.kDF = 1;
    psb->m_cfg.kSSHR_CL = 1;
    psb->m_cfg.kSS_SPLT_CL = 0;
    psb->m_cfg.kSKHR_CL = 0.1f;
    psb->m_cfg.kSK_SPLT_CL = 1;
    psb->m_cfg.collisions = btSoftBody::fCollision::CL_SS +
                            btSoftBody::fCollision::CL_RS;
    psb->randomizeConstraints();
    psb->generateClusters(16);
    psb->setPose(true, false);

    psb->setTotalMass(mass, true);
    collision.SoftBody = psb;

    //collision.CollisionShape = std::make_shared<btSphereShape>(radius);

    //btSoftBodyHelpers::CreateFromTriMesh(*m_Scene.GetPhysicSystem()->getSoftBodyWorldInfo(), data.second.size(),data.second,mesh.Data.IndexCount);

    //transform.Position = glm::vec3(collision.Transform.getOrigin().getX(),collision.Transform.getOrigin().getY(),collision.Transform.getOrigin().getZ());

    transform.Position = location;
    transform.Scale = glm::vec3(radius);

    return entity;
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

const entt::entity FLOOF::PhysicsGM::SpawnStatue(glm::vec3 Location, glm::vec3 Scale, const float mass) {
    const auto entity = m_Scene.CreateEntity("Simulated Statue");
    auto& collision = m_Scene.AddComponent<RigidBodyComponent>(entity,Location,Scale,mass,CollisionPrimitive::Capsule);
   auto& sm = m_Scene.AddComponent<StaticMeshComponent>(entity);
   m_Scene.AddComponent<TextureComponent>(entity, "Assets/BallTexture.png");
   sm.meshes = ModelManager::Get().LoadModelMesh("Assets/statue/source/statue1.fbx").meshes;

    auto& transform = m_Scene.GetComponent<TransformComponent>(entity);

    transform.Position = glm::vec3(collision.Transform.getOrigin().getX(),collision.Transform.getOrigin().getY(),collision.Transform.getOrigin().getZ());
    collision.CollisonVolumeOffset = glm::vec3(0.f,-3.f,0.f);
    transform.Scale = glm::vec3(Scale.x)/150.f;  // mesh is giga

    return entity;



}
