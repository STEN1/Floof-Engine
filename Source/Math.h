#pragma once

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include "glm/glm.hpp"
#include "glm/gtx/transform.hpp"
#include "glm/gtx/quaternion.hpp"
#include <random>

namespace FLOOF {
    class Math {
    public:
        Math() = delete;
        inline static constexpr double Gravity{ 9.807 };
        inline static constexpr glm::vec3 GravitationalPull = glm::vec3(0.f, -Gravity, 0.f);

        static size_t Cantor(size_t a, size_t b) { return (a + b + 1) * (a + b) / 2 + b; }

        static double RandDouble(double min, double max) {
            std::uniform_real_distribution<>dist(min, max);
            return dist(s_Generator);
        }
        static float RandFloat(float min, float max) {
            std::uniform_real_distribution<>dist(min, max);
            return static_cast<float>(dist(s_Generator));
        }
        static int RandInt(int min, int max) {
            std::uniform_int_distribution<> dist(min, max);
            return dist(s_Generator);
        }

        static glm::vec3 GetSafeNormal() {
            return glm::normalize(glm::vec3(RandFloat(0.1f, 1.f), RandFloat(0.1f, 1.f), RandFloat(0.1f, 1.f)));
        };

    private:
        inline static std::random_device s_RandomDevice;
        inline static std::mt19937_64 s_Generator = std::mt19937_64(s_RandomDevice());
    };
}
