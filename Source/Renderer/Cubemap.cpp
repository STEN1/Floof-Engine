#include "Cubemap.h"
#include "stb_image/stb_image.h"
#include <vector>
#include "../Floof.h"

namespace FLOOF {

    Cubemap::Cubemap(const std::array<std::string, 6>& paths)
    {
        auto renderer = VulkanRenderer::Get();
        // Load texture
        stbi_set_flip_vertically_on_load(false);

        uint32_t offsets[6];
        uint32_t currentOffset{};
        uint32_t correctSize{};
        int imageWidth, imageHeight;
        std::vector<std::vector<stbi_uc>> combinedData(6);
        for (uint32_t i = 0; i < 6; i++) {
            int xWidth, yHeight, channels;
            auto* data = stbi_load(paths[i].c_str(), &xWidth, &yHeight, &channels, 0);
            uint32_t size = xWidth * yHeight * 4;

            if (i == 0) {
                imageWidth = xWidth;
                imageHeight = yHeight;
                correctSize = size;
            } else {
                // assert that textures are the same dimetions.
                ASSERT(size == correctSize);
            }

            offsets[i] = currentOffset;
            currentOffset += size;

            combinedData[i].resize(size);
            if (channels == 4) {
                memcpy(combinedData[i].data(), data, size);
            } else if (channels == 3) {
                for (uint32_t h = 0; h < yHeight; h++) {
                    for (uint32_t w = 0; w < xWidth; w++) {
                        uint32_t readyDataIndex = (h * xWidth * 4) + (w * 4);
                        auto& rdR = combinedData[i][readyDataIndex];
                        auto& rdG = combinedData[i][readyDataIndex + 1];
                        auto& rdB = combinedData[i][readyDataIndex + 2];
                        auto& rdA = combinedData[i][readyDataIndex + 3];

                        uint32_t dataIndex = (h * xWidth * 3) + (w * 3);
                        auto& dR = data[dataIndex];
                        auto& dG = data[dataIndex + 1];
                        auto& dB = data[dataIndex + 2];

                        rdR = dR; rdG = dG; rdB = dB;
                        rdA = (stbi_uc)255;
                    }
                }
            } else if (channels == 1) {
                for (uint32_t h = 0; h < yHeight; h++) {
                    for (uint32_t w = 0; w < xWidth; w++) {
                        uint32_t readyDataIndex = (h * xWidth * 4) + (w * 4);
                        auto& rdR = combinedData[i][readyDataIndex];
                        auto& rdG = combinedData[i][readyDataIndex + 1];
                        auto& rdB = combinedData[i][readyDataIndex + 2];
                        auto& rdA = combinedData[i][readyDataIndex + 3];

                        uint32_t dataIndex = (h * xWidth * 1) + (w * 1);
                        auto& dR = data[dataIndex];

                        rdR = dR;
                        rdG = (stbi_uc)0;
                        rdB = (stbi_uc)0;
                        rdA = (stbi_uc)255;
                    }
                }
            } else {
                ASSERT(false);
            }
            stbi_image_free(data);
        }

        std::vector<stbi_uc> readyData;
        for (auto& data : combinedData) {
            readyData.insert(readyData.begin(), data.begin(), data.end());
        }

        VkFormat format = VK_FORMAT_R8G8B8A8_UNORM;

        // staging buffer
        VkBufferCreateInfo stagingCreateInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
        stagingCreateInfo.size = readyData.size() * sizeof(stbi_uc);
        stagingCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

        VmaAllocationCreateInfo stagingBufAllocCreateInfo = {};
        stagingBufAllocCreateInfo.usage = VMA_MEMORY_USAGE_AUTO;
        stagingBufAllocCreateInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
            VMA_ALLOCATION_CREATE_MAPPED_BIT;

        VkBuffer stagingBuffer{};
        VmaAllocation stagingBufferAlloc{};
        VmaAllocationInfo stagingBufferAllocInfo{};
        vmaCreateBuffer(renderer->GetAllocator(), &stagingCreateInfo, &stagingBufAllocCreateInfo, &stagingBuffer,
            &stagingBufferAlloc, &stagingBufferAllocInfo);

        ASSERT(stagingBufferAllocInfo.pMappedData != nullptr);
        memcpy(stagingBufferAllocInfo.pMappedData, readyData.data(), readyData.size() * sizeof(stbi_uc));


        // Image
        VkImageCreateInfo imageInfo = { VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.extent.width = imageWidth;
        imageInfo.extent.height = imageHeight;
        imageInfo.extent.depth = 1;
        imageInfo.mipLevels = 1;
        imageInfo.arrayLayers = 6;
        imageInfo.format = format;
        imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;

        VmaAllocationCreateInfo imageAllocCreateInfo = {};
        imageAllocCreateInfo.usage = VMA_MEMORY_USAGE_AUTO;

        vmaCreateImage(renderer->GetAllocator(), &imageInfo, &imageAllocCreateInfo, &CubemapTexture.Image,
            &CubemapTexture.Allocation, &CubemapTexture.AllocationInfo);

        std::vector<VkBufferImageCopy> bufferCopyRegions;

        for (uint32_t face = 0; face < 6; face++)
        {
            VkBufferImageCopy bufferCopyRegion = {};
            bufferCopyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            bufferCopyRegion.imageSubresource.mipLevel = 0;
            bufferCopyRegion.imageSubresource.baseArrayLayer = face;
            bufferCopyRegion.imageSubresource.layerCount = 1;
            bufferCopyRegion.imageExtent.width = imageWidth;
            bufferCopyRegion.imageExtent.height = imageHeight;
            bufferCopyRegion.imageExtent.depth = 1;
            bufferCopyRegion.bufferOffset = offsets[face];
            bufferCopyRegions.push_back(bufferCopyRegion);
        }

        auto commandBuffer = renderer->BeginSingleUseCommandBuffer();

        VkImageMemoryBarrier imgMemBarrier = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
        imgMemBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        imgMemBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        imgMemBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        imgMemBarrier.subresourceRange.baseMipLevel = 0;
        imgMemBarrier.subresourceRange.levelCount = 1;
        imgMemBarrier.subresourceRange.baseArrayLayer = 0;
        imgMemBarrier.subresourceRange.layerCount = 6;
        imgMemBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imgMemBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        imgMemBarrier.image = CubemapTexture.Image;
        imgMemBarrier.srcAccessMask = 0;
        imgMemBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        vkCmdPipelineBarrier(
            commandBuffer,
            VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            0,
            0, nullptr,
            0, nullptr,
            1, &imgMemBarrier);

        vkCmdCopyBufferToImage(commandBuffer, stagingBuffer, CubemapTexture.Image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            bufferCopyRegions.size(), bufferCopyRegions.data());

        imgMemBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        imgMemBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imgMemBarrier.image = CubemapTexture.Image;
        imgMemBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        imgMemBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        vkCmdPipelineBarrier(
            commandBuffer,
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
            0,
            0, nullptr,
            0, nullptr,
            1, &imgMemBarrier);

        renderer->EndSingleUseCommandBuffer(commandBuffer);

        VkSamplerCreateInfo samplerCreateInfo = { VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO };
        samplerCreateInfo.magFilter = VK_FILTER_LINEAR;
        samplerCreateInfo.minFilter = VK_FILTER_LINEAR;
        samplerCreateInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        samplerCreateInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerCreateInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerCreateInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerCreateInfo.mipLodBias = 0.0f;
        samplerCreateInfo.compareOp = VK_COMPARE_OP_NEVER;
        samplerCreateInfo.minLod = 0.0f;
        samplerCreateInfo.maxLod = 1.f;
        samplerCreateInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
        samplerCreateInfo.maxAnisotropy = 1.0f;

        VkResult result = vkCreateSampler(renderer->GetDevice(), &samplerCreateInfo, nullptr, &m_Sampler);
        ASSERT(result == VK_SUCCESS);

        VkImageViewCreateInfo imageViewCreateInfo = { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
        imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
        imageViewCreateInfo.format = format;
        imageViewCreateInfo.components = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A };
        imageViewCreateInfo.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
        // 6 array layers (faces)
        imageViewCreateInfo.subresourceRange.layerCount = 6;
        // Set number of mip levels
        imageViewCreateInfo.subresourceRange.levelCount = 1;
        imageViewCreateInfo.image = CubemapTexture.Image;

        result = vkCreateImageView(renderer->GetDevice(), &imageViewCreateInfo, nullptr, &CubemapTexture.ImageView);
        ASSERT(result == VK_SUCCESS);

        VkDescriptorImageInfo descriptorImageInfo{};
        descriptorImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        descriptorImageInfo.imageView = CubemapTexture.ImageView;
        descriptorImageInfo.sampler = m_Sampler;

        CubemapTexture.DesctriptorSet = renderer->AllocateTextureDescriptorSet(renderer->GetDescriptorSetLayout(RenderSetLayouts::Skybox));

        VkWriteDescriptorSet writeSet = { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
        writeSet.pImageInfo = &descriptorImageInfo;
        writeSet.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        writeSet.dstSet = CubemapTexture.DesctriptorSet;
        writeSet.dstBinding = 0;
        writeSet.descriptorCount = 1;

        renderer->FinishAllFrames();
        vkUpdateDescriptorSets(renderer->GetDevice(), 1, &writeSet, 0, nullptr);

        vmaDestroyBuffer(renderer->GetAllocator(), stagingBuffer, stagingBufferAlloc);
    }
    Cubemap::~Cubemap()
    {
        auto* renderer = VulkanRenderer::Get();
        renderer->FinishAllFrames();
        vkDestroyImageView(renderer->GetDevice(), CubemapTexture.ImageView, nullptr);
        vmaDestroyImage(renderer->GetAllocator(), CubemapTexture.Image, CubemapTexture.Allocation);
        vkDestroySampler(renderer->GetDevice(), m_Sampler, nullptr);
        renderer->FreeTextureDescriptorSet(CubemapTexture.DesctriptorSet);
    }
}