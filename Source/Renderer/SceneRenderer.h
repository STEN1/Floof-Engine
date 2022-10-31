#pragma once

#include "../Floof.h"
#include "VulkanRenderer.h"
#include <entt/entt.hpp>
#include "../Math.h"

namespace FLOOF {
  
    enum class SceneRendererType
    {
        Forward, Deferred, Size
    };

    static const char* SceneRendererTypeStrings[] =
    {
        "Forward", "Deferred"
    };

    class SceneRenderer {      
    public:
        SceneRenderer() = default;
        virtual ~SceneRenderer() = default;
        virtual VkDescriptorSet RenderToTexture(entt::registry& scene, glm::vec2 extent) = 0;
    };
}