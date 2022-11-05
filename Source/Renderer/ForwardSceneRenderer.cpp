#include "ForwardSceneRenderer.h"
#include "VulkanRenderer.h"
#include "../Components.h"
#include "../Application.h"
#include <iostream>

namespace FLOOF {
    ForwardSceneRenderer::ForwardSceneRenderer() {
        CreateTextureRenderer();
    }

    ForwardSceneRenderer::~ForwardSceneRenderer() {
        DestroyTextureRenderer();
    }

    VkDescriptorSet ForwardSceneRenderer::RenderToTexture(std::shared_ptr<Scene> scene, glm::vec2 extent) {
        if (extent == glm::vec2(0.f))
            return VK_NULL_HANDLE;
        if (extent != m_Extent)
            ResizeBuffers(extent);

        auto& app = Application::Get();
        auto* renderer = VulkanRenderer::Get();
        auto* window = renderer->GetVulkanWindow();
        auto frameIndex = window->FrameIndex;
        auto& frameData = window->Frames[frameIndex];
        auto waitSemaphore = frameData.ImageAvailableSemaphore;
        auto signalSemaphore = frameData.MainPassEndSemaphore;
        auto commandBuffer = frameData.MainCommandBuffer;

        VkExtent2D vkExtent;
        vkExtent.width = m_Extent.x;
        vkExtent.height = m_Extent.y;

        VkRenderPassBeginInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = m_RenderPass;
        renderPassInfo.framebuffer = m_TextureFrameBuffers[frameIndex].FrameBuffer;
        renderPassInfo.renderArea.offset = { 0, 0 };
        renderPassInfo.renderArea.extent = vkExtent;

        VkClearValue clearColors[2]{};
        clearColors[0].color = {};
        clearColors[0].color.float32[0] = 0.f;
        clearColors[0].color.float32[1] = 0.14f;
        clearColors[0].color.float32[2] = 0.28;
        clearColors[0].color.float32[3] = 1.f;
        clearColors[1].depthStencil = { 1.0f, 0 };

        renderPassInfo.clearValueCount = 2;
        renderPassInfo.pClearValues = clearColors;

        renderer->StartRenderPass(commandBuffer, &renderPassInfo);

        auto drawMode = app.GetDrawMode();
        if (drawMode != RenderPipelineKeys::Wireframe)
            drawMode = RenderPipelineKeys::ForwardLit;

        // Camera setup
        CameraComponent* camera = app.GetRenderCamera();
        glm::mat4 vp = camera->GetVP(glm::radians(70.f), vkExtent.width / (float)vkExtent.height, 1.f, 1000000.f);

        // Draw models
        auto pipelineLayout = renderer->BindGraphicsPipeline(commandBuffer, drawMode);
        {
            auto view = scene->m_Registry.view<TransformComponent, MeshComponent, TextureComponent>();
            for (auto [entity, transform, mesh, texture] : view.each()) {
                MeshPushConstants constants;
                glm::mat4 modelMat = transform.GetTransform();
                constants.MVP = vp * modelMat;
                constants.InvModelMat = glm::inverse(modelMat);
                vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT,
                    0, sizeof(MeshPushConstants), &constants);

                texture.Bind(commandBuffer, pipelineLayout);
                mesh.Draw(commandBuffer);
            }
        }

