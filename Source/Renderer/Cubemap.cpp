#include "Cubemap.h"
#include "stb_image/stb_image.h"
#include <vector>
#include "../Floof.h"
#include "Framebuffer.h"
#include "ModelManager.h"
#include "VulkanBuffer.h"

namespace FLOOF {

    Cubemap::Cubemap(const std::array<std::string, 6>& paths)
    {
        Format = VK_FORMAT_R8G8B8A8_UNORM;
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
        imageInfo.format = Format;
        imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
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

        VkImageViewCreateInfo imageViewCreateInfo = { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
        imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
        imageViewCreateInfo.format = Format;
        imageViewCreateInfo.components = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A };
        imageViewCreateInfo.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
        // 6 array layers (faces)
        imageViewCreateInfo.subresourceRange.layerCount = 6;
        // Set number of mip levels
        imageViewCreateInfo.subresourceRange.levelCount = 1;
        imageViewCreateInfo.image = CubemapTexture.Image;

        VkResult result = vkCreateImageView(renderer->GetDevice(), &imageViewCreateInfo, nullptr, &CubemapTexture.ImageView);
        ASSERT(result == VK_SUCCESS);

        auto sampler = renderer->GetTextureSamplerClamped();

        VkDescriptorImageInfo descriptorImageInfo{};
        descriptorImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        descriptorImageInfo.imageView = CubemapTexture.ImageView;
        descriptorImageInfo.sampler = sampler;

        CubemapTexture.DesctriptorSet = renderer->AllocateTextureDescriptorSet(renderer->GetDescriptorSetLayout(RenderSetLayouts::DiffuseTextureClamped));

        VkWriteDescriptorSet writeSet = { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
        writeSet.pImageInfo = &descriptorImageInfo;
        writeSet.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        writeSet.dstSet = CubemapTexture.DesctriptorSet;
        writeSet.dstBinding = 0;
        writeSet.descriptorCount = 1;

        vkUpdateDescriptorSets(renderer->GetDevice(), 1, &writeSet, 0, nullptr);

        vmaDestroyBuffer(renderer->GetAllocator(), stagingBuffer, stagingBufferAlloc);
    }
    Cubemap::Cubemap(const std::string& equirectangularMap)
    {
        auto* renderer = VulkanRenderer::Get();

        stbi_set_flip_vertically_on_load(true);
        int width, height, channels;
        auto* data = stbi_loadf(equirectangularMap.c_str(), &width, &height, &channels, 0);
        ASSERT(data != nullptr);

        auto size = width * height * 4 * sizeof(float);

        // staging buffer
        VkBufferCreateInfo stagingCreateInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
        stagingCreateInfo.size = size;
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
        if (channels == 4) {
            memcpy(stagingBufferAllocInfo.pMappedData, data, size);
        } else if (channels == 3) {
            std::vector<float> readyData(size);
            for (uint32_t h = 0; h < height; h++) {
                for (uint32_t w = 0; w < width; w++) {
                    uint32_t readyDataIndex = (h * width * 4) + (w * 4);
                    auto& rdR = readyData[readyDataIndex];
                    auto& rdG = readyData[readyDataIndex + 1];
                    auto& rdB = readyData[readyDataIndex + 2];
                    auto& rdA = readyData[readyDataIndex + 3];

                    uint32_t dataIndex = (h * width * 3) + (w * 3);
                    auto& dR = data[dataIndex];
                    auto& dG = data[dataIndex + 1];
                    auto& dB = data[dataIndex + 2];

                    rdR = dR; rdG = dG; rdB = dB;
                    rdA = 1.f;
                }
            }
            memcpy(stagingBufferAllocInfo.pMappedData, readyData.data(), size);
        } else {
            ASSERT(false);
        }

        stbi_image_free(data);

        VulkanTexture hdrTexture{};
        Format = VK_FORMAT_R32G32B32A32_SFLOAT;

        // Image
        VkImageCreateInfo imageInfo = { VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.extent.width = width;
        imageInfo.extent.height = height;
        imageInfo.extent.depth = 1;
        imageInfo.mipLevels = 1;
        imageInfo.arrayLayers = 1;
        imageInfo.format = Format;
        imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;

        VmaAllocationCreateInfo imageAllocCreateInfo = {};
        imageAllocCreateInfo.usage = VMA_MEMORY_USAGE_AUTO;

        vmaCreateImage(renderer->GetAllocator(), &imageInfo, &imageAllocCreateInfo, &hdrTexture.Image, &hdrTexture.Allocation, &hdrTexture.AllocationInfo);

        renderer->CopyBufferToImage(stagingBuffer, hdrTexture.Image, width, height);

        vmaDestroyBuffer(renderer->GetAllocator(), stagingBuffer, stagingBufferAlloc);

        VkImageViewCreateInfo hdrImageViewCreateInfo = { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
        hdrImageViewCreateInfo.image = hdrTexture.Image;
        hdrImageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        hdrImageViewCreateInfo.format = Format;
        hdrImageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        hdrImageViewCreateInfo.subresourceRange.baseMipLevel = 0;
        hdrImageViewCreateInfo.subresourceRange.levelCount = 1;
        hdrImageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
        hdrImageViewCreateInfo.subresourceRange.layerCount = 1;

        vkCreateImageView(renderer->GetDevice(), &hdrImageViewCreateInfo, nullptr, &hdrTexture.ImageView);

        hdrTexture.DesctriptorSet = renderer->AllocateTextureDescriptorSet(renderer->GetDescriptorSetLayout(RenderSetLayouts::DiffuseTextureClamped));

        auto sampler = renderer->GetTextureSamplerClamped();

        VkDescriptorImageInfo descriptorImageInfo{};
        descriptorImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        descriptorImageInfo.imageView = hdrTexture.ImageView;
        descriptorImageInfo.sampler = sampler;

        VkWriteDescriptorSet writeSet = { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
        writeSet.pImageInfo = &descriptorImageInfo;
        writeSet.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        writeSet.dstSet = hdrTexture.DesctriptorSet;
        writeSet.dstBinding = 0;
        writeSet.descriptorCount = 1;

        vkUpdateDescriptorSets(renderer->GetDevice(), 1, &writeSet, 0, nullptr);

        std::cout << "Loaded hdr texture: " << equirectangularMap << std::endl;

        // Create cubemap image
        uint32_t cubemapRes = 1024;
        VkImageCreateInfo cubeImageInfo = { VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
        cubeImageInfo.imageType = VK_IMAGE_TYPE_2D;
        cubeImageInfo.extent.width = cubemapRes;
        cubeImageInfo.extent.height = cubemapRes;
        cubeImageInfo.extent.depth = 1;
        cubeImageInfo.mipLevels = 1;
        cubeImageInfo.arrayLayers = 6;
        cubeImageInfo.format = Format;
        cubeImageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        cubeImageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        cubeImageInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        cubeImageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        cubeImageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        cubeImageInfo.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;

        VmaAllocationCreateInfo cubeImageAllocCreateInfo = {};
        cubeImageAllocCreateInfo.usage = VMA_MEMORY_USAGE_AUTO;

        vmaCreateImage(renderer->GetAllocator(), &cubeImageInfo, &cubeImageAllocCreateInfo, &CubemapTexture.Image,
            &CubemapTexture.Allocation, &CubemapTexture.AllocationInfo);

        // Cubemap view directions
        glm::mat4 captureProjection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
        glm::mat4 captureViews[] =
        {
           glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
           glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(-1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
           glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f,  1.0f,  0.0f), glm::vec3(0.0f,  0.0f,  1.0f)),
           glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f,  0.0f), glm::vec3(0.0f,  0.0f, -1.0f)),
           glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f,  0.0f,  1.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
           glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f,  0.0f, -1.0f), glm::vec3(0.0f, -1.0f,  0.0f))
        };

        {
            // init shader with compatible renderpass
            Framebuffer fb(cubemapRes, cubemapRes, Format);
            auto renderPass = fb.GetRenderPass();

            RenderPipelineParams params;
            params.Flags = RenderPipelineFlags::AlphaBlend;
            params.FragmentPath = "Shaders/EquiToCube.frag.spv";
            params.VertexPath = "Shaders/Cubemap.vert.spv";
            params.Key = RenderPipelineKeys::EquiToCube;
            params.PolygonMode = VK_POLYGON_MODE_FILL;
            params.Topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
            params.BindingDescription = SimpleVertex::GetBindingDescription();
            params.AttributeDescriptions = SimpleVertex::GetAttributeDescriptions();
            params.PushConstantSize = sizeof(SkyPushConstants);
            params.DescriptorSetLayoutBindings.resize(1);
            params.DescriptorSetLayoutBindings[0] = renderer->GetDescriptorSetLayout(RenderSetLayouts::DiffuseTextureClamped);
            params.Renderpass = renderPass;
            renderer->CreateGraphicsPipeline(params);
        }

        // Render spherical map to cubemap
        for (uint32_t i = 0; i < 6; i++) {
            VkImageViewCreateInfo imageViewInfo{};
            imageViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            imageViewInfo.image = CubemapTexture.Image;
            imageViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            imageViewInfo.format = Format;
            imageViewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            imageViewInfo.subresourceRange.baseMipLevel = 0;
            imageViewInfo.subresourceRange.levelCount = 1;
            imageViewInfo.subresourceRange.baseArrayLayer = i;
            imageViewInfo.subresourceRange.layerCount = 1;

            VkImageView cubeFaceImageView;
            vkCreateImageView(renderer->GetDevice(), &imageViewInfo, nullptr, &cubeFaceImageView);

            // Create framebuffer
            Framebuffer fb(cubemapRes, cubemapRes, cubeFaceImageView, Format);

            {
                // Render Spherical map to framebuffer
                auto renderPass = fb.GetRenderPass();
                auto frameBuffer = fb.GetFramebuffer();
                VkRenderPassBeginInfo renderPassInfo{};
                renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
                renderPassInfo.renderPass = renderPass;
                renderPassInfo.framebuffer = frameBuffer;
                renderPassInfo.renderArea.offset = { 0, 0 };

                VkExtent2D vkExtent;
                vkExtent.width = cubemapRes;
                vkExtent.height = cubemapRes;

                renderPassInfo.renderArea.extent = vkExtent;

                auto cubeBuffer = ModelManager::GetSkyboxCube();

                auto commandBuffer = renderer->BeginSingleUseCommandBuffer();
                renderer->StartRenderPass(commandBuffer, &renderPassInfo);
                auto pipelineLayout = renderer->BindGraphicsPipeline(commandBuffer, RenderPipelineKeys::EquiToCube);
                SkyPushConstants constants{};
                constants.VP = captureProjection * captureViews[i];
                constants.Model = glm::mat4(1.f);
                constants.InvModelMat = glm::inverse(constants.Model);
                vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(SkyPushConstants),
                    &constants);
                VkDeviceSize offset{ 0 };
                vkCmdBindVertexBuffers(commandBuffer, 0, 1, &cubeBuffer.Buffer, &offset);
                vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout,
                    0, 1, &hdrTexture.DesctriptorSet, 0, nullptr);
                vkCmdDraw(commandBuffer, 36, 1, 0, 0);
                vkCmdEndRenderPass(commandBuffer);
                renderer->EndSingleUseCommandBuffer(commandBuffer);
            }

