#pragma once

#include "../Floof.h"
#include "VulkanRenderer.h"
#include "TextureManager.h"

#include <string>

namespace FLOOF {
    struct Texture {
        Texture() = default;
        Texture(const std::string& path, bool flip = false, bool glNormal = false);
        Texture(TextureColor color);

        bool IsValid() { return VkTexture.DesctriptorSet != VK_NULL_HANDLE; }

        VulkanTexture VkTexture;
        std::string Path;
        TextureColor Color = TextureColor::None;
    };
}