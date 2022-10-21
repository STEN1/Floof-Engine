#pragma once

#include "../Floof.h"
#include "VulkanRenderer.h"

namespace FLOOF {

    /// <summary>
    /// Each new definition of texture type below,
    /// will add another texture to the material struct
    /// </summary>
    enum TextureType
    {
        Invalid = -1,
        Ao, //Ambient Occlusion
        Albedo,
        Diffuse = Albedo, //Either albedo or diffuse
        Specular,
        Roughness,
        Metallic,     
        Heightmap,
        Normals = Heightmap, //Usually Assimp confuses normals and heightmaps
        Lightmap,
        Size
    };

    struct Texture 
    {
        Texture() = default;

        Texture(VkImage vkImage, TextureType type)
        {
            SetVkTexture(vkImage, type);
        }

        /// <summary>
        /// Checks if the texture is valid
        /// </summary>
        bool IsValid() const
        {
            return vkImage != VK_NULL_HANDLE;
        }

        void SetVkTexture(VkImage vkImage, TextureType type)
        {
            this->vkImage = vkImage;
            this->type = type;
        }

        VkImage GetVkImage() const
        {
            return vkImage;
        }

        TextureType GetTextureType() const
        {
            return type;
        }

    private:
        VkImage vkImage = VK_NULL_HANDLE;
        TextureType type{ TextureType::Invalid };
        int width{ 0 }, height{ 0 };
    };
}