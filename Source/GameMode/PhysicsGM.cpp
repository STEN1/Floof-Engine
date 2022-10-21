#include "PhysicsGM.h"
#include "../Components.h"
#include "../Application.h"

#include "btBulletDynamicsCommon.h"

void FLOOF::PhysicsGM::OnCreate()
{
    int height = 10;
    int width = 10;
    float spacing = 5.f;

    for (size_t y = 0; y < height; y++)
    {
        for (int x = 0; x < width; x++)
        {
            SpawnBall(glm::vec3(x * spacing - (float(width) * spacing * 0.5f), y * spacing, 0.f), 2.f, 200.f, 0.9f, "Assets/BallTexture.png");
        }
    }

    auto* collisionConfiguration = new btDefaultCollisionConfiguration();
    auto* dispatcher = new btCollisionDispatcher(collisionConfiguration);
    auto* overlappingPairCache = new btDbvtBroadphase();
    auto* solver = new btSequentialImpulseConstraintSolver;
    auto* dynamicsWorld = new btDiscreteDynamicsWorld(dispatcher,overlappingPairCache,solver,collisionConfiguration);

    dynamicsWorld->setGravity(btVector3(0,-9.81,0));


}

void FLOOF::PhysicsGM::OnUpdateEditor(float deltaTime)
{
    ImGui::Begin("Physics");
    if (ImGui::Button("Spawn ball")) {
        auto& reg = m_Scene.GetCulledScene();
        auto* camera = Application::Get().GetRenderCamera();
        SpawnBall(camera->Position, 2.f, 200.f, 0.9f, "Assets/BallTexture.png");
    }
    ImGui::End();
}

const void FLOOF::PhysicsGM::SpawnBall(glm::vec3 location, const float radius, const float mass, const float elasticity, const std::string& texture)
{
    auto& m_Registry = m_Scene.GetCulledScene();

    const auto ballEntity = m_Registry.create();
    auto& transform = m_Registry.emplace<TransformComponent>(ballEntity);
    auto& ball = m_Registry.emplace<BallComponent>(ballEntity);
    auto& time = m_Registry.emplace<TimeComponent>(ballEntity);
    auto& spline = m_Registry.emplace<BSplineComponent>(ballEntity);

    ball.Radius = radius;
    ball.Mass = mass;
    ball.Elasticity = elasticity;

    auto& velocity = m_Registry.emplace<VelocityComponent>(ballEntity);
    m_Registry.emplace<MeshComponent>(ballEntity, "Assets/Ball.obj");
    m_Registry.emplace<TextureComponent>(ballEntity, texture);

    transform.Position = location;
    transform.Scale = glm::vec3(ball.Radius);

    ball.CollisionSphere.radius = ball.Radius;
    ball.CollisionSphere.pos = transform.Position;
}
