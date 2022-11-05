#pragma once

#include "SceneRenderer.h"
#include <unordered_map>
#include "VulkanBuffer.h"
#include "../Components.h"

namespace FLOOF {
    struct SceneFrameData {
        glm::vec3 CameraPos = glm::vec3(0.f);
        int LightCount = 0;
    };

    class ForwardSceneRenderer : public SceneRenderer {
    public:
        ForwardSceneRenderer();
        ~ForwardSceneRenderer();
        VkDescriptorSet RenderToTexture(std::shared_ptr<Scene> scene, glm::vec2 extent) override;

    private:
        void CreateTextureRenderer();
        void DestroyTextureRenderer();

        void ResizeBuffers(glm::vec2 extent);

        void CreateRenderPass();
        void CreateDepthBuffer();
        void CreateFrameBufferTextures();
        void CreateFrameBuffers();
        void CreateCommandPool();
        void AllocateCommandBuffers();
        void CreateSyncObjects();

        void DestroyRenderPass();
        void DestoryDepthBuffer();
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
        VulkanImageData m_DepthBuffer{};
        VkImageView m_DepthBufferImageView = VK_NULL_HANDLE;

        glm::vec2 m_Extent{ 0.f, 0.f };

        VulkanUBO<SceneFrameData> m_SceneDataUBO{};
        VulkanSSBO<PointLightComponent::PointLight> m_LightSSBO{};
    };
}