        auto view = scene->m_Registry.view<TransformComponent, StaticMeshComponent, TextureComponent>();
        for (auto [entity, transform, staticMesh, texture] : view.each()) {
            MeshPushConstants constants;
            glm::mat4 modelMat = transform.GetTransform();
            constants.MVP = vp * modelMat;
            constants.InvModelMat = glm::inverse(modelMat);
            vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT,
                0, sizeof(MeshPushConstants), &constants);
            texture.Bind(commandBuffer, pipelineLayout);
            for (auto& mesh : *staticMesh.meshes) {
                VkDeviceSize offset{ 0 };
                vkCmdBindVertexBuffers(commandBuffer, 0, 1, &mesh.VertexBuffer.Buffer, &offset);
                if (mesh.IndexBuffer.Buffer != VK_NULL_HANDLE) {
                    vkCmdBindIndexBuffer(commandBuffer, mesh.IndexBuffer.Buffer, 0, VK_INDEX_TYPE_UINT32);
                    vkCmdDrawIndexed(commandBuffer, mesh.IndexCount,
                        1, 0, 0, 0);
                } else {
                    vkCmdDraw(commandBuffer, mesh.VertexCount, 1, 0, 0);
                }
            }
        }
        // Draw debug lines
        auto* physicDrawer = app.GetPhysicsSystemDrawer();
        if (physicDrawer) {
            auto pipelineLayout = renderer->BindGraphicsPipeline(commandBuffer, RenderPipelineKeys::Line);
            auto* lineMesh = physicDrawer->GetUpdatedLineMesh();
            ColorPushConstants constants;
            constants.MVP = vp;
            vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(ColorPushConstants), &constants);
            lineMesh->Draw(commandBuffer);
        }

        // Draw wireframe for selected object
        if (auto* meshComponent = scene->m_Registry.try_get<MeshComponent>(scene->m_SelectedEntity)) {
            auto& transform = scene->m_Registry.get<TransformComponent>(scene->m_SelectedEntity);

            auto pipelineLayout = renderer->BindGraphicsPipeline(commandBuffer, RenderPipelineKeys::Wireframe);

            MeshPushConstants constants;
            glm::mat4 modelMat = transform.GetTransform();
            constants.MVP = vp * modelMat;
            constants.InvModelMat = glm::inverse(modelMat);
            vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT,
                0, sizeof(MeshPushConstants), &constants);

            meshComponent->Draw(commandBuffer);
        }

        // End main renderpass
        renderer->EndRenderPass(commandBuffer);

        VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = &waitSemaphore;
        submitInfo.pWaitDstStageMask = waitStages;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffer;
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = &signalSemaphore;

        renderer->QueueSubmitGraphics(1, &submitInfo);

        return m_TextureFrameBuffers[frameIndex].Descriptor;
    }

    void ForwardSceneRenderer::CreateTextureRenderer() {
        m_TextureFrameBuffers.resize(VulkanGlobals::MAX_FRAMES_IN_FLIGHT);
        CreateRenderPass();
        auto* renderer = VulkanRenderer::Get();
        {	// Default light shader
            RenderPipelineParams params;
            params.Flags = RenderPipelineFlags::AlphaBlend | RenderPipelineFlags::DepthPass;
            params.FragmentPath = "Shaders/ForwardLit.frag.spv";
            params.VertexPath = "Shaders/ForwardLit.vert.spv";
            params.Key = RenderPipelineKeys::ForwardLit;
            params.PolygonMode = VK_POLYGON_MODE_FILL;
            params.Topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
            params.BindingDescription = MeshVertex::GetBindingDescription();
            params.AttributeDescriptions = MeshVertex::GetAttributeDescriptions();
            params.PushConstantSize = sizeof(MeshPushConstants);
            params.DescriptorSetLayoutBindings.resize(1);
            params.DescriptorSetLayoutBindings[0] = renderer->m_DescriptorSetLayouts[RenderSetLayouts::DiffuseTexture];
            params.Renderpass = m_RenderPass;
            renderer->CreateGraphicsPipeline(params);
        }
        {	// Wireframe
            RenderPipelineParams params;
            params.Flags = RenderPipelineFlags::AlphaBlend;
            params.FragmentPath = "Shaders/Wireframe.frag.spv";
            params.VertexPath = "Shaders/Wireframe.vert.spv";
            params.Key = RenderPipelineKeys::Wireframe;
            params.PolygonMode = VK_POLYGON_MODE_LINE;
            params.Topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
            params.BindingDescription = MeshVertex::GetBindingDescription();
            params.AttributeDescriptions = MeshVertex::GetAttributeDescriptions();
            params.PushConstantSize = sizeof(MeshPushConstants);
            params.Renderpass = m_RenderPass;
            renderer->CreateGraphicsPipeline(params);
        }
        {	// Line drawing shader
            RenderPipelineParams params;
            params.Flags = RenderPipelineFlags::AlphaBlend;
            params.FragmentPath = "Shaders/Color.frag.spv";
            params.VertexPath = "Shaders/Color.vert.spv";
            params.Key = RenderPipelineKeys::Line;
            params.PolygonMode = VK_POLYGON_MODE_LINE;
            params.Topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
            params.BindingDescription = ColorVertex::GetBindingDescription();
            params.AttributeDescriptions = ColorVertex::GetAttributeDescriptions();
            params.PushConstantSize = sizeof(ColorPushConstants);
            params.Renderpass = m_RenderPass;
            renderer->CreateGraphicsPipeline(params);
        }
    }

    void ForwardSceneRenderer::DestroyTextureRenderer() {
        auto* renderer = VulkanRenderer::Get();

        DestoryFrameBuffers();
        DestoryDepthBuffer();
        DestroyRenderPass();
    }

    void ForwardSceneRenderer::ResizeBuffers(glm::vec2 extent) {
        std::cout << "Forward renderer: Resizing texture buffers.\n";

        m_Extent = extent;

        auto* renderer = VulkanRenderer::Get();
        renderer->FinishAllFrames();

        DestoryFrameBuffers();
        DestoryDepthBuffer();

        CreateDepthBuffer();
        CreateFrameBuffers();
    }

    void ForwardSceneRenderer::CreateRenderPass() {
        auto* renderer = VulkanRenderer::Get();
        auto* window = renderer->GetVulkanWindow();
        m_DepthFormat = renderer->FindDepthFormat();

        VkAttachmentDescription colorAttachments[2]{};
        colorAttachments[0].format = window->SurfaceFormat.format;
        colorAttachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
        colorAttachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAttachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        colorAttachments[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        colorAttachments[0].finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        colorAttachments[1].format = m_DepthFormat;
        colorAttachments[1].samples = VK_SAMPLE_COUNT_1_BIT;
        colorAttachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAttachments[1].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        colorAttachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        colorAttachments[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        colorAttachments[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        VkAttachmentReference colorAttachmentRef{};
        colorAttachmentRef.attachment = 0;
        colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkAttachmentReference depthStencilAttachmentRef{};
        depthStencilAttachmentRef.attachment = 1;
        depthStencilAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        VkSubpassDescription subpass{};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &colorAttachmentRef;
        subpass.pDepthStencilAttachment = &depthStencilAttachmentRef;

        // Use subpass dependencies for layout transitions
        std::array<VkSubpassDependency, 2> dependencies;

        dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
        dependencies[0].dstSubpass = 0;
        dependencies[0].srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependencies[0].srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
        dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

        dependencies[1].srcSubpass = 0;
        dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
        dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependencies[1].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        dependencies[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

        VkRenderPassCreateInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassInfo.attachmentCount = 2;
        renderPassInfo.pAttachments = colorAttachments;
        renderPassInfo.subpassCount = 1;
        renderPassInfo.pSubpasses = &subpass;
        renderPassInfo.dependencyCount = dependencies.size();
        renderPassInfo.pDependencies = dependencies.data();

        VkResult result = vkCreateRenderPass(renderer->m_LogicalDevice, &renderPassInfo, nullptr, &m_RenderPass);
        ASSERT(result == VK_SUCCESS);
        LOG("Forward renderer: Render pass created.\n");
    }

    void ForwardSceneRenderer::CreateDepthBuffer() {
        auto* renderer = VulkanRenderer::Get();

        ASSERT(m_DepthFormat != VK_FORMAT_UNDEFINED);

        VkImageCreateInfo depthImageInfo = { VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
        depthImageInfo.imageType = VK_IMAGE_TYPE_2D;
        depthImageInfo.extent.width = m_Extent.x;
        depthImageInfo.extent.height = m_Extent.y;
        depthImageInfo.extent.depth = 1;
        depthImageInfo.mipLevels = 1;
        depthImageInfo.arrayLayers = 1;
        depthImageInfo.format = m_DepthFormat;
        depthImageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        depthImageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        depthImageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
        depthImageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        depthImageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        depthImageInfo.flags = 0;

        VmaAllocationCreateInfo depthImageAllocCreateInfo = {};
        depthImageAllocCreateInfo.usage = VMA_MEMORY_USAGE_AUTO;

        VkImageViewCreateInfo depthImageViewInfo = { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
        depthImageViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        depthImageViewInfo.format = m_DepthFormat;
        depthImageViewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        depthImageViewInfo.subresourceRange.baseMipLevel = 0;
        depthImageViewInfo.subresourceRange.levelCount = 1;
        depthImageViewInfo.subresourceRange.baseArrayLayer = 0;
        depthImageViewInfo.subresourceRange.layerCount = 1;

        vmaCreateImage(renderer->m_Allocator, &depthImageInfo, &depthImageAllocCreateInfo,
            &m_DepthBuffer.Image,
            &m_DepthBuffer.Allocation,
            &m_DepthBuffer.AllocationInfo);

        depthImageViewInfo.image = m_DepthBuffer.Image;

        auto result = vkCreateImageView(renderer->m_LogicalDevice, &depthImageViewInfo, nullptr,
            &m_DepthBufferImageView);
        ASSERT(result == VK_SUCCESS);
    }

    void ForwardSceneRenderer::CreateFrameBuffers() {
        auto* renderer = VulkanRenderer::Get();

        CreateFrameBufferTextures();

        for (auto& textureFrameBuffer : m_TextureFrameBuffers) {
            VkImageView attachments[] = {
                textureFrameBuffer.Texture.ImageView,
                m_DepthBufferImageView,
            };

            VkFramebufferCreateInfo framebufferInfo{};
            framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            framebufferInfo.renderPass = m_RenderPass;
            framebufferInfo.attachmentCount = 2;
            framebufferInfo.pAttachments = attachments;
            framebufferInfo.width = m_Extent.x;
            framebufferInfo.height = m_Extent.y;
            framebufferInfo.layers = 1;

            VkResult result = vkCreateFramebuffer(renderer->m_LogicalDevice, &framebufferInfo, nullptr, &textureFrameBuffer.FrameBuffer);
            ASSERT(result == VK_SUCCESS);
        }
        LOG("Forward renderer: Framebuffers created.\n");
    }

    void ForwardSceneRenderer::CreateFrameBufferTextures() {
        auto* renderer = VulkanRenderer::Get();
        auto* window = renderer->GetVulkanWindow();

        VkFormat format = window->SurfaceFormat.format;

        uint32_t xWidth = m_Extent.x;
        uint32_t yHeight = m_Extent.y;

        for (auto& fbTexture : m_TextureFrameBuffers) {
            // Image
            VkImageCreateInfo imageInfo = { VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
            imageInfo.imageType = VK_IMAGE_TYPE_2D;
            imageInfo.extent.width = xWidth;
            imageInfo.extent.height = yHeight;
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

            vmaCreateImage(renderer->m_Allocator, &imageInfo, &imageAllocCreateInfo, &fbTexture.Texture.Image,
                &fbTexture.Texture.Allocation, &fbTexture.Texture.AllocationInfo);

            // create image view
            VkImageViewCreateInfo textureImageViewInfo = { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
            textureImageViewInfo.image = fbTexture.Texture.Image;
            textureImageViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            textureImageViewInfo.format = format;
            textureImageViewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            textureImageViewInfo.subresourceRange.baseMipLevel = 0;
            textureImageViewInfo.subresourceRange.levelCount = 1;
            textureImageViewInfo.subresourceRange.baseArrayLayer = 0;
            textureImageViewInfo.subresourceRange.layerCount = 1;
            vkCreateImageView(renderer->m_LogicalDevice, &textureImageViewInfo, nullptr, &fbTexture.Texture.ImageView);

            // Get descriptor set and point it to data.
            fbTexture.Descriptor = renderer->AllocateTextureDescriptorSet(renderer->m_DescriptorSetLayouts[RenderSetLayouts::DiffuseTexture]);

            VkSampler sampler = renderer->GetFontSampler();

            VkDescriptorImageInfo descriptorImageInfo{};
            descriptorImageInfo.sampler = sampler;
            descriptorImageInfo.imageView = fbTexture.Texture.ImageView;
            descriptorImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

            VkWriteDescriptorSet writeDescriptorSet{};
            writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            writeDescriptorSet.dstSet = fbTexture.Descriptor;
            writeDescriptorSet.dstBinding = 0;
            writeDescriptorSet.descriptorCount = 1;
            writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            writeDescriptorSet.pImageInfo = &descriptorImageInfo;

            vkUpdateDescriptorSets(renderer->m_LogicalDevice, 1, &writeDescriptorSet, 0, nullptr);
        }
    } 

    void ForwardSceneRenderer::CreateCommandPool() {
    }

    void ForwardSceneRenderer::AllocateCommandBuffers() {
    }

    void ForwardSceneRenderer::CreateSyncObjects() {
    }

    void ForwardSceneRenderer::DestroyRenderPass() {
        auto* renderer = VulkanRenderer::Get();
        vkDestroyRenderPass(renderer->m_LogicalDevice, m_RenderPass, nullptr);
    }

    void ForwardSceneRenderer::DestoryFrameBuffers() {
        auto* renderer = VulkanRenderer::Get();

        for (auto& textureFrameBuffer : m_TextureFrameBuffers) {
            vkDestroyFramebuffer(renderer->m_LogicalDevice, textureFrameBuffer.FrameBuffer, nullptr);
            vkDestroyImageView(renderer->m_LogicalDevice, textureFrameBuffer.Texture.ImageView, nullptr);
            vmaDestroyImage(renderer->m_Allocator, textureFrameBuffer.Texture.Image, textureFrameBuffer.Texture.Allocation);
            renderer->FreeTextureDescriptorSet(textureFrameBuffer.Descriptor);

            textureFrameBuffer.FrameBuffer = VK_NULL_HANDLE;
            textureFrameBuffer.Texture.ImageView = VK_NULL_HANDLE;
            textureFrameBuffer.Texture.Image = VK_NULL_HANDLE;
            textureFrameBuffer.Descriptor = VK_NULL_HANDLE;
        }
    }

    void ForwardSceneRenderer::DestoryRenderPipeline() {
    }

    void ForwardSceneRenderer::DestoryCommandPool() {
    }

    void ForwardSceneRenderer::DestroySyncObjects() {
    }

    void ForwardSceneRenderer::DestoryDepthBuffer() {
        auto* renderer = VulkanRenderer::Get();

        vkDestroyImageView(renderer->m_LogicalDevice, m_DepthBufferImageView, nullptr);
        m_DepthBufferImageView = VK_NULL_HANDLE;
        vmaDestroyImage(renderer->m_Allocator, m_DepthBuffer.Image, m_DepthBuffer.Allocation);
        m_DepthBuffer.Image = VK_NULL_HANDLE;
    }
}