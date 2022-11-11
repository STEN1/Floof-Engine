#pragma once

#include "../Floof.h"
#include "VulkanRenderer.h"
#include <entt/entt.hpp>
#include "../Math.h"
#include "../Scene.h"

namespace FLOOF {
    class SceneRenderer {      
    public:
        SceneRenderer() = default;
        virtual ~SceneRenderer() = default;
        virtual VkDescriptorSet RenderToTexture(std::shared_ptr<Scene> scene, glm::vec2 extent) = 0;
    };
}