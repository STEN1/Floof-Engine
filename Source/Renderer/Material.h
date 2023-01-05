#pragma once
#include "Texture.h"

namespace FLOOF
{
	struct Material
	{
        Texture Diffuse;
        Texture Normals;
        Texture Metallic;
        Texture Roughness;
        Texture AO;
        Texture Opacity;

        std::string Name;

        VkDescriptorSet DescriptorSet = VK_NULL_HANDLE;

        bool HasOpacity = false;
        bool TwoSided = false;

        void UpdateDescriptorSet();

        ~Material();
	};
    struct LandscapeMaterial
    {
        Texture Diffuse;
        Texture Normals;
        Texture Roughness;

        std::string Name;

        VkDescriptorSet DescriptorSet = VK_NULL_HANDLE;

        void UpdateDescriptorSet();

        ~LandscapeMaterial();
    };
}