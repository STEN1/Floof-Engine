#include "LightSwarm.h"
#include "../Components.h"
#include "../Math.h"
#include "../Utils.h"

namespace FLOOF {
    void LightSwarm::OnCreate(Scene* scene, entt::entity entity)
    {
        NativeScript::OnCreate(scene, entity);

        m_Extents = glm::vec3(1200.f, 400.f, 300.f);
        uint32_t numLights = 32;

        for (uint32_t i = 0; i < numLights; i++) {
            const auto lightEntity = CreateEntity("Light", entity);
            auto& light = AddComponent<PointLightComponent>(lightEntity);
            light.diffuse = glm::vec4(Utils::ColorFromScalar(Math::RandFloat(0.f, 1000.f)), 1.0);
            light.lightRange = 100000.f;
            glm::vec3 pos;
            pos.x = Math::RandFloat(-m_Extents.x, m_Extents.x);
            pos.y = Math::RandFloat(-m_Extents.y, m_Extents.y);
            pos.z = Math::RandFloat(-m_Extents.z, m_Extents.z);
            auto& transform = GetComponent<TransformComponent>(lightEntity);
            transform.Position = pos;
            m_Lights.push_back(lightEntity);
        }
    }
    void LightSwarm::OnUpdate(float deltaTime)
    {
        NativeScript::OnUpdate(deltaTime);

        static constexpr float speed = 100.f;

        for (entt::entity entity : m_Lights) {
            auto& transform = GetComponent<TransformComponent>(entity);
            transform.Position.y += speed * deltaTime;
            if (transform.Position.y > m_Extents.y) {
                transform.Position.y = -m_Extents.y;
                transform.Position.x = Math::RandFloat(-m_Extents.x, m_Extents.x);
                transform.Position.z = Math::RandFloat(-m_Extents.z, m_Extents.z);
            }
        }
    }
}