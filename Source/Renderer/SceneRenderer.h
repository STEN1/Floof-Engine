#pragma once

#include "../Floof.h"
#include "VulkanRenderer.h"
#include <entt/entt.hpp>

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
        virtual void Render(entt::registry& scene) = 0;
    };
}