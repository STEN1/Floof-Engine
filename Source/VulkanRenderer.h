#pragma once
#define GLFW_INCLUDE_VULKAN
#define VK_ENABLE_BETA_EXTENSIONS
#include <GLFW/glfw3.h>
#include <vector>

#include "Math.h"

#define VMA_STATIC_VULKAN_FUNCTIONS 0
#define VMA_DYNAMIC_VULKAN_FUNCTIONS 1
#include "vma/vk_mem_alloc.h"
#include <string>
#include "Vertex.h"

#include <unordered_map>
#include <memory>

#include "imgui_impl_vulkan.h"

namespace FLOOF {

    struct MeshPushConstants {
        glm::mat4 MVP;
        glm::mat4 InvModelMat;
    };

    struct ColorPushConstants {
        glm::mat4 MVP;
    };

    struct VulkanBuffer {
        VkBuffer Buffer = VK_NULL_HANDLE;
        VmaAllocation Allocation = VK_NULL_HANDLE;
        VmaAllocationInfo AllocationInfo{};
    };

    struct VulkanImage {
        VkImage Image;
        VmaAllocation Allocation;
        VmaAllocationInfo AllocationInfo;
    };

    struct VulkanCombinedTextureSampler {
        VkImage Image = VK_NULL_HANDLE;
        VkImageView ImageView = VK_NULL_HANDLE;
        VkSampler Sampler = VK_NULL_HANDLE;
        VmaAllocation Allocation = VK_NULL_HANDLE;
        VmaAllocationInfo AllocationInfo{};
    };

    enum RenderPipelineFlags : uint32_t {
        None = 0,
        AlphaBlend = 1 << 0,
        DepthPass = 1 << 1,
        MSAA = 1 << 2,
    };

    enum class RenderPipelineKeys : uint32_t {
        None = 0,
        Basic,
        Line,
        LineStrip,
        Lit,
        Normal,
        Point,
        LineWithDepth,
        LineStripWithDepth,
        LitColor,
    };

    inline RenderPipelineFlags operator | (RenderPipelineFlags lhs, RenderPipelineFlags rhs) {
        return static_cast<RenderPipelineFlags>(static_cast<uint32_t>(lhs) | static_cast<uint32_t>(rhs));
    }

    inline RenderPipelineFlags operator & (RenderPipelineFlags lhs, RenderPipelineFlags rhs) {
        return static_cast<RenderPipelineFlags>(static_cast<uint32_t>(lhs) & static_cast<uint32_t>(rhs));
    }

    struct RenderPipelineParams {
        RenderPipelineFlags Flags;
        RenderPipelineKeys Key;
        std::string FragmentPath;
        std::string VertexPath;
        VkPolygonMode PolygonMode;
        VkPrimitiveTopology Topology;
        VkVertexInputBindingDescription BindingDescription;
        std::vector<VkVertexInputAttributeDescription> AttributeDescriptions;
        uint32_t PushConstantSize;
        std::vector<VkDescriptorSetLayoutBinding> DescriptorSetLayoutBindings;
    };

    struct ImguiInfo {

    };

    class VulkanRenderer {
        friend class TextureComponent;
        friend class MeshComponent;
        friend class LineMeshComponent;
        friend class PointCloudComponent;
    public:
        VulkanRenderer(GLFWwindow* window);
        ~VulkanRenderer();

        uint32_t GetNextSwapchainImage();
        VkExtent2D GetExtent() { return m_SwapChainExtent; }
        VkPipelineLayout GetPipelineLayout(RenderPipelineKeys key) { return m_PipelineLayouts[key]; }

        VkCommandBuffer StartRecording();
        void EndRecording();
        void SubmitAndPresent();

        ImGui_ImplVulkan_InitInfo GetImguiInitInfo();
        VkRenderPass GetImguiRenderPass();

        void FinishAllFrames();
        VkPipelineLayout BindGraphicsPipeline(VkCommandBuffer cmdBuffer, RenderPipelineKeys Key);

        static VulkanRenderer* Get() { return s_Singleton; }

        template<typename VertexType>
        VulkanBuffer CreateVertexBuffer(const std::vector<VertexType>& vertices) {
            std::size_t size = sizeof(VertexType) * vertices.size();
            VkBufferCreateInfo bufferInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
            bufferInfo.size = size;
            bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
            bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

            VmaAllocationCreateInfo allocInfo = {};
            allocInfo.usage = VMA_MEMORY_USAGE_AUTO;
            allocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
                VMA_ALLOCATION_CREATE_MAPPED_BIT;

            VulkanBuffer stagingBuffer{};
            vmaCreateBuffer(m_Allocator, &bufferInfo, &allocInfo,
                &stagingBuffer.Buffer, &stagingBuffer.Allocation, &stagingBuffer.AllocationInfo);

            memcpy(stagingBuffer.AllocationInfo.pMappedData, vertices.data(), size);
            // No need to free stagingVertexBuffer memory because CPU_ONLY memory is always HOST_COHERENT.
            // Gets deleted in vmaDestroyBuffer call.

            bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
            allocInfo.flags = 0;
            VulkanBuffer vertexBuffer{};
            vmaCreateBuffer(m_Allocator, &bufferInfo, &allocInfo,
                &vertexBuffer.Buffer, &vertexBuffer.Allocation, &vertexBuffer.AllocationInfo);

            CopyBuffer(stagingBuffer.Buffer, vertexBuffer.Buffer, size);

            vmaDestroyBuffer(m_Allocator, stagingBuffer.Buffer, stagingBuffer.Allocation);

            return vertexBuffer;
        }
        VulkanBuffer CreateIndexBuffer(const std::vector<uint32_t>& indices);
        void DestroyVulkanBuffer(VulkanBuffer* buffer);
    private:
        inline static VulkanRenderer* s_Singleton = nullptr;

