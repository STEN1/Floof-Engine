#include "PhysicsGM.h"
#include "../Components.h"
#include "../Application.h"
#include "btBulletDynamicsCommon.h"

void FLOOF::PhysicsGM::OnCreate()
{
    int height = 5;
    int width = 5;
    float spacing = 4.f;

    for (size_t y = 10; y < height+10; y++)
    {
        for (int x = 0; x < width; x++)
        {
            for(int z = 0; z < width; z++){
                SpawnBall(glm::vec3(x * spacing - (float(width) * spacing * 0.5f), y * spacing, z * spacing - (float(width) * spacing * 0.5f)), spacing/2.f, 200.f, 0.9f,"Assets/BallTexture.png");
                }
            }

    }

    SpawnBall(glm::vec3(0.f,-150.f,0.f), 75.f, 0.f, 0.9f,"Assets/LightBlue.png");

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
    ImGui::End();

}

const entt::entity FLOOF::PhysicsGM::SpawnBall(glm::vec3 location, const float radius, const float mass, const float elasticity, const std::string& texture)
{
    auto& m_Registry = m_Scene.GetCulledScene();

    const auto ballEntity = m_Registry.create();
    auto& transform = m_Registry.emplace<TransformComponent>(ballEntity);
    auto& ball = m_Registry.emplace<BallComponent>(ballEntity);
    auto& time = m_Registry.emplace<TimeComponent>(ballEntity);
    auto& spline = m_Registry.emplace<BSplineComponent>(ballEntity);
    auto& collision = m_Registry.emplace<RigidBodyComponent>(ballEntity);

    collision.CollisionShape = std::make_shared<btSphereShape>(radius);
    collision.Transform.setIdentity();
    collision.Transform.setOrigin(btVector3(location.x,location.y,location.z));

    collision.DefaultMotionState = std::make_shared<btDefaultMotionState>(collision.Transform);
    btRigidBody::btRigidBodyConstructionInfo rbInfo(mass, collision.DefaultMotionState.get(), collision.CollisionShape.get());
    collision.RigidBody = std::make_shared<btRigidBody>(rbInfo);

    ball.Radius = radius;
    ball.Mass = mass;
    ball.Elasticity = elasticity;

    auto& velocity = m_Registry.emplace<VelocityComponent>(ballEntity);
    m_Registry.emplace<MeshComponent>(ballEntity, "Assets/Ball.obj");
    m_Registry.emplace<TextureComponent>(ballEntity, texture);

    transform.Position = glm::vec3(collision.Transform.getOrigin().getX(),collision.Transform.getOrigin().getY(),collision.Transform.getOrigin().getZ());
    transform.Scale = glm::vec3(ball.Radius);

    ball.CollisionSphere.radius = ball.Radius;
    ball.CollisionSphere.pos = transform.Position;

    return ballEntity;

}
