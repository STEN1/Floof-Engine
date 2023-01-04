#include "TextureManager.h"
#include "stb_image/stb_image.h"
#include "imgui_impl_vulkan.h"
#include "Framebuffer.h"
#include "ModelManager.h"

namespace FLOOF {
    void TextureManager::DestroyAll() {
        auto renderer = VulkanRenderer::Get();
        auto device = renderer->GetDevice();
        auto allocator = VulkanRenderer::Get()->GetAllocator();
        for (auto &[key, val]: s_TextureCache) {
            vmaDestroyImage(allocator, val.Image, val.Allocation);
            vkDestroyImageView(device, val.ImageView, nullptr);
            renderer->FreeTextureDescriptorSet(val.DesctriptorSet);
        }
        s_TextureCache.clear();
        for (auto &[key, val]: s_ColorTextureCache) {
            vmaDestroyImage(allocator, val.Image, val.Allocation);
            vkDestroyImageView(device, val.ImageView, nullptr);
            renderer->FreeTextureDescriptorSet(val.DesctriptorSet);
        }
        s_ColorTextureCache.clear();
        renderer->FreeTextureDescriptorSet(s_BRDFLut.DesctriptorSet);
        vkDestroyImageView(renderer->GetDevice(), s_BRDFLut.ImageView, nullptr);
        vmaDestroyImage(renderer->GetAllocator(), s_BRDFLut.Image, s_BRDFLut.Allocation);
    }

