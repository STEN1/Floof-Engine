#include "LightSwarm.h"
#include "../Components.h"
#include "../Math.h"
#include "../Utils.h"

namespace FLOOF {
    void LightSwarm::OnCreate(Scene* scene, entt::entity entity)
    {
        NativeScript::OnCreate(scene, entity);

        m_Extents = glm::vec3(1200.f, 200.f, 1200.f);
        uint32_t numLights = 128;
        m_LightCount = numLights;

        for (uint32_t i = 0; i < numLights; i++) {
            const auto lightEntity = CreateEntity("Light", entity);
            auto& light = AddComponent<PointLightComponent>(lightEntity);
            auto& mesh = AddComponent<StaticMeshComponent>(lightEntity, "Assets/Ball.obj");
            mesh.meshes[0].MeshMaterial.Diffuse = Texture(TextureColor::White);
            mesh.meshes[0].MeshMaterial.Roughness = Texture(TextureColor::DarkGrey);
            mesh.meshes[0].MeshMaterial.Metallic = Texture(TextureColor::White);
            mesh.meshes[0].MeshMaterial.UpdateDescriptorSet();
            light.diffuse = glm::vec4(Utils::ColorFromScalar(Math::RandFloat(0.f, 1000.f)), 1.0);
            glm::vec3 pos;
            pos.x = Math::RandFloat(-m_Extents.x, m_Extents.x);
            pos.y = Math::RandFloat(-m_Extents.y, m_Extents.y);
            pos.z = Math::RandFloat(-m_Extents.z, m_Extents.z);
            auto& transform = GetComponent<TransformComponent>(lightEntity);
            transform.Scale = glm::vec3(light.outerRange * 0.01f);
            transform.Position = pos;
            m_Lights.push_back(lightEntity);
        }
    }
    void LightSwarm::OnUpdate(float deltaTime)
    {
        NativeScript::OnUpdate(deltaTime);

        static constexpr float baseSpeed = 10.f;
        float speed = baseSpeed;
        float speedModifier = 100.f / m_LightCount;

        for (entt::entity entity : m_Lights) {
            auto& transform = GetComponent<TransformComponent>(entity);
            speed += speedModifier;
            transform.Position.y += speed * deltaTime;
            if (transform.Position.y > m_Extents.y) {
                transform.Position.y = -m_Extents.y;
                transform.Position.x = Math::RandFloat(-m_Extents.x, m_Extents.x);
                transform.Position.z = Math::RandFloat(-m_Extents.z, m_Extents.z);
            }
        }
    }
}