#pragma once

#include "../Floof.h"
#include "VulkanRenderer.h"
#include <entt/entt.hpp>
#include "../Math.h"

namespace FLOOF {
  
    enum class SceneRendererType
    {
        Forward, Deferred, Size
    };

    static const char* SceneRendererTypeStrings[] =
    {
        "Forward", "Deferred"
    };

    class SceneRenderer {      
    public:
        SceneRenderer() = default;
        virtual ~SceneRenderer() = default;
        virtual VkDescriptorSet RenderToTexture(entt::registry& scene, glm::vec2 extent) = 0;
    };

    struct ImGui_FrameRenderBuffers {
        VkDeviceMemory      VertexBufferMemory;
        VkDeviceMemory      IndexBufferMemory;
        VkDeviceSize        VertexBufferSize;
        VkDeviceSize        IndexBufferSize;
        VkBuffer            VertexBuffer;
        VkBuffer            IndexBuffer;
    };

    // Each viewport will hold 1 ImGui_ImplVulkanH_WindowRenderBuffers
    // [Please zero-clear before use!]
    struct ImGui_WindowRenderBuffers {
        uint32_t            Index;
        uint32_t            Count;
        ImGui_FrameRenderBuffers* FrameRenderBuffers;
    };

    // Vulkan data
    struct ImGui_Data {
        ImGui_ImplVulkan_InitInfo   VulkanInitInfo;
        VkRenderPass                RenderPass;
        VkDeviceSize                BufferMemoryAlignment;
        VkPipelineCreateFlags       PipelineCreateFlags;
        VkDescriptorSetLayout       DescriptorSetLayout;
        VkPipelineLayout            PipelineLayout;
        VkPipeline                  Pipeline;
        uint32_t                    Subpass;
        VkShaderModule              ShaderModuleVert;
        VkShaderModule              ShaderModuleFrag;

        // Font data
        VkSampler                   FontSampler;
        VkDeviceMemory              FontMemory;
        VkImage                     FontImage;
        VkImageView                 FontView;
        VkDescriptorSet             FontDescriptorSet;
        VkDeviceMemory              UploadBufferMemory;
        VkBuffer                    UploadBuffer;

        // Render buffers for main window
        ImGui_WindowRenderBuffers MainWindowRenderBuffers;

        ImGui_Data() {
            memset((void*)this, 0, sizeof(*this));
            BufferMemoryAlignment = 256;
        }
    };
}