#pragma once

#include <unordered_map>
#include "VulkanBuffer.h"
#include "Skybox.h"
#include "IBL.h"
#include "../Components.h"

namespace FLOOF {
    struct SceneFrameData {
        glm::vec4 CameraPos = glm::vec4(0.f);
        glm::vec4 SunDirection = glm::vec4(0.0f, 1.0f, 2.0f, 1.f);
        glm::vec4 SunColor = glm::vec4(1.0f, 1.0f, 1.0f, 1.f);
        glm::mat4 VP = glm::mat4(1.f);
        glm::mat4 LightSpaceMatrix = glm::mat4(1.f);
        float sunStrenght = 15.f;
        int LightCount = 0;
    };
    
    struct SceneRenderFinishedData {
        VkDescriptorSet Texture = VK_NULL_HANDLE;
        VkSemaphore SceneRenderFinishedSemaphore = VK_NULL_HANDLE;
    };

    struct TextureFrameBuffer {
        VulkanTexture VKTexture;
        VkFramebuffer FrameBuffer = VK_NULL_HANDLE;
    };

    class Scene;
    class PhysicsDebugDraw;
    class SceneRenderer {
        friend class RendererPanel;
    public:
        SceneRenderer();
        ~SceneRenderer();
        SceneRenderFinishedData RenderToTexture(Scene* scene, glm::vec2 extent, CameraComponent* camera, RenderPipelineKeys drawMode,
            PhysicsDebugDraw* physicDrawer, VkSemaphore waitSemaphore);

    private:
        void ShadowPass(VkCommandBuffer commandBuffer, Scene* scene, CameraComponent* camera);

        void CreateTextureRenderer();
        void DestroyTextureRenderer();

        void ResizeBuffers(glm::vec2 extent);

        void CreateRenderPass();
        void CreateDepthBuffer();
        void CreateResolveBuffer();
        void CreateFrameBufferTextures();
        void CreateFrameBuffers();
        void CreateSyncObjects();

        void DestroyRenderPass();
        void DestoryDepthBuffer();
        void DestroyResolveBuffer();
        void DestoryFrameBuffers();
        void DestroySyncObjects();

        VkRenderPass m_RenderPass;

        uint32_t m_ShadowRes = 1024;
        std::vector<std::unique_ptr<DepthFramebuffer>> m_ShadowDepthBuffers;
        std::vector<TextureFrameBuffer> m_TextureFrameBuffers;

        std::vector<VkSemaphore> m_waitSemaphores;
        std::vector<VkSemaphore> m_SignalSemaphores;      

        VulkanImageData m_ResolveBuffer{};
        VkImageView m_ResolveImageView = VK_NULL_HANDLE;

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