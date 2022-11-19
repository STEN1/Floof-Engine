#pragma once
#include <array>
#include <string>
#include "VulkanRenderer.h"

namespace FLOOF {
    class Cubemap {
    public:
        Cubemap(const std::array<std::string, 6>& paths);
        Cubemap(const std::string& equirectangularMap);
        ~Cubemap();
        VulkanTexture CubemapTexture{};
        VkSampler m_Sampler = VK_NULL_HANDLE;
    };
}