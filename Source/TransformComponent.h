#pragma once
#include "Math.h"

namespace FLOOF {
    struct TransformComponent {
        inline static constexpr bool in_place_delete = true;

        TransformComponent* Parent = nullptr;

        glm::vec3 Position = glm::vec3(0.f);
        glm::vec3 Rotation = glm::vec3(0.f);
        glm::vec3 Scale = glm::vec3(1.f);

        glm::mat4 GetLocalTransform() const {
            return glm::translate(Position)
                * glm::toMat4(glm::quat(Rotation))
                * glm::scale(Scale);
        }

        glm::mat4 GetTransform() const {
            TransformComponent* parent = Parent;

            glm::mat4 transform = GetLocalTransform();

            while (parent) {
                transform = parent->GetLocalTransform() * transform;
                parent = parent->Parent;
            }
            return transform;
        }

        glm::vec3 GetWorldPosition() const {
            glm::mat4 transform = GetTransform();
            glm::vec3 pos(transform[3].x, transform[3].y, transform[3].z);
            return pos;
        }
    };
}