            vkDestroyImageView(renderer->GetDevice(), cubeFaceImageView, nullptr);
        }

        // Destroy hdr spherical texture
        renderer->FreeTextureDescriptorSet(hdrTexture.DesctriptorSet);
        vkDestroyImageView(renderer->GetDevice(), hdrTexture.ImageView, nullptr);
        vmaDestroyImage(renderer->GetAllocator(), hdrTexture.Image, hdrTexture.Allocation);


        // Create cubemap image view
        VkImageViewCreateInfo imageViewCreateInfo = { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
        imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
        imageViewCreateInfo.format = Format;
        imageViewCreateInfo.components = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A };
        imageViewCreateInfo.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
        // 6 array layers (faces)
        imageViewCreateInfo.subresourceRange.layerCount = 6;
        // Set number of mip levels
        imageViewCreateInfo.subresourceRange.levelCount = 1;
        imageViewCreateInfo.image = CubemapTexture.Image;

        VkResult result = vkCreateImageView(renderer->GetDevice(), &imageViewCreateInfo, nullptr, &CubemapTexture.ImageView);
        ASSERT(result == VK_SUCCESS);

        // Create cubemap descriptor
        descriptorImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        descriptorImageInfo.imageView = CubemapTexture.ImageView;
        descriptorImageInfo.sampler = sampler;

        CubemapTexture.DesctriptorSet = renderer->AllocateTextureDescriptorSet(renderer->GetDescriptorSetLayout(RenderSetLayouts::DiffuseTextureClamped));

        writeSet.pImageInfo = &descriptorImageInfo;
        writeSet.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        writeSet.dstSet = CubemapTexture.DesctriptorSet;
        writeSet.dstBinding = 0;
        writeSet.descriptorCount = 1;

        vkUpdateDescriptorSets(renderer->GetDevice(), 1, &writeSet, 0, nullptr);
    }
    VulkanTexture Cubemap::GetIrradienceMap()
    {
        auto* renderer = VulkanRenderer::Get();

        VulkanTexture irradienceTexture{};

        uint32_t cubemapRes = 32;

        VkImageCreateInfo cubeImageInfo = { VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
        cubeImageInfo.imageType = VK_IMAGE_TYPE_2D;
        cubeImageInfo.extent.width = cubemapRes;
        cubeImageInfo.extent.height = cubemapRes;
        cubeImageInfo.extent.depth = 1;
        cubeImageInfo.mipLevels = 1;
        cubeImageInfo.arrayLayers = 6;
        cubeImageInfo.format = Format;
        cubeImageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        cubeImageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        cubeImageInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        cubeImageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        cubeImageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        cubeImageInfo.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;

        VmaAllocationCreateInfo cubeImageAllocCreateInfo = {};
        cubeImageAllocCreateInfo.usage = VMA_MEMORY_USAGE_AUTO;

        vmaCreateImage(renderer->GetAllocator(), &cubeImageInfo, &cubeImageAllocCreateInfo, &irradienceTexture.Image,
            &irradienceTexture.Allocation, &irradienceTexture.AllocationInfo);

        {
            // init shader with compatible renderpass
            Framebuffer fb(cubemapRes, cubemapRes, Format);
            auto renderPass = fb.GetRenderPass();

            RenderPipelineParams params;
            params.Flags = RenderPipelineFlags::AlphaBlend;
            params.FragmentPath = "Shaders/IrradianceConv.frag.spv";
            params.VertexPath = "Shaders/Cubemap.vert.spv";
            params.Key = RenderPipelineKeys::IrradianceConv;
            params.PolygonMode = VK_POLYGON_MODE_FILL;
            params.Topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
            params.BindingDescription = SimpleVertex::GetBindingDescription();
            params.AttributeDescriptions = SimpleVertex::GetAttributeDescriptions();
            params.PushConstantSize = sizeof(SkyPushConstants);
            params.DescriptorSetLayoutBindings.resize(1);
            params.DescriptorSetLayoutBindings[0] = renderer->GetDescriptorSetLayout(RenderSetLayouts::DiffuseTextureClamped);
            params.Renderpass = renderPass;
            renderer->CreateGraphicsPipeline(params);
        }

        // Cubemap view directions
        glm::mat4 captureProjection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
        glm::mat4 captureViews[] =
        {
           glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
           glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(-1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
           glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f,  1.0f,  0.0f), glm::vec3(0.0f,  0.0f,  1.0f)),
           glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f,  0.0f), glm::vec3(0.0f,  0.0f, -1.0f)),
           glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f,  0.0f,  1.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
           glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f,  0.0f, -1.0f), glm::vec3(0.0f, -1.0f,  0.0f))
        };


        for (uint32_t i = 0; i < 6; i++) {
            VkImageViewCreateInfo imageViewInfo{};
            imageViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            imageViewInfo.image = irradienceTexture.Image;
            imageViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            imageViewInfo.format = Format;
            imageViewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            imageViewInfo.subresourceRange.baseMipLevel = 0;
            imageViewInfo.subresourceRange.levelCount = 1;
            imageViewInfo.subresourceRange.baseArrayLayer = i;
            imageViewInfo.subresourceRange.layerCount = 1;

            VkImageView cubeFaceImageView;
            vkCreateImageView(renderer->GetDevice(), &imageViewInfo, nullptr, &cubeFaceImageView);

            Framebuffer fb(cubemapRes, cubemapRes, cubeFaceImageView, Format);

            {
                auto renderPass = fb.GetRenderPass();
                auto frameBuffer = fb.GetFramebuffer();

                VkRenderPassBeginInfo renderPassInfo{};
                renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
                renderPassInfo.renderPass = renderPass;
                renderPassInfo.framebuffer = frameBuffer;
                renderPassInfo.renderArea.offset = { 0, 0 };

                VkExtent2D vkExtent;
                vkExtent.width = cubemapRes;
                vkExtent.height = cubemapRes;

                renderPassInfo.renderArea.extent = vkExtent;

                auto cubeBuffer = ModelManager::GetSkyboxCube();

                auto commandBuffer = renderer->BeginSingleUseCommandBuffer();
                renderer->StartRenderPass(commandBuffer, &renderPassInfo);
                auto pipelineLayout = renderer->BindGraphicsPipeline(commandBuffer, RenderPipelineKeys::IrradianceConv);
                SkyPushConstants constants{};
                constants.VP = captureProjection * captureViews[i];
                constants.Model = glm::mat4(1.f);
                constants.InvModelMat = glm::inverse(constants.Model);
                vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(SkyPushConstants),
                    &constants);
                VkDeviceSize offset{ 0 };
                vkCmdBindVertexBuffers(commandBuffer, 0, 1, &cubeBuffer.Buffer, &offset);
                vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout,
                    0, 1, &CubemapTexture.DesctriptorSet, 0, nullptr);
                vkCmdDraw(commandBuffer, 36, 1, 0, 0);
                vkCmdEndRenderPass(commandBuffer);
                renderer->EndSingleUseCommandBuffer(commandBuffer);
            }

            vkDestroyImageView(renderer->GetDevice(), cubeFaceImageView, nullptr);
        }

        std::cout << "Irradiance map loaded.\n";

        // Create cubemap image view
        VkImageViewCreateInfo imageViewCreateInfo = { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
        imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
        imageViewCreateInfo.format = Format;
        imageViewCreateInfo.components = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A };
        imageViewCreateInfo.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
        // 6 array layers (faces)
        imageViewCreateInfo.subresourceRange.layerCount = 6;
        // Set number of mip levels
        imageViewCreateInfo.subresourceRange.levelCount = 1;
        imageViewCreateInfo.image = irradienceTexture.Image;

        VkResult result = vkCreateImageView(renderer->GetDevice(), &imageViewCreateInfo, nullptr, &irradienceTexture.ImageView);
        ASSERT(result == VK_SUCCESS);

        auto sampler = renderer->GetTextureSamplerClamped();

        // Create cubemap descriptor
        VkDescriptorImageInfo descriptorImageInfo{};
        descriptorImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        descriptorImageInfo.imageView = irradienceTexture.ImageView;
        descriptorImageInfo.sampler = sampler;

        irradienceTexture.DesctriptorSet = renderer->AllocateTextureDescriptorSet(renderer->GetDescriptorSetLayout(RenderSetLayouts::DiffuseTextureClamped));

        VkWriteDescriptorSet writeSet = { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
        writeSet.pImageInfo = &descriptorImageInfo;
        writeSet.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        writeSet.dstSet = irradienceTexture.DesctriptorSet;
        writeSet.dstBinding = 0;
        writeSet.descriptorCount = 1;

        vkUpdateDescriptorSets(renderer->GetDevice(), 1, &writeSet, 0, nullptr);

        return irradienceTexture;
    }
    VulkanTexture Cubemap::GetPrefilterMap()
    {
        auto* renderer = VulkanRenderer::Get();

        VulkanTexture prefilterMap{};

        uint32_t cubemapRes = 512;

        uint32_t mipLevels = static_cast<uint32_t>(std::floor(std::log2(cubemapRes))) + 1;

        VkImageCreateInfo cubeImageInfo = { VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
        cubeImageInfo.imageType = VK_IMAGE_TYPE_2D;
        cubeImageInfo.extent.width = cubemapRes;
        cubeImageInfo.extent.height = cubemapRes;
        cubeImageInfo.extent.depth = 1;
        cubeImageInfo.mipLevels = mipLevels;
        cubeImageInfo.arrayLayers = 6;
        cubeImageInfo.format = Format;
        cubeImageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        cubeImageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        cubeImageInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        cubeImageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        cubeImageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        cubeImageInfo.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;

        VmaAllocationCreateInfo cubeImageAllocCreateInfo = {};
        cubeImageAllocCreateInfo.usage = VMA_MEMORY_USAGE_AUTO;

        vmaCreateImage(renderer->GetAllocator(), &cubeImageInfo, &cubeImageAllocCreateInfo, &prefilterMap.Image,
            &prefilterMap.Allocation, &prefilterMap.AllocationInfo);
        
        {
            // init shader with compatible renderpass
            Framebuffer fb(cubemapRes, cubemapRes, Format);
            auto renderPass = fb.GetRenderPass();

            RenderPipelineParams params;
            params.Flags = RenderPipelineFlags::AlphaBlend;
            params.FragmentPath = "Shaders/Prefilter.frag.spv";
            params.VertexPath = "Shaders/Cubemap.vert.spv";
            params.Key = RenderPipelineKeys::Prefilter;
            params.PolygonMode = VK_POLYGON_MODE_FILL;
            params.Topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
            params.BindingDescription = SimpleVertex::GetBindingDescription();
            params.AttributeDescriptions = SimpleVertex::GetAttributeDescriptions();
            params.PushConstantSize = sizeof(SkyPushConstants);
            params.DescriptorSetLayoutBindings.resize(2);
            params.DescriptorSetLayoutBindings[0] = renderer->GetDescriptorSetLayout(RenderSetLayouts::DiffuseTextureClamped);
            params.DescriptorSetLayoutBindings[1] = renderer->GetDescriptorSetLayout(RenderSetLayouts::SceneFrameUBO);
            params.Renderpass = renderPass;
            renderer->CreateGraphicsPipeline(params);
        }

        // Cubemap view directions
        glm::mat4 captureProjection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
        glm::mat4 captureViews[] =
        {
           glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
           glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(-1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
           glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f,  1.0f,  0.0f), glm::vec3(0.0f,  0.0f,  1.0f)),
           glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f,  0.0f), glm::vec3(0.0f,  0.0f, -1.0f)),
           glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f,  0.0f,  1.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
           glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f,  0.0f, -1.0f), glm::vec3(0.0f, -1.0f,  0.0f))
        };

        for (uint32_t i = 0; i < 6; i++) {
            for (uint32_t mip = 0; mip < mipLevels; mip++) {
                uint32_t mipRes = cubemapRes * std::pow(0.5, mip);

                VkImageViewCreateInfo imageViewInfo{};
                imageViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
                imageViewInfo.image = prefilterMap.Image;
                imageViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
                imageViewInfo.format = Format;
                imageViewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                imageViewInfo.subresourceRange.baseMipLevel = mip;
                imageViewInfo.subresourceRange.levelCount = 1;
                imageViewInfo.subresourceRange.baseArrayLayer = i;
                imageViewInfo.subresourceRange.layerCount = 1;

                VkImageView cubeFaceImageView;
                vkCreateImageView(renderer->GetDevice(), &imageViewInfo, nullptr, &cubeFaceImageView);

                Framebuffer fb(mipRes, mipRes, cubeFaceImageView, Format);

                {
                    auto renderPass = fb.GetRenderPass();
                    auto frameBuffer = fb.GetFramebuffer();

                    VkRenderPassBeginInfo renderPassInfo{};
                    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
                    renderPassInfo.renderPass = renderPass;
                    renderPassInfo.framebuffer = frameBuffer;
                    renderPassInfo.renderArea.offset = { 0, 0 };

                    VkExtent2D vkExtent;
                    vkExtent.width = mipRes;
                    vkExtent.height = mipRes;

                    renderPassInfo.renderArea.extent = vkExtent;

                    auto commandBuffer = renderer->BeginSingleUseCommandBuffer();
                    renderer->StartRenderPass(commandBuffer, &renderPassInfo);
                    auto pipelineLayout = renderer->BindGraphicsPipeline(commandBuffer, RenderPipelineKeys::Prefilter);
                    auto cubeBuffer = ModelManager::GetSkyboxCube();
                    SkyPushConstants constants{};
                    constants.VP = captureProjection * captureViews[i];
                    constants.Model = glm::mat4(1.f);
                    constants.InvModelMat = glm::inverse(constants.Model);
                    vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(SkyPushConstants),
                        &constants);
                    VkDeviceSize offset{ 0 };

                    float roughness = (float)mip / (float)(mipLevels - 1);
                    VulkanUBO<float> ubo(roughness, RenderSetLayouts::SceneFrameUBO);
                    auto uboDescriptor = ubo.GetDescriptorSet();
                    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout,
                        1, 1, &uboDescriptor, 0, nullptr);

                    vkCmdBindVertexBuffers(commandBuffer, 0, 1, &cubeBuffer.Buffer, &offset);
                    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout,
                        0, 1, &CubemapTexture.DesctriptorSet, 0, nullptr);
                    vkCmdDraw(commandBuffer, 36, 1, 0, 0);
                    vkCmdEndRenderPass(commandBuffer);
                    renderer->EndSingleUseCommandBuffer(commandBuffer);
                }
                vkDestroyImageView(renderer->GetDevice(), cubeFaceImageView, nullptr);
            }
        }

        // Create cubemap image view
        VkImageViewCreateInfo imageViewCreateInfo = { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
        imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
        imageViewCreateInfo.format = Format;
        imageViewCreateInfo.components = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A };
        imageViewCreateInfo.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
        // 6 array layers (faces)
        imageViewCreateInfo.subresourceRange.layerCount = 6;
        // Set number of mip levels
        imageViewCreateInfo.subresourceRange.levelCount = mipLevels;
        imageViewCreateInfo.image = prefilterMap.Image;

        VkResult result = vkCreateImageView(renderer->GetDevice(), &imageViewCreateInfo, nullptr, &prefilterMap.ImageView);
        ASSERT(result == VK_SUCCESS);

        auto sampler = renderer->GetTextureSamplerClamped();

        // Create cubemap descriptor
        VkDescriptorImageInfo descriptorImageInfo{};
        descriptorImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        descriptorImageInfo.imageView = prefilterMap.ImageView;
        descriptorImageInfo.sampler = sampler;

        prefilterMap.DesctriptorSet = renderer->AllocateTextureDescriptorSet(renderer->GetDescriptorSetLayout(RenderSetLayouts::DiffuseTextureClamped));

        VkWriteDescriptorSet writeSet = { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
        writeSet.pImageInfo = &descriptorImageInfo;
        writeSet.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        writeSet.dstSet = prefilterMap.DesctriptorSet;
        writeSet.dstBinding = 0;
        writeSet.descriptorCount = 1;

        vkUpdateDescriptorSets(renderer->GetDevice(), 1, &writeSet, 0, nullptr);

        return prefilterMap;
    }
    Cubemap::~Cubemap()
    {
        auto* renderer = VulkanRenderer::Get();
        vkDestroyImageView(renderer->GetDevice(), CubemapTexture.ImageView, nullptr);
        vmaDestroyImage(renderer->GetAllocator(), CubemapTexture.Image, CubemapTexture.Allocation);
        renderer->FreeTextureDescriptorSet(CubemapTexture.DesctriptorSet);
    }
}