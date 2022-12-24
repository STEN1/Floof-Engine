#include "Material.h"

namespace FLOOF {
    void Material::UpdateDescriptorSet()
    {
        auto* renderer = VulkanRenderer::Get();
       if(!renderer)
           return;
        VkSampler sampler = renderer->GetTextureSampler();

        if (!Diffuse.IsValid())
            Diffuse = Texture(TextureColor::White);

        if (!Normals.IsValid())
            Normals = Texture(TextureColor::FlatNormal);

        if (!Metallic.IsValid())
            Metallic = Texture(TextureColor::Black);

        if (!Roughness.IsValid())
            Roughness = Texture(TextureColor::White);

        if (!AO.IsValid())
            AO = Texture(TextureColor::White);

        if (!Opacity.IsValid())
            Opacity = Texture(TextureColor::White);
        else if (Opacity.IsValid() && Opacity.Color == TextureColor::None)
            HasOpacity = true;

        std::vector<VkDescriptorImageInfo> imageInfos{
            { sampler, Diffuse.VkTexture.ImageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL },
            { sampler, Normals.VkTexture.ImageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL },
            { sampler, Metallic.VkTexture.ImageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL },
            { sampler, Roughness.VkTexture.ImageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL },
            { sampler, AO.VkTexture.ImageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL },
            { sampler, Opacity.VkTexture.ImageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL },
        };

        if (DescriptorSet == VK_NULL_HANDLE) {
            renderer->FinishAllFrames();
            DescriptorSet = renderer->AllocateMaterialDescriptorSet(renderer->GetDescriptorSetLayout(RenderSetLayouts::Material));
        }

        std::array<VkWriteDescriptorSet, 6> writeDescriptorSets{};
        for (size_t i = 0; i < imageInfos.size(); i++) {
            writeDescriptorSets[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            writeDescriptorSets[i].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            writeDescriptorSets[i].descriptorCount = 1;
            writeDescriptorSets[i].dstSet = DescriptorSet;
            writeDescriptorSets[i].dstBinding = static_cast<uint32_t>(i);
            writeDescriptorSets[i].pImageInfo = &imageInfos[i];
        }

        renderer->FinishAllFrames();
        vkUpdateDescriptorSets(renderer->GetDevice(),
            static_cast<uint32_t>(writeDescriptorSets.size()), writeDescriptorSets.data(), 0, nullptr);
    }

    Material::~Material() {
        auto* renderer = VulkanRenderer::Get();
        if (DescriptorSet != VK_NULL_HANDLE) {
            renderer->FinishAllFrames();
            renderer->FreeMaterialDescriptorSet(DescriptorSet);
        }
    }
    void LandscapeMaterial::UpdateDescriptorSet()
    {
        auto* renderer = VulkanRenderer::Get();

        VkSampler sampler = renderer->GetTextureSampler();

        if (!Diffuse.IsValid())
            Diffuse = Texture(TextureColor::White);

        if (!Normals.IsValid())
            Normals = Texture(TextureColor::FlatNormal);

        if (!Roughness.IsValid())
            Roughness = Texture(TextureColor::White);

        std::vector<VkDescriptorImageInfo> imageInfos{
            { sampler, Diffuse.VkTexture.ImageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL },
            { sampler, Normals.VkTexture.ImageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL },
            { sampler, Roughness.VkTexture.ImageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL },
        };

        if (DescriptorSet == VK_NULL_HANDLE) {
            renderer->FinishAllFrames();
            DescriptorSet = renderer->AllocateLandscapeMaterialDescriptorSet(renderer->GetDescriptorSetLayout(RenderSetLayouts::LandscapeMaterial));
        }

        std::array<VkWriteDescriptorSet, 3> writeDescriptorSets{};
        for (size_t i = 0; i < imageInfos.size(); i++) {
            writeDescriptorSets[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            writeDescriptorSets[i].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            writeDescriptorSets[i].descriptorCount = 1;
            writeDescriptorSets[i].dstSet = DescriptorSet;
            writeDescriptorSets[i].dstBinding = static_cast<uint32_t>(i);
            writeDescriptorSets[i].pImageInfo = &imageInfos[i];
        }

        renderer->FinishAllFrames();
        vkUpdateDescriptorSets(renderer->GetDevice(),
            static_cast<uint32_t>(writeDescriptorSets.size()), writeDescriptorSets.data(), 0, nullptr);
    }
    LandscapeMaterial::~LandscapeMaterial()
    {
        auto* renderer = VulkanRenderer::Get();
        if (DescriptorSet != VK_NULL_HANDLE) {
            renderer->FinishAllFrames();
            renderer->FreeLandscapeMaterialDescriptorSet(DescriptorSet);
        }
    }
}