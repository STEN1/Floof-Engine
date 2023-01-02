#pragma once

#include <unordered_map>
#include "VulkanBuffer.h"
#include "Skybox.h"
#include "IBL.h"
#include "../CameraComponent.h"
#include "../PointLightComponent.h"
#include "../LineMeshComponent.h"
#include "Mesh.h"
#include <thread>

namespace FLOOF {
#define FLOOF_CASCADE_COUNT 4
    struct SceneFrameData {
        glm::vec4 CameraPos = glm::vec4(0.f);
        glm::vec4 SunPosition = glm::vec4(0.2f, 1.f, 1.2f, 1.f);
        glm::vec4 SunColor = glm::vec4(1.0f, 1.0f, 1.0f, 1.f);
        glm::mat4 VP = glm::mat4(1.f);
        glm::mat4 LightSpaceMatrix[FLOOF_CASCADE_COUNT];
        glm::mat4 View = glm::mat4(1.f);
        glm::vec4 SplitDists = glm::vec4(0.f);
        float sunStrenght = 25.f;
        int TileSize = 128;
        int CascadeCount = FLOOF_CASCADE_COUNT;
        float AmbientIntensity = 1.f;
        float Bias = 0.00005f;
        int TileCountX = 0;
        int ShowLightComplexity = 0;
    };
    
    struct DrawData {
        StaticMeshComponent* staticMesh;
        glm::mat4 transform;
        glm::mat4 invTransform;
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
            PhysicsDebugDraw* physicDrawer, VkSemaphore waitSemaphore, bool drawDebugLines = false);

    private:
        VkSemaphore ShadowPass(VkSemaphore waitSemaphore, const std::vector<DrawData>& drawData, std::thread& lightSortThread, Scene* scene, CameraComponent* camera);
        VkSemaphore MainPass(VkSemaphore waitSemaphore, const std::vector<DrawData>& drawData, Scene* scene, CameraComponent* camera, const glm::mat4& cameraProjection, const glm::mat4& cameraView, PhysicsDebugDraw* physicDrawer);
        VkSemaphore AlphaClearPass(VkSemaphore waitSemaphore);

        void SortLightsInTiles(Scene* scene, CameraComponent* camera);

        void CreateTextureRenderer();
        void DestroyTextureRenderer();

        void ResizeBuffers(glm::vec2 extent);

        void CreateRenderPass();
        void CreateDepthBuffer();
        void CreateResolveBuffer();
        void CreateFrameBufferTextures();
        void CreateOffscreenFrameBuffer();
        void CreateFrameBuffers();
        void CreateSyncObjects();

        void DestroyRenderPass();
        void DestoryDepthBuffer();
        void DestroyResolveBuffer();
        void DestoryFrameBuffers();
        void DestroySyncObjects();

        VkRenderPass m_RenderPass;

        uint32_t m_ShadowRes = 4096;
        float m_ShadowZExtentOffset = 2048.f;
        std::vector<std::unique_ptr<DepthFramebuffer>> m_ShadowDepthBuffers;
        float m_ShadowFarClip = 2048.f;

        float m_FarClip = 1000000.f;
        float m_NearClip = 1.f;

        std::vector<TextureFrameBuffer> m_TextureFrameBuffers;

        std::vector<VkSemaphore> m_ShadowPassSignalSemaphores;
        std::vector<VkSemaphore> m_MainPassSignalSemaphores;
        std::vector<VkSemaphore> m_AlphaClearPassSignalSemaphores;

        VulkanImageData m_ResolveImageData{};
        VkImageView m_ResolveImageView = VK_NULL_HANDLE;

        TextureFrameBuffer m_OffscreenFrameBuffer{};

        VkFormat m_DepthFormat{};
        VulkanImageData m_DepthBuffer;
        VkImageView m_DepthBufferImageView = VK_NULL_HANDLE;

        glm::vec2 m_Extent{ 0.f, 0.f };

        RenderPipelineKeys m_DrawMode = RenderPipelineKeys::PBR;

        SceneFrameData m_SceneFrameData;

        std::vector<VulkanUBO<SceneFrameData>> m_SceneDataUBO{};
        std::vector<VulkanSSBO<PointLightComponent::PointLight>> m_LightSSBO{};
        std::vector<VulkanSSBO<std::pair<int, int>>> m_LightCountOffsetsSSBO{};

        std::unique_ptr<Skybox> m_Skybox;

        VulkanTexture m_IrradienceMap;
        VulkanTexture m_PrefilterMap;
        VkDescriptorSet m_IrradiencePrefilterDescriptorSet;

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
        bool m_DrawDebugLines = false;
        bool m_DrawLightComplexity = false;
    };
}