#include "SponzaGM.h"
#include "../Components.h"
#include "../Renderer/ModelManager.h"

void FLOOF::SponzaGM::OnCreate()
{
}

void FLOOF::SponzaGM::OnUpdateEditor(float deltaTime)
{
}

const void FLOOF::SponzaGM::SpawnBall(glm::vec3 location, const float radius, const float mass, const float elasticity, const std::string& texture)
{
    auto& m_Registry = m_Scene.GetCulledScene();

    const auto ballEntity = m_Registry.create();
    auto& transform = m_Registry.emplace<TransformComponent>(ballEntity);
    auto& spline = m_Registry.emplace<BSplineComponent>(ballEntity);

    m_Registry.emplace<MeshComponent>(ballEntity, "Assets/Ball.obj");
    m_Registry.emplace<TextureComponent>(ballEntity, texture);

    transform.Position = location;

}
