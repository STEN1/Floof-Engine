#pragma once
#include "Math.h"

namespace FLOOF {
    struct PointLightComponent {
        glm::vec4 diffuse = { 1.0f, 0.7f, 0.5f, 0.f };
        glm::vec4 ambient = diffuse * 0.1f;

        float lightRange = 50.f;

        struct PointLight {
            glm::vec4 position;
            glm::vec4 diffuse;
            glm::vec4 ambient;
            float linear;
            float quadratic;
            float lightRange;
            float pad;
        };
    };
}