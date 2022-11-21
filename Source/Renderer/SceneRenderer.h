#pragma once

#include <unordered_map>
#include "VulkanBuffer.h"
#include "../Components.h"
#include "Skybox.h"
#include "IBL.h"

namespace FLOOF {
    struct SceneFrameData {
        glm::vec4 CameraPos = glm::vec4(0.f);
        glm::vec4 SunDirection = glm::vec4(0.0f, 1.0f, 2.0f, 1.f);
        glm::vec4 SunColor = glm::vec4(1.0f, 1.0f, 1.0f, 1.f);
        float sunStrenght = 15.f;
        int LightCount = 0;
    };

    class SceneRenderer {
        friend class RendererPanel;
    public:
        SceneRenderer();
        ~SceneRenderer();
        VkDescriptorSet RenderToTexture(std::shared_ptr<Scene> scene, glm::vec2 extent);

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
            VulkanTexture VKTexture;
            VkFramebuffer FrameBuffer = VK_NULL_HANDLE;
        };

        std::vector<TextureFrameBuffer> m_TextureFrameBuffers;
        VkFormat m_DepthFormat{};
        VulkanImageData m_DepthBuffer{};
        VkImageView m_DepthBufferImageView = VK_NULL_HANDLE;

        glm::vec2 m_Extent{ 0.f, 0.f };

        SceneFrameData m_SceneFrameData;
        VulkanUBO<SceneFrameData> m_SceneDataUBO{};
        VulkanSSBO<PointLightComponent::PointLight> m_LightSSBO{};
        std::unique_ptr<Skybox> m_Skybox;
        VulkanTexture m_IrradienceMap;
        VulkanTexture m_PrefilterMap;
        VulkanTexture m_BRDFLut;
    };
}