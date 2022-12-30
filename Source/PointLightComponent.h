#pragma once
#include "Math.h"

namespace FLOOF {
    struct PointLightComponent {
        glm::vec4 diffuse = { 1.0f, 0.7f, 0.5f, 0.f };
        float intensity = 10000.f;
        float innerRange = 0.f;
        float outerRange = 256.f;

        struct PointLight {
            glm::vec4 position;
            glm::vec4 diffuse;
            float intensity;
            float innerRange;
            float outerRange;
            float pad = 0.f;
        };
    };
}