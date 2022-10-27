#pragma once

#include "SceneRenderer.h"
#include <unordered_map>

namespace FLOOF {
    class ForwardSceneRenderer : public SceneRenderer {
    public:
        ForwardSceneRenderer();
        ~ForwardSceneRenderer();
        void Render(entt::registry& registry) override;
        VkDescriptorSet RenderToTexture(entt::registry& registry, glm::vec2 extent) override;

    private:
        void CreateTextureRenderer();
        void DestroyTextureRenderer();

        void ResizeBuffers(glm::vec2 extent);

        VkPipelineLayout BindRenderPipeline(VkCommandBuffer cmdBuffer, RenderPipelineKeys Key);

        void CreateRenderPass();
        void CreateDepthBuffer();
        void CreateFrameTextures();
        void CreateFrameBuffers();
        void CreateRenderPipeline(const RenderPipelineParams& params);
        void CreatePipelineLayout();
        void CreateCommandPool();
        void AllocateCommandBuffers();
        void CreateSyncObjects();

        void DestroyRenderPass();
        void DestoryDepthBuffer();
        void DestroyFrameTextures();
        void DestoryFrameBuffers();
        void DestoryRenderPipeline();
        void DestoryCommandPool();
        void DestroySyncObjects();

        VkRenderPass m_RenderPass;

        struct TextureFrameBuffer {
            VulkanCombinedTextureSampler Texture;
            VkFramebuffer FrameBuffer = VK_NULL_HANDLE;
            VkDescriptorSet Descriptor;
        };

        std::vector<TextureFrameBuffer> m_TextureFrameBuffers;
        VkFormat m_DepthFormat{};
        VulkanImage m_DepthBuffer{};
        VkImageView m_DepthBufferImageView = VK_NULL_HANDLE;

        std::unordered_map<RenderPipelineKeys, VkDescriptorSetLayout> m_DescriptorSetLayouts;
        std::unordered_map<RenderPipelineKeys, VkPipelineLayout> m_PipelineLayouts;
        std::unordered_map<RenderPipelineKeys, VkPipeline> m_GraphicsPipelines;

        glm::vec2 m_Extent{ 0.f, 0.f };
    };
}