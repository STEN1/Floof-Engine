#pragma once

#include <unordered_map>
#include "VulkanBuffer.h"
#include "Skybox.h"
#include "IBL.h"
#include "../Components.h"

namespace FLOOF {
#define FLOOF_CASCADE_COUNT 4
    struct SceneFrameData {
        glm::vec4 CameraPos = glm::vec4(0.f);
        glm::vec4 SunPosition = glm::vec4(0.1f, 1.f, 0.1f, 1.f);
        glm::vec4 SunColor = glm::vec4(1.0f, 1.0f, 1.0f, 1.f);
        glm::mat4 VP = glm::mat4(1.f);
        glm::mat4 LightSpaceMatrix[FLOOF_CASCADE_COUNT];
        glm::mat4 View = glm::mat4(1.f);
        glm::vec4 SplitDists = glm::vec4(0.f);
        float sunStrenght = 15.f;
        int LightCount = 0;
        int CascadeCount = FLOOF_CASCADE_COUNT;
        float AmbientIntensity = 1.f;
        float Bias = 0.00005f;
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
        VkSemaphore ShadowPass(VkSemaphore waitSemaphore, Scene* scene, CameraComponent* camera);

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

        uint32_t m_ShadowRes = 4096;
        float m_ShadowZExtentOffset = 1000.f;
        std::vector<std::unique_ptr<DepthFramebuffer>> m_ShadowDepthBuffers;
        float m_ShadowFarClip = 5000.f;

        std::vector<TextureFrameBuffer> m_TextureFrameBuffers;

        std::vector<VkSemaphore> m_ShadowPassSignalSemaphores;
        std::vector<VkSemaphore> m_MainPassSignalSemaphores;      

        VulkanImageData m_ResolveBuffer{};
        VkImageView m_ResolveImageView = VK_NULL_HANDLE;

        VkFormat m_DepthFormat{};
        std::vector<VulkanImageData> m_DepthBuffers;
        std::vector<VkImageView> m_DepthBufferImageViews;

        glm::vec2 m_Extent{ 0.f, 0.f };

        SceneFrameData m_SceneFrameData;
        std::vector<VulkanUBO<SceneFrameData>> m_SceneDataUBO{};
        VulkanSSBO<PointLightComponent::PointLight> m_LightSSBO{};
        std::unique_ptr<Skybox> m_Skybox;
        VulkanTexture m_IrradienceMap;
        VulkanTexture m_PrefilterMap;
        VulkanTexture m_BRDFLut;

        std::vector<std::unique_ptr<LineMeshComponent>> m_DebugLineMesh;
        mutable std::vector<ColorVertex> m_DebugVertexData;
        void DebugDrawLine(const glm::vec3& start, const glm::vec3& end, glm::vec3 color) const;
        void DrawRect(const glm::vec3& bl, const glm::vec3& br, const glm::vec3& tl, const glm::vec3& tr, const glm::vec3& color) const;
        void DebugDrawFrustum(const glm::vec3& nbl, const glm::vec3& nbr, const glm::vec3& ntl, const glm::vec3& ntr,
            const glm::vec3& fbl, const glm::vec3& fbr, const glm::vec3& ftl, const glm::vec3& ftr, const glm::vec3& color) const;

        void DrawDebugCameraLines(CameraComponent* camera) const;
        void DrawDebugCameraLines();
        bool m_DrawDebugCameraLines = false;
    };
}