#include "CameraComponent.h"

namespace FLOOF {
    CameraComponent::CameraComponent()
        : CameraComponent(glm::vec3(0.f)) {
    }

    CameraComponent::CameraComponent(glm::vec3 position) : Position{ position } {
        Up = glm::vec3(0.f, -1.f, 0.f);
        Forward = glm::vec3(0.f, 0.f, 1.f);
        Right = glm::normalize(glm::cross(Forward, Up));
    }
#undef near
#undef far
    glm::mat4 CameraComponent::GetVP(float fov, float aspect, float near, float far) {
        glm::mat4 view = GetView();
        glm::mat4 projection = GetPerspective(fov, aspect, near, far);
        return projection * view;
    }

    glm::mat4 CameraComponent::GetView() {
        return glm::lookAt(Position, Position + Forward, Up);
    }

    glm::mat4 CameraComponent::GetPerspective(float fov, float aspect, float near, float far) {
        FOV = fov;
        Aspect = aspect;
        Near = near;
        Far = far;
        return glm::perspective(fov, aspect, near, far);
    }

    void CameraComponent::MoveForward(float amount) {
        Position += Forward * amount;
    }

    void CameraComponent::MoveRight(float amount) {
        glm::vec3 right = glm::normalize(glm::cross(Forward, Up));
        Position += right * amount;
    }

    void CameraComponent::Pitch(float amount) {
        if (amount == 0.f) return;
        glm::vec3 right = glm::normalize(glm::cross(Forward, Up));
        glm::mat4 rotation = glm::rotate(amount, right);
        Forward = glm::normalize(glm::vec3(rotation * glm::vec4(Forward, 1.f)));
        Right = glm::normalize(glm::cross(Forward, Up));
    }

    void CameraComponent::Yaw(float amount) {
        if (amount == 0.f) return;
        glm::mat4 rotation = glm::rotate(-amount, Up);
        Forward = glm::normalize(glm::vec3(rotation * glm::vec4(Forward, 1.f)));
        Right = glm::normalize(glm::cross(Forward, Up));
    }

    void CameraComponent::MoveUp(float amount) {
        if (amount == 0.f) return;
        Position.y += amount;

    }

    void CameraComponent::Lookat(const glm::vec3 eye, const glm::vec3 center) {
        Position = eye;
        Forward = glm::normalize(center - eye);
        Right = glm::normalize(glm::cross(Forward, Up));
    }
}