        void CopyBuffer(VkBuffer src, VkBuffer dst, VkDeviceSize size);
        void CopyBufferToImage(VkBuffer srcBuffer, VkImage dstImage, uint32_t sizeX, uint32_t sizeY);

        GLFWwindow* m_Window;

        void InitSurface();
        void InitInstance();
        void InitDevice();

        void InitPhysicalDevice();
        void InitLogicalDevice();

        void InitVulkanAllocator();

        void InitSwapChain();
        void InitImageViews();

        void InitRenderPass();
        void InitGraphicsPipeline(const RenderPipelineParams& params);

        void InitFramebuffers();
        void InitDepthBuffer();
        void InitCommandPool();
        void InitCommandBuffer();

        void InitDescriptorPools();

        void InitSyncObjects();

        void InitGlfwCallbacks();

        void CleanupSwapChain();

    public:
        VkCommandBuffer AllocateBeginOneTimeCommandBuffer();
        void EndSubmitFreeCommandBuffer(VkCommandBuffer);
        void RecreateSwapChain();
    private:
        void WaitWhileMinimized();

        VkShaderModule MakeShaderModule(const char* path);

        VkImageViewCreateInfo MakeImageViewCreateInfo(int i);

        void ValidatePhysicalDeviceExtentions();
        void ValidatePhysicalDeviceSurfaceCapabilities();

        VkSurfaceFormatKHR GetSurfaceFormat(VkFormat format, VkColorSpaceKHR colorSpace);
        VkPresentModeKHR GetPresentMode(VkPresentModeKHR presentMode);
        VkExtent2D GetWindowExtent();

        VkFormat FindDepthFormat();
        VkFormat FindSupportedFormat(const std::vector<VkFormat>& candidates,
            VkImageTiling tiling,
            VkFormatFeatureFlags features);

        VkFormat m_DepthFormat;
        VulkanImage m_DepthBuffer;
        VkImageView m_DepthBufferImageView;

        VkSurfaceKHR m_Surface;
        VkInstance m_Instance;
        VkPhysicalDevice m_PhysicalDevice;
        VkDevice m_LogicalDevice;
        VkSwapchainKHR m_SwapChain = VK_NULL_HANDLE;
        VkPhysicalDeviceFeatures m_PhysicalDeviceFeatures;
        VmaAllocator m_Allocator;

        std::unordered_map<RenderPipelineKeys, VkDescriptorSetLayout> m_DescriptorSetLayouts;

        VkQueue m_GraphicsQueue;
        VkQueue m_PresentQueue;

        std::vector<VkImage> m_SwapChainImages;
        VkSurfaceFormatKHR m_SwapChainImageFormat;
        VkPresentModeKHR m_PresentMode;
        VkExtent2D m_SwapChainExtent;

        VkRenderPass m_RenderPass;
        std::unordered_map<RenderPipelineKeys, VkPipelineLayout> m_PipelineLayouts;
        std::unordered_map<RenderPipelineKeys, VkPipeline> m_GraphicsPipelines;
        VkCommandPool m_CommandPool;
        std::vector<VkCommandBuffer> m_CommandBuffers;

    public:
        VkDescriptorSet AllocateTextureDescriptorSet();
        void FreeTextureDescriptorSet(VkDescriptorSet desctriptorSet);
    private:
        VkDescriptorPool m_TextureDescriptorPool;

        std::vector<VkImageView> m_SwapChainImageViews;
        std::vector<VkFramebuffer> m_SwapChainFramebuffers;

        std::vector<VkSemaphore> m_ImageAvailableSemaphores;
        std::vector<VkSemaphore> m_RenderFinishedSemaphores;
        std::vector<VkFence> m_InFlightFences;

        const int MAX_FRAMES_IN_FLIGHT = 2;
        uint32_t m_CurrentFrame = 0;
        uint32_t m_CurrentImageIndex = 0;

        const std::vector<const char*> m_RequiredDeviceExtentions = {
            VK_KHR_SWAPCHAIN_EXTENSION_NAME,
            VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME,
#ifdef VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME
            VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME,
#endif
        };

        const std::vector<const char*> m_RequiredInstanceExtentions = {
            VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
            VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME,
#ifdef VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME
            VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME,
#endif
        };

        struct QueueFamilyIndices {
            int Graphics = -1;
            int Compute = -1;
            int Transfer = -1;
            int SparseBinding = -1;
            int PresentIndex = -1;
        };
        QueueFamilyIndices m_QueueFamilyIndices{};
        void PopulateQueueFamilyIndices(QueueFamilyIndices& QFI);

        struct SwapChainSupportDetails {
            VkSurfaceCapabilitiesKHR capabilities;
            std::vector<VkSurfaceFormatKHR> formats;
            std::vector<VkPresentModeKHR> presentModes;
        };
        SwapChainSupportDetails m_SwapChainSupport;

        // Validation layers for debug builds.
#ifndef NDEBUG
        const std::vector<const char*> m_ValidationLayers = {
        "VK_LAYER_KHRONOS_validation"
        };
#endif
    };
}