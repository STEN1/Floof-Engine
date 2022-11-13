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

        std::string Name;

        VkDescriptorSet DescriptorSet = VK_NULL_HANDLE;

        void UpdateDescriptorSet();

        ~Material();
	};
}