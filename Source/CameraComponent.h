#pragma once
#include "Math.h"

namespace FLOOF {
    struct CameraComponent {
        CameraComponent();
        CameraComponent(glm::vec3 position);

        glm::mat4 GetVP(float fov, float aspect, float near, float far);
        glm::mat4 GetView();
        glm::mat4 GetPerspective(float fov, float aspect, float near, float far);

        void Lookat(const glm::vec3 eye, const glm::vec3 center);

        void MoveForward(float amount);

        void MoveRight(float amount);

        void MoveUp(float amount);

        void Pitch(float amount);

        void Yaw(float amount);

        glm::vec3 Position;
        glm::vec3 Forward;
        glm::vec3 Up;
        glm::vec3 Right;
        float FOV = 1.f;
        float Near = 0.1f;
        float Far = 100.f;
        float Aspect = 16.f / 9.f;
    };
}