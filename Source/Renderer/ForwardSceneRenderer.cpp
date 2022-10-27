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

    void ForwardSceneRenderer::Render(entt::registry& registry) {
        auto m_Renderer = VulkanRenderer::Get();
        auto& app = Application::Get();
        auto* vulkanWindow = m_Renderer->GetVulkanWindow();
        auto& currentFrameData = vulkanWindow->Frames[vulkanWindow->FrameIndex];

        m_Renderer->StartRenderPass(
            currentFrameData.MainCommandBuffer,
            m_Renderer->GetMainRenderPass(),
            vulkanWindow->FrameBuffers[vulkanWindow->ImageIndex],
            vulkanWindow->Extent
        );
        auto commandBuffer = vulkanWindow->Frames[vulkanWindow->FrameIndex].MainCommandBuffer;
        auto drawMode = app.GetDrawMode();

        // Camera setup
        auto extent = m_Renderer->GetExtent();
        CameraComponent* camera = app.GetRenderCamera();
        glm::mat4 vp = camera->GetVP(glm::radians(70.f), extent.width / (float)extent.height, 1.f, 1000000.f);
        
        // Draw models
        auto pipelineLayout = m_Renderer->BindGraphicsPipeline(commandBuffer, drawMode);
        {
            auto view = registry.view<TransformComponent, MeshComponent, TextureComponent>();
            for (auto [entity, transform, mesh, texture] : view.each()) {
                MeshPushConstants constants;
                glm::mat4 modelMat = transform.GetTransform();
                constants.MVP = vp * modelMat;
                constants.InvModelMat = glm::inverse(modelMat);
                vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT,
                    0, sizeof(MeshPushConstants), &constants);

                texture.Bind(commandBuffer);
                mesh.Draw(commandBuffer);
            }
        }

        auto view = registry.view<TransformComponent, StaticMeshComponent, TextureComponent>();
        for (auto [entity, transform, staticMesh, texture] : view.each()) {
            MeshPushConstants constants;
            glm::mat4 modelMat = transform.GetTransform();
            constants.MVP = vp * modelMat;
            constants.InvModelMat = glm::inverse(modelMat);
            vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT,
                0, sizeof(MeshPushConstants), &constants);
            texture.Bind(commandBuffer);
            for (auto& mesh : staticMesh.meshes)
            {
                VkDeviceSize offset{ 0 };
                vkCmdBindVertexBuffers(commandBuffer, 0, 1, &mesh.VertexBuffer.Buffer, &offset);
                if (mesh.IndexBuffer.Buffer != VK_NULL_HANDLE) {
                    vkCmdBindIndexBuffer(commandBuffer, mesh.IndexBuffer.Buffer, 0, VK_INDEX_TYPE_UINT32);
                    vkCmdDrawIndexed(commandBuffer, mesh.IndexCount,
                        1, 0, 0, 0);
                }
                else {
                    vkCmdDraw(commandBuffer, mesh.VertexCount, 1, 0, 0);
                }
            }            
        }
        // Draw debug lines
        auto* physicDrawer = app.GetPhysicsSystemDrawer();
        if (physicDrawer) {
            auto pipelineLayout = m_Renderer->BindGraphicsPipeline(commandBuffer, RenderPipelineKeys::Line);
            auto* lineMesh = physicDrawer->GetUpdatedLineMesh();
            ColorPushConstants constants;
            constants.MVP = vp;
            vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT,0, sizeof(ColorPushConstants), &constants);
            lineMesh->Draw(commandBuffer);
        }

        // End main renderpass
        VulkanSubmitInfo submitInfo{};
        submitInfo.CommandBuffer = commandBuffer;
        submitInfo.WaitSemaphore = currentFrameData.ImageAvailableSemaphore;
        submitInfo.SignalSemaphore = currentFrameData.MainPassEndSemaphore;
        submitInfo.WaitStages = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        m_Renderer->EndRenderPass(submitInfo);
    }

    VkDescriptorSet ForwardSceneRenderer::RenderToTexture(entt::registry& registry, glm::vec2 extent) {
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

        renderer->StartRenderPass(
            commandBuffer,
            m_RenderPass,
            m_TextureFrameBuffers[frameIndex].FrameBuffer,
            vkExtent
        );
        auto drawMode = app.GetDrawMode();

        // Camera setup
        CameraComponent* camera = app.GetRenderCamera();
        glm::mat4 vp = camera->GetVP(glm::radians(70.f), vkExtent.width / (float)vkExtent.height, 1.f, 1000000.f);

        // Draw models
        auto pipelineLayout = BindRenderPipeline(commandBuffer, drawMode);
        {
            auto view = registry.view<TransformComponent, MeshComponent, TextureComponent>();
            for (auto [entity, transform, mesh, texture] : view.each()) {
                MeshPushConstants constants;
                glm::mat4 modelMat = transform.GetTransform();
                constants.MVP = vp * modelMat;
                constants.InvModelMat = glm::inverse(modelMat);
                vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT,
                    0, sizeof(MeshPushConstants), &constants);

                texture.Bind(commandBuffer);
                mesh.Draw(commandBuffer);
            }
        }

        auto view = registry.view<TransformComponent, StaticMeshComponent, TextureComponent>();
        for (auto [entity, transform, staticMesh, texture] : view.each()) {
            MeshPushConstants constants;
            glm::mat4 modelMat = transform.GetTransform();
            constants.MVP = vp * modelMat;
            constants.InvModelMat = glm::inverse(modelMat);
            vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT,
                0, sizeof(MeshPushConstants), &constants);
            texture.Bind(commandBuffer);
            for (auto& mesh : staticMesh.meshes) {
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
            auto pipelineLayout = BindRenderPipeline(commandBuffer, RenderPipelineKeys::Line);
            auto* lineMesh = physicDrawer->GetUpdatedLineMesh();
            ColorPushConstants constants;
            constants.MVP = vp;
            vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(ColorPushConstants), &constants);
            lineMesh->Draw(commandBuffer);
        }

        // End main renderpass
        VulkanSubmitInfo submitInfo{};
        submitInfo.CommandBuffer = commandBuffer;
        submitInfo.WaitSemaphore = waitSemaphore;
        submitInfo.SignalSemaphore = signalSemaphore;
        submitInfo.WaitStages = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        renderer->EndRenderPass(submitInfo);

        return m_TextureFrameBuffers[frameIndex].Descriptor;
    }

    void ForwardSceneRenderer::CreateTextureRenderer() {
        m_TextureFrameBuffers.resize(VulkanGlobals::MAX_FRAMES_IN_FLIGHT);
        CreateRenderPass();
        {	// Default light shader
            RenderPipelineParams params;
            params.Flags = RenderPipelineFlags::AlphaBlend | RenderPipelineFlags::DepthPass;
            params.FragmentPath = "Shaders/Basic.frag.spv";
            params.VertexPath = "Shaders/Basic.vert.spv";
            params.Key = RenderPipelineKeys::Basic;
            params.PolygonMode = VK_POLYGON_MODE_FILL;
            params.Topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
            params.BindingDescription = MeshVertex::GetBindingDescription();
            params.AttributeDescriptions = MeshVertex::GetAttributeDescriptions();
            params.PushConstantSize = sizeof(MeshPushConstants);
            params.DescriptorSetLayoutBindings.resize(1);
            params.DescriptorSetLayoutBindings[0].binding = 0;
            params.DescriptorSetLayoutBindings[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            params.DescriptorSetLayoutBindings[0].descriptorCount = 1;
            params.DescriptorSetLayoutBindings[0].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
            CreateRenderPipeline(params);
        }
        {	// Wireframe
            RenderPipelineParams params;
            params.Flags = RenderPipelineFlags::AlphaBlend;
            params.FragmentPath = "Shaders/Basic.frag.spv";
            params.VertexPath = "Shaders/Basic.vert.spv";
            params.Key = RenderPipelineKeys::Wireframe;
            params.PolygonMode = VK_POLYGON_MODE_LINE;
            params.Topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
            params.BindingDescription = MeshVertex::GetBindingDescription();
            params.AttributeDescriptions = MeshVertex::GetAttributeDescriptions();
            params.PushConstantSize = sizeof(MeshPushConstants);
            params.DescriptorSetLayoutBindings.resize(1);
            params.DescriptorSetLayoutBindings[0].binding = 0;
            params.DescriptorSetLayoutBindings[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            params.DescriptorSetLayoutBindings[0].descriptorCount = 1;
            params.DescriptorSetLayoutBindings[0].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
            CreateRenderPipeline(params);
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
            CreateRenderPipeline(params);
        }
    }

    void ForwardSceneRenderer::DestroyTextureRenderer() {
        auto* renderer = VulkanRenderer::Get();

        DestoryFrameBuffers();
        DestoryDepthBuffer();

        for (auto& [key, val] : m_DescriptorSetLayouts) {
            vkDestroyDescriptorSetLayout(renderer->m_LogicalDevice, val, nullptr);
        }
        for (auto& [key, val] : m_GraphicsPipelines) {
            vkDestroyPipeline(renderer->m_LogicalDevice, val, nullptr);
        }
        for (auto& [key, val] : m_PipelineLayouts) {
            vkDestroyPipelineLayout(renderer->m_LogicalDevice, val, nullptr);
        }

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

    VkPipelineLayout ForwardSceneRenderer::BindRenderPipeline(VkCommandBuffer cmdBuffer, RenderPipelineKeys key) {
        vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_GraphicsPipelines[key]);
        return m_PipelineLayouts[key];
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
        colorAttachments[0].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

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

        VkSubpassDependency dependency{};
        dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        dependency.dstSubpass = 0;
        dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        dependency.srcAccessMask = 0;
        dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

        VkRenderPassCreateInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassInfo.attachmentCount = 2;
        renderPassInfo.pAttachments = colorAttachments;
        renderPassInfo.subpassCount = 1;
        renderPassInfo.pSubpasses = &subpass;
        renderPassInfo.dependencyCount = 1;
        renderPassInfo.pDependencies = &dependency;

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

        CreateFrameTextures();

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

    void ForwardSceneRenderer::CreateRenderPipeline(const RenderPipelineParams& params) {
        auto* renderer = VulkanRenderer::Get();

        auto vertShader = renderer->MakeShaderModule(params.VertexPath.c_str());
        auto fragShader = renderer->MakeShaderModule(params.FragmentPath.c_str());

        VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
        vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
        vertShaderStageInfo.module = vertShader;
        vertShaderStageInfo.pName = "main";

        VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
        fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        fragShaderStageInfo.module = fragShader;
        fragShaderStageInfo.pName = "main";

        VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

        VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
        vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertexInputInfo.vertexBindingDescriptionCount = 1;
        vertexInputInfo.pVertexBindingDescriptions = &params.BindingDescription;
        vertexInputInfo.vertexAttributeDescriptionCount = params.AttributeDescriptions.size();
        vertexInputInfo.pVertexAttributeDescriptions = params.AttributeDescriptions.data();

        VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
        inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssembly.topology = params.Topology;
        inputAssembly.primitiveRestartEnable = VK_FALSE;

        std::vector<VkDynamicState> dynamicStates = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
        };

        VkPipelineDynamicStateCreateInfo dynamicState{};
        dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
        dynamicState.pDynamicStates = dynamicStates.data();

        VkPipelineViewportStateCreateInfo viewportState{};
        viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportState.viewportCount = 1;
        viewportState.scissorCount = 1;

        VkPipelineRasterizationStateCreateInfo rasterizer{};
        rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizer.depthClampEnable = VK_FALSE;
        rasterizer.rasterizerDiscardEnable = VK_FALSE;
        rasterizer.polygonMode = params.PolygonMode;
        rasterizer.lineWidth = 1.0f;
        rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
        rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
        rasterizer.depthBiasEnable = VK_FALSE;
        rasterizer.depthBiasConstantFactor = 0.0f; // Optional
        rasterizer.depthBiasClamp = 0.0f; // Optional
        rasterizer.depthBiasSlopeFactor = 0.0f; // Optional

        VkPipelineMultisampleStateCreateInfo multisampling{};
        multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

        VkPipelineColorBlendAttachmentState colorBlendAttachment{};
        colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        if (params.Flags & RenderPipelineFlags::AlphaBlend) {
            colorBlendAttachment.blendEnable = VK_TRUE;
        } else {
            colorBlendAttachment.blendEnable = VK_FALSE;
        }
        colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
        colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
        colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD; // Optional
        colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
        colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
        colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD; // Optional

        VkPipelineColorBlendStateCreateInfo colorBlending{};
        colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        colorBlending.logicOpEnable = VK_FALSE;
        colorBlending.logicOp = VK_LOGIC_OP_COPY; // Optional
        colorBlending.attachmentCount = 1;
        colorBlending.pAttachments = &colorBlendAttachment;
        colorBlending.blendConstants[0] = 0.0f; // Optional
        colorBlending.blendConstants[1] = 0.0f; // Optional
        colorBlending.blendConstants[2] = 0.0f; // Optional
        colorBlending.blendConstants[3] = 0.0f; // Optional

        VkPushConstantRange pushConstants{};
        pushConstants.offset = 0;
        pushConstants.size = params.PushConstantSize;
        pushConstants.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        if (params.DescriptorSetLayoutBindings.size() != 0) {
            VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo{};
            descriptorSetLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
            //descriptorSetLayoutCreateInfo.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT;
            descriptorSetLayoutCreateInfo.bindingCount = params.DescriptorSetLayoutBindings.size();
            descriptorSetLayoutCreateInfo.pBindings = params.DescriptorSetLayoutBindings.data();

            vkCreateDescriptorSetLayout(renderer->m_LogicalDevice, &descriptorSetLayoutCreateInfo, nullptr, &m_DescriptorSetLayouts[params.Key]);

            pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
            pipelineLayoutInfo.setLayoutCount = 1;
            pipelineLayoutInfo.pSetLayouts = &m_DescriptorSetLayouts[params.Key];
            pipelineLayoutInfo.pushConstantRangeCount = 1;
            pipelineLayoutInfo.pPushConstantRanges = &pushConstants;
        } else {
            pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
            pipelineLayoutInfo.setLayoutCount = 0;
            pipelineLayoutInfo.pSetLayouts = nullptr;
            pipelineLayoutInfo.pushConstantRangeCount = 1;
            pipelineLayoutInfo.pPushConstantRanges = &pushConstants;
        }

        VkResult plResult = vkCreatePipelineLayout(renderer->m_LogicalDevice, &pipelineLayoutInfo, nullptr, &m_PipelineLayouts[params.Key]);
        ASSERT(plResult == VK_SUCCESS);

        VkPipelineDepthStencilStateCreateInfo depthStencilStateInfo = { VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO };
        if (params.Flags & RenderPipelineFlags::DepthPass) {
            depthStencilStateInfo.depthTestEnable = VK_TRUE;
            depthStencilStateInfo.depthWriteEnable = VK_TRUE;
        } else {
            depthStencilStateInfo.depthTestEnable = VK_FALSE;
            depthStencilStateInfo.depthWriteEnable = VK_FALSE;
        }
        depthStencilStateInfo.depthCompareOp = VK_COMPARE_OP_LESS;
        depthStencilStateInfo.depthBoundsTestEnable = VK_FALSE;
        depthStencilStateInfo.stencilTestEnable = VK_FALSE;

        VkGraphicsPipelineCreateInfo pipelineInfo{};
        pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineInfo.stageCount = 2;
        pipelineInfo.pStages = shaderStages;
        pipelineInfo.pVertexInputState = &vertexInputInfo;
        pipelineInfo.pInputAssemblyState = &inputAssembly;
        pipelineInfo.pViewportState = &viewportState;
        pipelineInfo.pRasterizationState = &rasterizer;
        pipelineInfo.pMultisampleState = &multisampling;
        pipelineInfo.pDepthStencilState = &depthStencilStateInfo;
        pipelineInfo.pColorBlendState = &colorBlending;
        pipelineInfo.pDynamicState = &dynamicState;
        pipelineInfo.layout = m_PipelineLayouts[params.Key];
        pipelineInfo.renderPass = m_RenderPass;
        pipelineInfo.subpass = 0;
        pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
        pipelineInfo.basePipelineIndex = -1; // Optional

        VkResult gplResult = vkCreateGraphicsPipelines(renderer->m_LogicalDevice, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_GraphicsPipelines[params.Key]);
        ASSERT(gplResult == VK_SUCCESS);

        vkDestroyShaderModule(renderer->m_LogicalDevice, vertShader, nullptr);
        vkDestroyShaderModule(renderer->m_LogicalDevice, fragShader, nullptr);
        LOG("Render pipeline created.\n");
    }

    void ForwardSceneRenderer::CreatePipelineLayout() {
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

    void ForwardSceneRenderer::CreateFrameTextures() {
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

            VkSamplerCreateInfo samplerInfo = {};
            samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
            samplerInfo.magFilter = VK_FILTER_LINEAR;
            samplerInfo.minFilter = VK_FILTER_LINEAR;
            samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
            samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
            samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
            samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
            samplerInfo.minLod = -1000;
            samplerInfo.maxLod = 1000;
            samplerInfo.maxAnisotropy = 1.0f;
            vkCreateSampler(renderer->m_LogicalDevice, &samplerInfo, nullptr, &fbTexture.Texture.Sampler);

            // Get descriptor set and point it to data.
            fbTexture.Descriptor = renderer->AllocateTextureDescriptorSet();

            VkDescriptorImageInfo descriptorImageInfo{};
            descriptorImageInfo.sampler = fbTexture.Texture.Sampler;
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
}