    VulkanTexture TextureManager::GetTextureFromPath(const std::string &path, bool flip, bool glNormal) {
        auto it = s_TextureCache.find(path);
        if (it != s_TextureCache.end())
            return it->second;

        auto renderer = VulkanRenderer::Get();
        if(!renderer)
            return {};
        // Load texture
        int xWidth, yHeight, channels;
        stbi_set_flip_vertically_on_load(flip);
        auto *data = stbi_load(path.c_str(), &xWidth, &yHeight, &channels, 0);

        VulkanTexture texture;
        if (data == nullptr) {
            std::cout << "Could not load texture: " << path << std::endl;
            return texture;
        }

        uint32_t size = xWidth * yHeight * 4;

        VkFormat format = VK_FORMAT_R8G8B8A8_UNORM;

        // staging buffer
        VkBufferCreateInfo stagingCreateInfo = {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
        stagingCreateInfo.size = size;
        stagingCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

        VmaAllocationCreateInfo stagingBufAllocCreateInfo = {};
        stagingBufAllocCreateInfo.usage = VMA_MEMORY_USAGE_AUTO;
        stagingBufAllocCreateInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
                                          VMA_ALLOCATION_CREATE_MAPPED_BIT;

        VkBuffer stagingBuffer{};
        VmaAllocation stagingBufferAlloc{};
        VmaAllocationInfo stagingBufferAllocInfo{};
        vmaCreateBuffer(renderer->m_Allocator, &stagingCreateInfo, &stagingBufAllocCreateInfo, &stagingBuffer,
                        &stagingBufferAlloc, &stagingBufferAllocInfo);

        ASSERT(stagingBufferAllocInfo.pMappedData != nullptr);
        if (channels == 4) {
            if (glNormal) {
                for (uint32_t h = 0; h < yHeight; h++) {
                    for (uint32_t w = 0; w < xWidth; w++) {
                        uint32_t dataIndex = (h * xWidth * 4) + (w * 4);
                        data[dataIndex + 1] = (stbi_uc) 255 - data[dataIndex + 1];
                    }
                }
            }
            memcpy(stagingBufferAllocInfo.pMappedData, data, size);
        } else if (channels == 3) {
            std::vector<stbi_uc> readyData(size);
            for (uint32_t h = 0; h < yHeight; h++) {
                for (uint32_t w = 0; w < xWidth; w++) {
                    uint32_t readyDataIndex = (h * xWidth * 4) + (w * 4);
                    auto &rdR = readyData[readyDataIndex];
                    auto &rdG = readyData[readyDataIndex + 1];
                    auto &rdB = readyData[readyDataIndex + 2];
                    auto &rdA = readyData[readyDataIndex + 3];

                    uint32_t dataIndex = (h * xWidth * 3) + (w * 3);
                    auto &dR = data[dataIndex];
                    auto &dG = data[dataIndex + 1];
                    auto &dB = data[dataIndex + 2];

                    rdR = dR;
                    rdG = dG;
                    rdB = dB;
                    rdA = (stbi_uc) 255;
                }
            }
            if (glNormal) {
                for (uint32_t h = 0; h < yHeight; h++) {
                    for (uint32_t w = 0; w < xWidth; w++) {
                        uint32_t dataIndex = (h * xWidth * 4) + (w * 4);
                        readyData[dataIndex + 1] = (stbi_uc) 255 - readyData[dataIndex + 1];
                    }
                }
            }
            memcpy(stagingBufferAllocInfo.pMappedData, readyData.data(), size);
        } else if (channels == 1) {
            std::vector<stbi_uc> readyData(size);
            for (uint32_t h = 0; h < yHeight; h++) {
                for (uint32_t w = 0; w < xWidth; w++) {
                    uint32_t readyDataIndex = (h * xWidth * 4) + (w * 4);
                    auto &rdR = readyData[readyDataIndex];
                    auto &rdG = readyData[readyDataIndex + 1];
                    auto &rdB = readyData[readyDataIndex + 2];
                    auto &rdA = readyData[readyDataIndex + 3];

                    uint32_t dataIndex = (h * xWidth * 1) + (w * 1);
                    auto &dR = data[dataIndex];

                    rdR = dR;
                    rdG = dR;
                    rdB = dR;
                    rdA = (stbi_uc) 255;
                }
            }
            memcpy(stagingBufferAllocInfo.pMappedData, readyData.data(), size);
        } else {
            ASSERT(false);
        }

        stbi_image_free(data);

        uint32_t mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(xWidth, yHeight)))) + 1;

        // Image
        VkImageCreateInfo imageInfo = {VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.extent.width = xWidth;
        imageInfo.extent.height = yHeight;
        imageInfo.extent.depth = 1;
        imageInfo.mipLevels = mipLevels;
        imageInfo.arrayLayers = 1;
        imageInfo.format = format;
        imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.flags = 0;

        VmaAllocationCreateInfo imageAllocCreateInfo = {};
        imageAllocCreateInfo.usage = VMA_MEMORY_USAGE_AUTO;

        vmaCreateImage(renderer->m_Allocator, &imageInfo, &imageAllocCreateInfo, &texture.Image,
                       &texture.Allocation, &texture.AllocationInfo);

        // copy image from staging buffer to image buffer(gpu only memory)
        renderer->CopyBufferToImage(stagingBuffer, texture.Image, xWidth, yHeight);

        // free staging buffer
        vmaDestroyBuffer(renderer->m_Allocator, stagingBuffer, stagingBufferAlloc);

        renderer->TransitionImageLayout(texture.Image,
                                        VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, mipLevels);
        // Generate mipmaps transitions layout to shader read optimal
        renderer->GenerateMipmaps(texture.Image, format, xWidth, yHeight, mipLevels);

        // create image view
        VkImageViewCreateInfo textureImageViewInfo = {VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
        textureImageViewInfo.image = texture.Image;
        textureImageViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        textureImageViewInfo.format = format;
        textureImageViewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        textureImageViewInfo.subresourceRange.baseMipLevel = 0;
        textureImageViewInfo.subresourceRange.levelCount = mipLevels;
        textureImageViewInfo.subresourceRange.baseArrayLayer = 0;
        textureImageViewInfo.subresourceRange.layerCount = 1;
        vkCreateImageView(renderer->m_LogicalDevice, &textureImageViewInfo, nullptr, &texture.ImageView);

        // Using font sampler so imgui has an easy time displaying textures in editor.
        // Actual descriptor for rendering with proper sampler is stored with material.
        VkSampler sampler = renderer->GetFontSampler();

        texture.DesctriptorSet = ImGui_ImplVulkan_AddTexture(sampler, texture.ImageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

        s_TextureCache[path] = texture;

        return texture;
    }

    VulkanTexture TextureManager::GetTextureFromColor(TextureColor color) {
        auto it = s_ColorTextureCache.find(color);
        if (it != s_ColorTextureCache.end())
            return it->second;

        auto *renderer = VulkanRenderer::Get();
        if(!renderer)
            return {};


        uint32_t xWidth = 2;
        uint32_t yHeight = 2;
        uint32_t size = xWidth * yHeight * 4;
        VkFormat format = VK_FORMAT_R8G8B8A8_UNORM;

        stbi_uc dR = 0;
        stbi_uc dG = 0;
        stbi_uc dB = 0;

        switch (color) {
            case TextureColor::None:
                break;
            case TextureColor::Red:
                dR = (stbi_uc) 255;
                break;
            case TextureColor::Green:
                dG = (stbi_uc) 255;
                break;
            case TextureColor::Blue:
                dB = (stbi_uc) 255;
                break;
            case TextureColor::White:
                dR = (stbi_uc) 255;
                dG = (stbi_uc) 255;
                dB = (stbi_uc) 255;
                break;
            case TextureColor::Black:
                break;
            case TextureColor::DarkGrey:
                dR = (stbi_uc)25;
                dG = (stbi_uc)25;
                dB = (stbi_uc)25;
                break;
            case TextureColor::Grey:
                dR = (stbi_uc)128;
                dG = (stbi_uc)128;
                dB = (stbi_uc)128;
                break;
            case TextureColor::LightGrey:
                dR = (stbi_uc)200;
                dG = (stbi_uc)200;
                dB = (stbi_uc)200;
                break;
            case TextureColor::FlatNormal:
                dR = (stbi_uc) 128;
                dG = (stbi_uc) 128;
                dB = (stbi_uc) 255;
                break;
        }

        // staging buffer
        VkBufferCreateInfo stagingCreateInfo = {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
        stagingCreateInfo.size = size;
        stagingCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

        VmaAllocationCreateInfo stagingBufAllocCreateInfo = {};
        stagingBufAllocCreateInfo.usage = VMA_MEMORY_USAGE_AUTO;
        stagingBufAllocCreateInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
                                          VMA_ALLOCATION_CREATE_MAPPED_BIT;

        VkBuffer stagingBuffer{};
        VmaAllocation stagingBufferAlloc{};
        VmaAllocationInfo stagingBufferAllocInfo{};
        vmaCreateBuffer(renderer->m_Allocator, &stagingCreateInfo, &stagingBufAllocCreateInfo, &stagingBuffer,
                        &stagingBufferAlloc, &stagingBufferAllocInfo);

        std::vector<stbi_uc> readyData(size);
        for (uint32_t h = 0; h < yHeight; h++) {
            for (uint32_t w = 0; w < xWidth; w++) {
                uint32_t readyDataIndex = (h * xWidth * 4) + (w * 4);
                auto &rdR = readyData[readyDataIndex];
                auto &rdG = readyData[readyDataIndex + 1];
                auto &rdB = readyData[readyDataIndex + 2];
                auto &rdA = readyData[readyDataIndex + 3];

                rdR = dR;
                rdG = dG;
                rdB = dB;
                rdA = (stbi_uc) 255;
            }
        }
        ASSERT(stagingBufferAllocInfo.pMappedData != nullptr);
        memcpy(stagingBufferAllocInfo.pMappedData, readyData.data(), size);

        VulkanTexture texture;
        // Image
        VkImageCreateInfo imageInfo = {VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.extent.width = xWidth;
        imageInfo.extent.height = yHeight;
        imageInfo.extent.depth = 1;
        imageInfo.mipLevels = 1;
        imageInfo.arrayLayers = 1;
        imageInfo.format = format;
        imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.flags = 0;

        VmaAllocationCreateInfo imageAllocCreateInfo = {};
        imageAllocCreateInfo.usage = VMA_MEMORY_USAGE_AUTO;

        vmaCreateImage(renderer->m_Allocator, &imageInfo, &imageAllocCreateInfo, &texture.Image,
                       &texture.Allocation, &texture.AllocationInfo);

        // copy image from staging buffer to image buffer(gpu only memory)
        renderer->CopyBufferToImage(stagingBuffer, texture.Image, xWidth, yHeight);

        // free staging buffer
        vmaDestroyBuffer(renderer->m_Allocator, stagingBuffer, stagingBufferAlloc);

        // create image view
        VkImageViewCreateInfo textureImageViewInfo = {VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
        textureImageViewInfo.image = texture.Image;
        textureImageViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        textureImageViewInfo.format = format;
        textureImageViewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        textureImageViewInfo.subresourceRange.baseMipLevel = 0;
        textureImageViewInfo.subresourceRange.levelCount = 1;
        textureImageViewInfo.subresourceRange.baseArrayLayer = 0;
        textureImageViewInfo.subresourceRange.layerCount = 1;
        vkCreateImageView(renderer->m_LogicalDevice, &textureImageViewInfo, nullptr, &texture.ImageView);

        // Using font sampler so imgui has an easy time displaying textures in editor.
        // Actual descriptor for rendering with proper sampler is stored with material.
        VkSampler sampler = renderer->GetFontSampler();

        texture.DesctriptorSet = ImGui_ImplVulkan_AddTexture(sampler, texture.ImageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

        s_ColorTextureCache[color] = texture;

        return texture;
    }

    VulkanTexture TextureManager::GetBRDFLut() {
        if (s_BRDFLut.Image != VK_NULL_HANDLE)
            return s_BRDFLut;

        auto *renderer = VulkanRenderer::Get();

        VulkanTexture brdfLut;

        VkFormat format = VK_FORMAT_R32G32B32A32_SFLOAT;
        uint32_t lutRes = 512;

        VkImageCreateInfo imageInfo = {VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.extent.width = lutRes;
        imageInfo.extent.height = lutRes;
        imageInfo.extent.depth = 1;
        imageInfo.mipLevels = 1;
        imageInfo.arrayLayers = 1;
        imageInfo.format = format;
        imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.flags = 0;

        VmaAllocationCreateInfo imageAllocCreateInfo = {};
        imageAllocCreateInfo.usage = VMA_MEMORY_USAGE_AUTO;

        vmaCreateImage(renderer->GetAllocator(), &imageInfo, &imageAllocCreateInfo, &brdfLut.Image,
                       &brdfLut.Allocation, &brdfLut.AllocationInfo);

        {
            // init shader with compatible renderpass
            Framebuffer fb(lutRes, lutRes, format);
            auto renderPass = fb.GetRenderPass();

            RenderPipelineParams params;
            params.Flags = RenderPipelineFlags::AlphaBlend;
            params.FragmentPath = "Shaders/BRDF.frag.spv";
            params.VertexPath = "Shaders/BRDF.vert.spv";
            params.Key = RenderPipelineKeys::BRDF;
            params.PolygonMode = VK_POLYGON_MODE_FILL;
            params.Topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
            params.BindingDescription = MeshVertex::GetBindingDescription();
            params.AttributeDescriptions = MeshVertex::GetAttributeDescriptions();
            params.PushConstantSize = sizeof(SkyPushConstants);
            params.DescriptorSetLayoutBindings.resize(2);
            params.DescriptorSetLayoutBindings[0] = renderer->GetDescriptorSetLayout(RenderSetLayouts::DiffuseTextureClamped);
            params.DescriptorSetLayoutBindings[1] = renderer->GetDescriptorSetLayout(RenderSetLayouts::SceneFrameUBO);
            params.Renderpass = renderPass;
            renderer->CreateGraphicsPipeline(params);
        }

        {
            VkImageViewCreateInfo imageViewInfo{};
            imageViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            imageViewInfo.image = brdfLut.Image;
            imageViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            imageViewInfo.format = format;
            imageViewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            imageViewInfo.subresourceRange.baseMipLevel = 0;
            imageViewInfo.subresourceRange.levelCount = 1;
            imageViewInfo.subresourceRange.baseArrayLayer = 0;
            imageViewInfo.subresourceRange.layerCount = 1;

            VkImageView lutImageView;
            vkCreateImageView(renderer->GetDevice(), &imageViewInfo, nullptr, &lutImageView);
            // render lut
            Framebuffer fb(lutRes, lutRes, lutImageView, format);

            auto renderpass = fb.GetRenderPass();
            auto framebuffer = fb.GetFramebuffer();

            auto renderPass = fb.GetRenderPass();
            auto frameBuffer = fb.GetFramebuffer();

            VkRenderPassBeginInfo renderPassInfo{};
            renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
            renderPassInfo.renderPass = renderPass;
            renderPassInfo.framebuffer = frameBuffer;
            renderPassInfo.renderArea.offset = {0, 0};

            VkExtent2D vkExtent;
            vkExtent.width = lutRes;
            vkExtent.height = lutRes;

            renderPassInfo.renderArea.extent = vkExtent;

            auto ndcRect = ModelManager::GetNDCRect();

            auto commandBuffer = renderer->BeginSingleUseCommandBuffer();
            renderer->StartRenderPass(commandBuffer, &renderPassInfo);

            auto pipelineLayout = renderer->BindGraphicsPipeline(commandBuffer, RenderPipelineKeys::BRDF);

            VkDeviceSize offset{0};
            vkCmdBindVertexBuffers(commandBuffer, 0, 1, &ndcRect.Buffer, &offset);

            vkCmdDraw(commandBuffer, 4, 1, 0, 0);

            vkCmdEndRenderPass(commandBuffer);

            renderer->EndSingleUseCommandBuffer(commandBuffer);

            vkDestroyImageView(renderer->GetDevice(), lutImageView, nullptr);
        }

        // create image view
        VkImageViewCreateInfo textureImageViewInfo = {VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
        textureImageViewInfo.image = brdfLut.Image;
        textureImageViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        textureImageViewInfo.format = format;
        textureImageViewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        textureImageViewInfo.subresourceRange.baseMipLevel = 0;
        textureImageViewInfo.subresourceRange.levelCount = 1;
        textureImageViewInfo.subresourceRange.baseArrayLayer = 0;
        textureImageViewInfo.subresourceRange.layerCount = 1;
        vkCreateImageView(renderer->m_LogicalDevice, &textureImageViewInfo, nullptr, &brdfLut.ImageView);

        VkSampler sampler = renderer->GetTextureSamplerClamped();

        brdfLut.DesctriptorSet = renderer->AllocateTextureDescriptorSet(renderer->GetDescriptorSetLayout(RenderSetLayouts::DiffuseTextureClamped));

        VkDescriptorImageInfo lutImageInfo{};
        lutImageInfo.sampler = sampler;
        lutImageInfo.imageView = brdfLut.ImageView;
        lutImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        VkWriteDescriptorSet writeSet{};
        writeSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writeSet.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        writeSet.descriptorCount = 1;
        writeSet.dstSet = brdfLut.DesctriptorSet;
        writeSet.dstBinding = 0;
        writeSet.pImageInfo = &lutImageInfo;

        vkUpdateDescriptorSets(renderer->GetDevice(), 1, &writeSet, 0, nullptr);

        s_BRDFLut = brdfLut;

        return brdfLut;
    }
}