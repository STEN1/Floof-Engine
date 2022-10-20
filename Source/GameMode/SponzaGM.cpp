#include "SponzaGM.h"
#include "../Components.h"

void FLOOF::SponzaGM::OnCreate()
{
    int height = 20;
    int width = 20;
    float spacing = 10.f;

    for (size_t y = 0; y < height; y++)
    {
        for (int x = 0; x < width; x++)
        {
            SpawnBall(glm::vec3(x * spacing - (float(width) * spacing * 0.5f), y * spacing, 0.f), 2.f, 200.f, 0.9f, "Assets/BallTexture.png");
        }
    }
}

void FLOOF::SponzaGM::OnUpdateEditor(float deltaTime)
{
}

const void FLOOF::SponzaGM::SpawnBall(glm::vec3 location, const float radius, const float mass, const float elasticity, const std::string& texture)
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
