#pragma once
#include "NativeScript.h"

namespace FLOOF {
    class LightSwarm : public NativeScript {
    public:
        virtual void OnCreate(Scene* scene, entt::entity entity) override;
        virtual void OnUpdate(float deltaTime) override;
    private:
        std::vector<entt::entity> m_Lights;
        glm::vec3 m_Extents;
    };
}