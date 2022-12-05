#include "LightSwarm.h"
#include "../Components.h"
#include "../Math.h"
#include "../Utils.h"

namespace FLOOF {
    void LightSwarm::OnCreate(Scene* scene, entt::entity entity)
    {
        NativeScript::OnCreate(scene, entity);

        glm::vec3 extents(200.f, 200.f, 200.f);
        uint32_t numLights = 50;

        for (uint32_t i = 0; i < numLights; i++) {
            const auto lightEntity = CreateEntity("Light", entity);
            auto& light = AddComponent<PointLightComponent>(lightEntity);
            light.diffuse = glm::vec4(Utils::ColorFromScalar(Math::RandFloat(0.f, 1000.f)), 1.0);
            glm::vec3 pos;
            pos.x = Math::RandFloat(-extents.x, extents.x);
            pos.y = Math::RandFloat(-extents.y, extents.y);
            pos.z = Math::RandFloat(-extents.z, extents.z);
            auto& transform = GetComponent<TransformComponent>(lightEntity);
            transform.Position = pos;
        }
    }
    void LightSwarm::OnUpdate(float deltaTime)
    {
        NativeScript::OnUpdate(deltaTime);
    }
}