#include "VulkanRenderer.h"
#include "../Floof.h"
#include <cstring>
#include <limits>
#include <algorithm>
#include <fstream>

namespace FLOOF {
#ifndef NDEBUG
    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT messageType,
        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
        void* pUserData) {

        // Message is important enough to show
        if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT) {
            std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;
            int breakMeDaddy = 80085;
        }

        return VK_FALSE;
    }

    static VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger) {
        auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
        if (func != nullptr) {
            return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
        } else {
            return VK_ERROR_EXTENSION_NOT_PRESENT;
        }
    }

    static void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator) {
        auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
        if (func != nullptr) {
            func(instance, debugMessenger, pAllocator);
        }
    }
#endif

    VulkanRenderer::VulkanRenderer(GLFWwindow* window)
        : m_Window{ window } {
        s_Singleton = this;
        CreateInstance();
#ifndef NDEBUG
        CreateDebugUtilsMessenger();
#endif
        CreateSurface();
        CreateDevice();

        CreateVulkanAllocator();

        CreateWindow(m_VulkanWindow);
        
        
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
            CreateGraphicsPipeline(params);
        }
        {	// Lit color shader for terrain.
            RenderPipelineParams params;
            params.Flags = RenderPipelineFlags::AlphaBlend | RenderPipelineFlags::DepthPass;
            params.FragmentPath = "Shaders/LitColor.frag.spv";
            params.VertexPath = "Shaders/LitColor.vert.spv";
            params.Key = RenderPipelineKeys::LitColor;
            params.PolygonMode = VK_POLYGON_MODE_FILL;
            params.Topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
            params.BindingDescription = ColorNormalVertex::GetBindingDescription();
            params.AttributeDescriptions = ColorNormalVertex::GetAttributeDescriptions();
            params.PushConstantSize = sizeof(MeshPushConstants);
            CreateGraphicsPipeline(params);
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
            CreateGraphicsPipeline(params);
        }
        {	// Line drawing shader with depth
            RenderPipelineParams params;
            params.Flags = RenderPipelineFlags::AlphaBlend | RenderPipelineFlags::DepthPass;
            params.FragmentPath = "Shaders/Color.frag.spv";
            params.VertexPath = "Shaders/Color.vert.spv";
            params.Key = RenderPipelineKeys::LineWithDepth;
            params.PolygonMode = VK_POLYGON_MODE_LINE;
            params.Topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
            params.BindingDescription = ColorVertex::GetBindingDescription();
            params.AttributeDescriptions = ColorVertex::GetAttributeDescriptions();
            params.PushConstantSize = sizeof(ColorPushConstants);
            CreateGraphicsPipeline(params);
        }
        {	// Line strip drawing shader with depth
            RenderPipelineParams params;
            params.Flags = RenderPipelineFlags::AlphaBlend | RenderPipelineFlags::DepthPass;
            params.FragmentPath = "Shaders/Color.frag.spv";
            params.VertexPath = "Shaders/Color.vert.spv";
            params.Key = RenderPipelineKeys::LineStripWithDepth;
            params.PolygonMode = VK_POLYGON_MODE_LINE;
            params.Topology = VK_PRIMITIVE_TOPOLOGY_LINE_STRIP;
            params.BindingDescription = ColorVertex::GetBindingDescription();
            params.AttributeDescriptions = ColorVertex::GetAttributeDescriptions();
            params.PushConstantSize = sizeof(ColorPushConstants);
            CreateGraphicsPipeline(params);
        }
        {	// LineStrip drawing shader
            RenderPipelineParams params;
            params.Flags = RenderPipelineFlags::AlphaBlend;
            params.FragmentPath = "Shaders/Color.frag.spv";
            params.VertexPath = "Shaders/Color.vert.spv";
            params.Key = RenderPipelineKeys::LineStrip;
            params.PolygonMode = VK_POLYGON_MODE_LINE;
            params.Topology = VK_PRIMITIVE_TOPOLOGY_LINE_STRIP;
            params.BindingDescription = ColorVertex::GetBindingDescription();
            params.AttributeDescriptions = ColorVertex::GetAttributeDescriptions();
            params.PushConstantSize = sizeof(ColorPushConstants);
            CreateGraphicsPipeline(params);
        }
        {	// Point drawing shader
            RenderPipelineParams params;
            params.Flags = RenderPipelineFlags::AlphaBlend | RenderPipelineFlags::DepthPass;
            params.FragmentPath = "Shaders/Color.frag.spv";
            params.VertexPath = "Shaders/Color.vert.spv";
            params.Key = RenderPipelineKeys::Point;
            params.PolygonMode = VK_POLYGON_MODE_POINT;
            params.Topology = VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
            params.BindingDescription = ColorVertex::GetBindingDescription();
            params.AttributeDescriptions = ColorVertex::GetAttributeDescriptions();
            params.PushConstantSize = sizeof(ColorPushConstants);
            CreateGraphicsPipeline(params);
        }
        CreateDescriptorPools();
        CreateCommandPool();
        AllocateCommandBuffers(m_VulkanWindow);
        InitGlfwCallbacks();
    }

    VulkanRenderer::~VulkanRenderer() {
        vkDestroyImageView(m_LogicalDevice, m_DepthBufferImageView, nullptr);
        vmaDestroyImage(m_Allocator, m_DepthBuffer.Image, m_DepthBuffer.Allocation);
        vmaDestroyAllocator(m_Allocator);

        DestroyWindow(m_VulkanWindow);


        vkDestroyDescriptorPool(m_LogicalDevice, m_TextureDescriptorPool, nullptr);
        vkDestroyCommandPool(m_LogicalDevice, m_CommandPool, nullptr);
        for (auto& [key, val] : m_DescriptorSetLayouts) {
            vkDestroyDescriptorSetLayout(m_LogicalDevice, val, nullptr);
        }
        for (auto& [key, val] : m_GraphicsPipelines) {
            vkDestroyPipeline(m_LogicalDevice, val, nullptr);
        }
        for (auto& [key, val] : m_PipelineLayouts) {
            vkDestroyPipelineLayout(m_LogicalDevice, val, nullptr);
        }
        vkDestroyRenderPass(m_LogicalDevice, m_RenderPass, nullptr);


        vkDestroySurfaceKHR(m_Instance, m_Surface, nullptr);
        vkDestroyDevice(m_LogicalDevice, nullptr);

#ifndef NDEBUG
        DestroyDebugUtilsMessengerEXT(m_Instance, m_DebugMessenger, nullptr);
#endif
        vkDestroyInstance(m_Instance, nullptr);
        s_Singleton = nullptr;
    }

    uint32_t VulkanRenderer::GetNextSwapchainImage(VulkanWindow& window) {
        vkWaitForFences(m_LogicalDevice, 1, &window.Frames[window.FrameIndex].Fence, VK_TRUE, UINT64_MAX);

        uint32_t imageIndex;
        VkResult result = vkAcquireNextImageKHR(m_LogicalDevice, window.Swapchain, UINT64_MAX, window.Frames[window.FrameIndex].ImageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);
        if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
            RecreateSwapChain(window);
            return GetNextSwapchainImage(window);
        }
        ASSERT(result == VK_SUCCESS || result == VK_SUBOPTIMAL_KHR);

        vkResetFences(m_LogicalDevice, 1, &window.Frames[window.FrameIndex].Fence);

        return imageIndex;
    }

    void VulkanRenderer::NewFrame(VulkanWindow& window) {
        window.ImageIndex = GetNextSwapchainImage(window);
        window.SubmitInfos.clear();
    }

    void VulkanRenderer::StartRenderPass(VkCommandBuffer commandBuffer, VkRenderPass renderPass, VkFramebuffer frameBuffer, VkExtent2D extent) {
        vkResetCommandBuffer(commandBuffer, 0);

        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = 0; // Optional
        beginInfo.pInheritanceInfo = nullptr; // Optional

        VkResult beginResult = vkBeginCommandBuffer(commandBuffer, &beginInfo);
        ASSERT(beginResult == VK_SUCCESS);

        VkRenderPassBeginInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = renderPass;
        renderPassInfo.framebuffer = frameBuffer;
        renderPassInfo.renderArea.offset = { 0, 0 };
        renderPassInfo.renderArea.extent = extent;
        VkClearValue clearColor[2]{};
        clearColor[0].color = { {0.0f, 0.0f, 0.0f, 1.0f} };
        clearColor[1].depthStencil = { 1.0f, 0 };
        renderPassInfo.clearValueCount = 2;
        if (renderPass == m_ImGuiRenderPass)
            renderPassInfo.clearValueCount = 0;
        renderPassInfo.pClearValues = clearColor;

        vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

        VkViewport viewport{};
        viewport.x = 0.f;
        viewport.y = 0.f;
        viewport.width = static_cast<float>(extent.width);
        viewport.height = static_cast<float>(extent.height);
        viewport.minDepth = 0.f;
        viewport.maxDepth = 1.f;

        vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

        VkRect2D scissor{};
        scissor.offset = { 0, 0 };
        scissor.extent = extent;
        vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
    }

    void VulkanRenderer::EndRenderPass(VkCommandBuffer* commandBuffer, VkSemaphore* waitSemaphore, VkSemaphore* signalSemaphore, VkPipelineStageFlags* waitStages) {
        vkCmdEndRenderPass(*commandBuffer);
        VkResult endResult = vkEndCommandBuffer(*commandBuffer);
        ASSERT(endResult == VK_SUCCESS);

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = waitSemaphore;
        submitInfo.pWaitDstStageMask = waitStages;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = commandBuffer;
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = signalSemaphore;

        m_VulkanWindow.SubmitInfos.push_back(submitInfo);
    }

    void VulkanRenderer::Present(VulkanWindow& window) {
        VkResult result = vkQueueSubmit(m_GraphicsQueue, window.SubmitInfos.size(), window.SubmitInfos.data(),
            window.Frames[window.FrameIndex].Fence);
        ASSERT(result == VK_SUCCESS);

        VkPresentInfoKHR presentInfo{};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = &window.Frames[window.FrameIndex].RenderFinishedSemaphore;
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = &window.Swapchain;
        presentInfo.pImageIndices = &window.ImageIndex;
        presentInfo.pResults = nullptr; // Optional

        result = vkQueuePresentKHR(m_PresentQueue, &presentInfo);
        if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
            RecreateSwapChain(window);
        }
        ASSERT(result == VK_SUCCESS ||
            result == VK_ERROR_OUT_OF_DATE_KHR ||
            result == VK_SUBOPTIMAL_KHR);

        window.FrameIndex = (window.FrameIndex + 1) % VulkanGlobals::MAX_FRAMES_IN_FLIGHT;
    }

    ImGui_ImplVulkan_InitInfo VulkanRenderer::GetImguiInitInfo() {
        ImGui_ImplVulkan_InitInfo initInfo{};

        initInfo.Instance = m_Instance;
        initInfo.PhysicalDevice = m_PhysicalDevice;
        initInfo.Device = m_LogicalDevice;
        initInfo.QueueFamily = m_QueueFamilyIndices.Graphics;
        initInfo.Queue = m_GraphicsQueue;
        initInfo.DescriptorPool = m_TextureDescriptorPool;
        initInfo.Subpass = 0;
        initInfo.MinImageCount = m_VulkanWindow.ImageCount;
        initInfo.ImageCount = m_VulkanWindow.ImageCount;
        initInfo.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
        initInfo.Allocator = nullptr;
        initInfo.CheckVkResultFn = nullptr;

        return initInfo;
    }

    VkRenderPass VulkanRenderer::GetImguiRenderPass() {
        return m_ImGuiRenderPass;
    }

    VkRenderPass VulkanRenderer::GetMainRenderPass() {
        return m_RenderPass;
    }

    void VulkanRenderer::FinishAllFrames() {
        vkDeviceWaitIdle(m_LogicalDevice);
    }

    VkPipelineLayout VulkanRenderer::BindGraphicsPipeline(VkCommandBuffer cmdBuffer, RenderPipelineKeys Key) {
        vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_GraphicsPipelines[Key]);
        return GetPipelineLayout(Key);
    }

    VulkanBuffer VulkanRenderer::CreateIndexBuffer(const std::vector<uint32_t>& indices) {
        std::size_t size = sizeof(uint32_t) * indices.size();
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

        memcpy(stagingBuffer.AllocationInfo.pMappedData, indices.data(), size);
        // No need to free stagingVertexBuffer memory because CPU_ONLY memory is always HOST_COHERENT.
        // Gets deleted in vmaDestroyBuffer call.

        bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
        allocInfo.flags = 0;
        VulkanBuffer indexBuffer{};
        vmaCreateBuffer(m_Allocator, &bufferInfo, &allocInfo,
            &indexBuffer.Buffer, &indexBuffer.Allocation, &indexBuffer.AllocationInfo);

        CopyBuffer(stagingBuffer.Buffer, indexBuffer.Buffer, size);

        vmaDestroyBuffer(m_Allocator, stagingBuffer.Buffer, stagingBuffer.Allocation);

        return indexBuffer;
    }

    void VulkanRenderer::CopyBuffer(VkBuffer src, VkBuffer dst, VkDeviceSize size) {
        VkCommandBuffer commandBuffer = AllocateBeginOneTimeCommandBuffer();

        VkBufferCopy copyRegion{};
        copyRegion.srcOffset = 0; // Optional
        copyRegion.dstOffset = 0; // Optional
        copyRegion.size = size;
        vkCmdCopyBuffer(commandBuffer, src, dst, 1, &copyRegion);

        EndSubmitFreeCommandBuffer(commandBuffer);
    }

    void VulkanRenderer::CopyBufferToImage(VkBuffer srcBuffer, VkImage dstImage, uint32_t sizeX, uint32_t sizeY) {
        VkCommandBufferAllocateInfo commandAllocInfo{};
        commandAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        commandAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        commandAllocInfo.commandPool = m_CommandPool;
        commandAllocInfo.commandBufferCount = 1;
        VkCommandBuffer commandBuffer;
        vkAllocateCommandBuffers(m_LogicalDevice, &commandAllocInfo, &commandBuffer);
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        vkBeginCommandBuffer(commandBuffer, &beginInfo);

        VkImageMemoryBarrier imgMemBarrier = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
        imgMemBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        imgMemBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        imgMemBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        imgMemBarrier.subresourceRange.baseMipLevel = 0;
        imgMemBarrier.subresourceRange.levelCount = 1;
        imgMemBarrier.subresourceRange.baseArrayLayer = 0;
        imgMemBarrier.subresourceRange.layerCount = 1;
        imgMemBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imgMemBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        imgMemBarrier.image = dstImage;
        imgMemBarrier.srcAccessMask = 0;
        imgMemBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        vkCmdPipelineBarrier(
            commandBuffer,
            VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            0,
            0, nullptr,
            0, nullptr,
            1, &imgMemBarrier);

        VkBufferImageCopy region = {};
        region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.layerCount = 1;
        region.imageExtent.width = sizeX;
        region.imageExtent.height = sizeY;
        region.imageExtent.depth = 1;

        vkCmdCopyBufferToImage(commandBuffer, srcBuffer, dstImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

        imgMemBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        imgMemBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imgMemBarrier.image = dstImage;
        imgMemBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        imgMemBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        vkCmdPipelineBarrier(
            commandBuffer,
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
            0,
            0, nullptr,
            0, nullptr,
            1, &imgMemBarrier);


        vkEndCommandBuffer(commandBuffer);

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffer;

        vkQueueSubmit(m_GraphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
        vkQueueWaitIdle(m_GraphicsQueue);
        vkFreeCommandBuffers(m_LogicalDevice, m_CommandPool, 1, &commandBuffer);
    }

    void VulkanRenderer::DestroyVulkanBuffer(VulkanBuffer* buffer) {
        vmaDestroyBuffer(m_Allocator, buffer->Buffer, buffer->Allocation);
    }

    void VulkanRenderer::CreateSurface() {
        VkResult result = glfwCreateWindowSurface(m_Instance, m_Window, nullptr, &m_Surface);

        ASSERT(result == VK_SUCCESS);
        LOG("Vulkan surface created.\n");
    }

    void VulkanRenderer::CreateInstance() {
        VkApplicationInfo appInfo{};
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName = "VulkanApp";
        appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.pEngineName = "Floof";
        appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.apiVersion = VK_API_VERSION_1_1;

        uint32_t glfwExtensionCount = 0;
        const char** glfwExtensions;
        glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

        // Assumes glfw knows what its doing. Might be footgun :)
        std::vector<const char*> extensions(glfwExtensionCount);
        for (int i = 0; i < glfwExtensionCount; i++) {
            extensions[i] = glfwExtensions[i];
        }

        uint32_t extensionCount{};
        vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
        std::vector<VkExtensionProperties> enumeratedExtensions(extensionCount);
        vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, enumeratedExtensions.data());
        LOG("Available extensions:\n");
        for (auto& extension : enumeratedExtensions) {
            LOG("\t");
            LOG(extension.extensionName);
            LOG("\n");
        }

        for (auto& reqExt : m_RequiredInstanceExtentions) {
            bool foundReqExt = false;
            for (auto& availableExt : enumeratedExtensions) {
                if (strcmp(reqExt, availableExt.extensionName) == 0) {
                    extensions.push_back(reqExt);
                    foundReqExt = true;
                }
            }
            if (foundReqExt == false) {
                std::cout << "Could not find instance ext: " << reqExt << std::endl;
            }
        }

        std::cout << "Enabled instance extentions:\n";
        for (auto& ext : extensions) {
            std::cout << "\t" << ext << "\n";
        }

        VkInstanceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
#ifdef VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME
        createInfo.flags = VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
#endif
        createInfo.pApplicationInfo = &appInfo;
        createInfo.enabledExtensionCount = extensions.size();
        createInfo.ppEnabledExtensionNames = extensions.data();

        // Enable validation layers in debug builds.
        // TODO: Make custom callback function for validation layer logging.
#ifdef NDEBUG
        createInfo.enabledLayerCount = 0;
#else
        createInfo.enabledLayerCount = (uint32_t)m_ValidationLayers.size();
        createInfo.ppEnabledLayerNames = m_ValidationLayers.data();
#endif

        VkResult createInstanceResult = vkCreateInstance(&createInfo, nullptr, &m_Instance);
        ASSERT(createInstanceResult == VK_SUCCESS);


        LOG("Vulkan instance created.\n");
    }

#ifndef NDEBUG
    void VulkanRenderer::CreateDebugUtilsMessenger() {
        VkDebugUtilsMessengerCreateInfoEXT createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        createInfo.pfnUserCallback = debugCallback;
        createInfo.pUserData = nullptr; // Optional

        auto result = CreateDebugUtilsMessengerEXT(m_Instance, &createInfo, nullptr, &m_DebugMessenger);
        ASSERT(result == VK_SUCCESS);
    }
#endif

    void VulkanRenderer::CreateDevice() {
        CreatePhysicalDevice();
        CreateLogicalDevice();
        LOG("Physical and logical device created.\n");
    }

    void VulkanRenderer::CreatePhysicalDevice() {
        uint32_t deviceCount{};
        vkEnumeratePhysicalDevices(m_Instance, &deviceCount, nullptr);
        std::vector<VkPhysicalDevice> devices(deviceCount);
        vkEnumeratePhysicalDevices(m_Instance, &deviceCount, devices.data());
        LOG("Available devices:\n");
        int deviceIndex = 0;
        for (int i = 0; i < devices.size(); i++) {
            VkPhysicalDeviceProperties deviceProperties;
            vkGetPhysicalDeviceProperties(devices[i], &deviceProperties);
            if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
                deviceIndex = i;
            LOG("\t");
            LOG(deviceProperties.deviceName);
            LOG("\n");
        }
        ASSERT(deviceCount > 0);
        m_PhysicalDevice = devices[deviceIndex];

        PopulateQueueFamilyIndices(m_QueueFamilyIndices);
        //ValidatePhysicalDeviceExtentions(); doing this in instance creation.
        ValidatePhysicalDeviceSurfaceCapabilities();
    }

    void VulkanRenderer::CreateLogicalDevice() {
        vkGetPhysicalDeviceFeatures(m_PhysicalDevice, &m_PhysicalDeviceFeatures);
        VkDeviceQueueCreateInfo dqCreateInfo{};
        dqCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        dqCreateInfo.queueFamilyIndex = m_QueueFamilyIndices.Graphics;
        dqCreateInfo.queueCount = 1;
        float queuePrio = 1.f;
        dqCreateInfo.pQueuePriorities = &queuePrio;

        std::vector<VkDeviceQueueCreateInfo> dqcreateInfos = { dqCreateInfo };

        if (m_QueueFamilyIndices.Graphics != m_QueueFamilyIndices.PresentIndex) {
            std::cout << "Graphics and present index are not the same. Creating from different queue families.\n";
            std::cout << "Graphics queue index: " << m_QueueFamilyIndices.Graphics
                << " Present queue index: " << m_QueueFamilyIndices.PresentIndex << std::endl;

            VkDeviceQueueCreateInfo createInfoPresentQ{};
            createInfoPresentQ.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            createInfoPresentQ.queueFamilyIndex = m_QueueFamilyIndices.PresentIndex;
            createInfoPresentQ.queueCount = 1;
            float queuePrio = 1.f;
            createInfoPresentQ.pQueuePriorities = &queuePrio;

            dqcreateInfos.push_back(createInfoPresentQ);
        }

        uint32_t extCount{};
        vkEnumerateDeviceExtensionProperties(m_PhysicalDevice, nullptr, &extCount, nullptr);
        std::vector<VkExtensionProperties> enumeratedExt(extCount);
        vkEnumerateDeviceExtensionProperties(m_PhysicalDevice, nullptr, &extCount, enumeratedExt.data());

        std::vector<const char*> foundExtentions;
        for (auto& reqExt : m_RequiredDeviceExtentions) {
            bool foundExt = false;
            for (auto& ext : enumeratedExt) {
                if (strcmp(reqExt, ext.extensionName) == 0) {
                    foundExtentions.push_back(reqExt);
                    foundExt = true;
                }
            }
            if (foundExt == false) {
                std::cout << "Could not find device ext: " << reqExt << std::endl;
            }
        }

        VkDeviceCreateInfo dCreateInfo{};
        dCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        dCreateInfo.pQueueCreateInfos = dqcreateInfos.data();
        dCreateInfo.queueCreateInfoCount = dqcreateInfos.size();
        dCreateInfo.pEnabledFeatures = &m_PhysicalDeviceFeatures;
        dCreateInfo.enabledExtensionCount = foundExtentions.size();
        dCreateInfo.ppEnabledExtensionNames = foundExtentions.data();
        dCreateInfo.enabledLayerCount = 0;

        VkResult deviceCreationResult = vkCreateDevice(m_PhysicalDevice, &dCreateInfo, nullptr, &m_LogicalDevice);
        ASSERT(deviceCreationResult == VK_SUCCESS);
        ASSERT(m_QueueFamilyIndices.Graphics != -1);
        vkGetDeviceQueue(m_LogicalDevice, m_QueueFamilyIndices.Graphics, 0, &m_GraphicsQueue);
        ASSERT(m_QueueFamilyIndices.PresentIndex != -1);
        vkGetDeviceQueue(m_LogicalDevice, m_QueueFamilyIndices.PresentIndex, 0, &m_PresentQueue);
    }

    void VulkanRenderer::CreateVulkanAllocator() {
        VmaVulkanFunctions vulkanFunctions = {};
        vulkanFunctions.vkGetInstanceProcAddr = &vkGetInstanceProcAddr;
        vulkanFunctions.vkGetDeviceProcAddr = &vkGetDeviceProcAddr;

        VmaAllocatorCreateInfo allocatorCreateInfo = {};
        allocatorCreateInfo.vulkanApiVersion = VK_API_VERSION_1_1;
        allocatorCreateInfo.physicalDevice = m_PhysicalDevice;
        allocatorCreateInfo.device = m_LogicalDevice;
        allocatorCreateInfo.instance = m_Instance;
        allocatorCreateInfo.pVulkanFunctions = &vulkanFunctions;

        VkResult result = vmaCreateAllocator(&allocatorCreateInfo, &m_Allocator);
        ASSERT(result == VK_SUCCESS);
    }

    void VulkanRenderer::CreateWindow(VulkanWindow& window) {
        CreateSwapChain(window);
        CreateDepthBuffer(window.Extent);
        CreateRenderPass(window);
        CreateImGuiRenderPass(window);
        CreateFramebuffers(window);
        CreateSyncObjects(window);
    }

    void VulkanRenderer::DestroyWindow(VulkanWindow& window) {
        CleanupSwapChain(window);
        for (int i = 0; i < window.Frames.size(); i++) {
            vkDestroySemaphore(m_LogicalDevice, window.Frames[i].ImageAvailableSemaphore, nullptr);
            vkDestroySemaphore(m_LogicalDevice, window.Frames[i].RenderFinishedSemaphore, nullptr);
            vkDestroyFence(m_LogicalDevice, window.Frames[i].Fence, nullptr);
        }

    }

    void VulkanRenderer::CreateSwapChain(VulkanWindow& window) {
        window.SurfaceFormat = GetSurfaceFormat(VK_FORMAT_B8G8R8A8_UNORM, VK_COLORSPACE_SRGB_NONLINEAR_KHR);
        window.PresentMode = GetPresentMode(VK_PRESENT_MODE_MAILBOX_KHR);
        window.Extent = GetWindowExtent();
        std::cout << "m_SwapChainExtent: x = " << window.Extent.width << " y = " << window.Extent.height << std::endl;

        window.ImageCount = m_SwapChainSupport.capabilities.minImageCount + 1;
        if (m_SwapChainSupport.capabilities.maxImageCount > 0 && window.ImageCount > m_SwapChainSupport.capabilities.maxImageCount) {
            window.ImageCount = m_SwapChainSupport.capabilities.maxImageCount;
        }

        VkSwapchainCreateInfoKHR createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        createInfo.surface = m_Surface;
        createInfo.minImageCount = window.ImageCount;
        createInfo.imageFormat = window.SurfaceFormat.format;
        createInfo.imageColorSpace = window.SurfaceFormat.colorSpace;
        createInfo.imageExtent = window.Extent;
        createInfo.imageArrayLayers = 1;
        createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;


        std::vector<uint32_t> queueFamilyIndices = { (uint32_t)m_QueueFamilyIndices.Graphics, (uint32_t)m_QueueFamilyIndices.PresentIndex };
        if (m_QueueFamilyIndices.Graphics != m_QueueFamilyIndices.PresentIndex) {
            createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
            createInfo.queueFamilyIndexCount = queueFamilyIndices.size();
            createInfo.pQueueFamilyIndices = queueFamilyIndices.data();
        } else {
            createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
            createInfo.queueFamilyIndexCount = 0; // Optional
            createInfo.pQueueFamilyIndices = nullptr; // Optional
        }

        createInfo.preTransform = m_SwapChainSupport.capabilities.currentTransform;
        createInfo.presentMode = window.PresentMode;
        // TODO: Change this?
        createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR;
        createInfo.clipped = VK_TRUE;
        createInfo.oldSwapchain = VK_NULL_HANDLE;

        VkResult result = vkCreateSwapchainKHR(m_LogicalDevice, &createInfo, nullptr, &window.Swapchain);
        ASSERT(result == VK_SUCCESS);

        vkGetSwapchainImagesKHR(m_LogicalDevice, window.Swapchain, &window.ImageCount, nullptr);
        window.SwapChainImages.resize(window.ImageCount);
        vkGetSwapchainImagesKHR(m_LogicalDevice, window.Swapchain, &window.ImageCount, window.SwapChainImages.data());
        LOG("Swapchain created.\n");
        window.SwapChainImageViews.resize(window.ImageCount);
        for (size_t i = 0; i < window.ImageCount; i++) {
            VkImageViewCreateInfo createInfo{};
            createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            createInfo.image = window.SwapChainImages[i];
            createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            createInfo.format = window.SurfaceFormat.format;
            createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            createInfo.subresourceRange.baseMipLevel = 0;
            createInfo.subresourceRange.levelCount = 1;
            createInfo.subresourceRange.baseArrayLayer = 0;
            createInfo.subresourceRange.layerCount = 1;

            VkResult result = vkCreateImageView(m_LogicalDevice, &createInfo, nullptr, &window.SwapChainImageViews[i]);
            ASSERT(result == VK_SUCCESS);
        }
        LOG("Image views created.\n");
    }

    void VulkanRenderer::CreateRenderPass(VulkanWindow& window) {
        VkAttachmentDescription colorAttachments[2]{};
        colorAttachments[0].format = window.SurfaceFormat.format;
        colorAttachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
        colorAttachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAttachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        colorAttachments[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        colorAttachments[0].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        //colorAttachments[0].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

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

        VkResult result = vkCreateRenderPass(m_LogicalDevice, &renderPassInfo, nullptr, &m_RenderPass);
        ASSERT(result == VK_SUCCESS);
        LOG("Render pass created.\n");
    }

    void VulkanRenderer::CreateImGuiRenderPass(VulkanWindow& window) {

        VkAttachmentDescription colorAttachments[2]{};
        colorAttachments[0].format = window.SurfaceFormat.format;
        colorAttachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
        colorAttachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
        colorAttachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        colorAttachments[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        colorAttachments[0].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        colorAttachments[1].format = m_DepthFormat;
        colorAttachments[1].samples = VK_SAMPLE_COUNT_1_BIT;
        colorAttachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
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

        VkResult result = vkCreateRenderPass(m_LogicalDevice, &renderPassInfo, nullptr, &m_ImGuiRenderPass);
        ASSERT(result == VK_SUCCESS);
        LOG("Render pass created.\n");
    }

    void VulkanRenderer::CreateGraphicsPipeline(const RenderPipelineParams& params) {
        auto vertShader = MakeShaderModule(params.VertexPath.c_str());
        auto fragShader = MakeShaderModule(params.FragmentPath.c_str());

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
            descriptorSetLayoutCreateInfo.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT;
            descriptorSetLayoutCreateInfo.bindingCount = params.DescriptorSetLayoutBindings.size();
            descriptorSetLayoutCreateInfo.pBindings = params.DescriptorSetLayoutBindings.data();

            vkCreateDescriptorSetLayout(m_LogicalDevice, &descriptorSetLayoutCreateInfo, nullptr, &m_DescriptorSetLayouts[params.Key]);

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

        VkResult plResult = vkCreatePipelineLayout(m_LogicalDevice, &pipelineLayoutInfo, nullptr, &m_PipelineLayouts[params.Key]);
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

        VkResult gplResult = vkCreateGraphicsPipelines(m_LogicalDevice, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_GraphicsPipelines[params.Key]);
        ASSERT(gplResult == VK_SUCCESS);

        vkDestroyShaderModule(m_LogicalDevice, vertShader, nullptr);
        vkDestroyShaderModule(m_LogicalDevice, fragShader, nullptr);
        LOG("Render pipeline created.\n");
    }

    void VulkanRenderer::CreateFramebuffers(VulkanWindow& window) {
        window.FrameBuffers.resize(window.ImageCount);
        for (size_t i = 0; i < window.FrameBuffers.size(); i++) {
            VkImageView attachments[] = {
                window.SwapChainImageViews[i],
                m_DepthBufferImageView
            };

            VkFramebufferCreateInfo framebufferInfo{};
            framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            framebufferInfo.renderPass = m_RenderPass;
            framebufferInfo.attachmentCount = 2;
            framebufferInfo.pAttachments = attachments;
            framebufferInfo.width = window.Extent.width;
            framebufferInfo.height = window.Extent.height;
            framebufferInfo.layers = 1;

            VkResult result = vkCreateFramebuffer(m_LogicalDevice, &framebufferInfo, nullptr, &window.FrameBuffers[i]);
            ASSERT(result == VK_SUCCESS);
        }
        LOG("Framebuffers created.\n");
    }

    void VulkanRenderer::CreateDepthBuffer(VkExtent2D extent) {
        m_DepthFormat = FindDepthFormat();
        ASSERT(m_DepthFormat != VK_FORMAT_UNDEFINED);

        VkImageCreateInfo depthImageInfo = { VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
        depthImageInfo.imageType = VK_IMAGE_TYPE_2D;
        depthImageInfo.extent.width = extent.width;
        depthImageInfo.extent.height = extent.height;
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

        vmaCreateImage(m_Allocator, &depthImageInfo, &depthImageAllocCreateInfo,
            &m_DepthBuffer.Image, &m_DepthBuffer.Allocation, &m_DepthBuffer.AllocationInfo);

        VkImageViewCreateInfo depthImageViewInfo = { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
        depthImageViewInfo.image = m_DepthBuffer.Image;
        depthImageViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        depthImageViewInfo.format = m_DepthFormat;
        depthImageViewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        depthImageViewInfo.subresourceRange.baseMipLevel = 0;
        depthImageViewInfo.subresourceRange.levelCount = 1;
        depthImageViewInfo.subresourceRange.baseArrayLayer = 0;
        depthImageViewInfo.subresourceRange.layerCount = 1;

        auto result = vkCreateImageView(m_LogicalDevice, &depthImageViewInfo, nullptr, &m_DepthBufferImageView);
        ASSERT(result == VK_SUCCESS);
    }

    void VulkanRenderer::CreateCommandPool() {
        VkCommandPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        poolInfo.queueFamilyIndex = m_QueueFamilyIndices.Graphics;

        VkResult result = vkCreateCommandPool(m_LogicalDevice, &poolInfo, nullptr, &m_CommandPool);
        ASSERT(result == VK_SUCCESS);
        LOG("Command pool created.\n");
    }

    void VulkanRenderer::AllocateCommandBuffers(VulkanWindow& window) {
        for (auto& frame : window.Frames) {
            VkCommandBufferAllocateInfo allocInfo{};
            allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
            allocInfo.commandPool = m_CommandPool;
            allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
            allocInfo.commandBufferCount = 1;

            VkResult result = vkAllocateCommandBuffers(m_LogicalDevice, &allocInfo, &frame.MainCommandBuffer);
            ASSERT(result == VK_SUCCESS);
            result = vkAllocateCommandBuffers(m_LogicalDevice, &allocInfo, &frame.ImGuiCommandBuffer);
            ASSERT(result == VK_SUCCESS);
        }
        LOG("Command buffers created.\n");
    }

    void VulkanRenderer::CreateDescriptorPools() {
        VkDescriptorPoolSize poolSize{};
        poolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        poolSize.descriptorCount = 256;

        // Create texture descriptor pool.
        VkDescriptorPoolCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        createInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT |
            VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT;
        createInfo.maxSets = 256;
        createInfo.pPoolSizes = &poolSize;
        createInfo.poolSizeCount = 1;
        VkResult result = vkCreateDescriptorPool(m_LogicalDevice, &createInfo, nullptr, &m_TextureDescriptorPool);
        ASSERT(result == VK_SUCCESS);

    }

    void VulkanRenderer::CreateSyncObjects(VulkanWindow& window) {
        VkSemaphoreCreateInfo semaphoreInfo{};
        semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        VkFenceCreateInfo fenceInfo{};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        for (int i = 0; i < window.Frames.size(); i++) {
            VkResult result = vkCreateSemaphore(m_LogicalDevice, &semaphoreInfo, nullptr, &window.Frames[i].ImageAvailableSemaphore);
            ASSERT(result == VK_SUCCESS);
            result = vkCreateSemaphore(m_LogicalDevice, &semaphoreInfo, nullptr, &window.Frames[i].RenderFinishedSemaphore);
            ASSERT(result == VK_SUCCESS);
            result = vkCreateSemaphore(m_LogicalDevice, &semaphoreInfo, nullptr, &window.Frames[i].MainPassEndSemaphore);
            ASSERT(result == VK_SUCCESS);
            result = vkCreateFence(m_LogicalDevice, &fenceInfo, nullptr, &window.Frames[i].Fence);
            ASSERT(result == VK_SUCCESS);
        }

        LOG("Sync objects created.\n");
    }

    void VulkanRenderer::InitGlfwCallbacks() {
        glfwSetWindowUserPointer(m_Window, (void*)this);
        glfwSetWindowSizeCallback(m_Window, [](GLFWwindow* window, int width, int height) {
            VulkanRenderer* renderer = (VulkanRenderer*)glfwGetWindowUserPointer(window);
            renderer->RecreateSwapChain(*renderer->GetVulkanWindow());
            LOG("Resize callback\n");
            });
    }

    void VulkanRenderer::CleanupSwapChain(VulkanWindow& window) {
        for (size_t i = 0; i < window.ImageCount; i++) {
            vkDestroyFramebuffer(m_LogicalDevice, window.FrameBuffers[i], nullptr);
        }

        for (size_t i = 0; i < window.Frames.size(); i++) {
            vkDestroyImageView(m_LogicalDevice, window.SwapChainImageViews[i], nullptr);
        }

        vkDestroySwapchainKHR(m_LogicalDevice, window.Swapchain, nullptr);
    }

    VkCommandBuffer VulkanRenderer::AllocateBeginOneTimeCommandBuffer() {
        VkCommandBufferAllocateInfo commandAllocInfo{};
        commandAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        commandAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        commandAllocInfo.commandPool = m_CommandPool;
        commandAllocInfo.commandBufferCount = 1;
        VkCommandBuffer commandBuffer;
        vkAllocateCommandBuffers(m_LogicalDevice, &commandAllocInfo, &commandBuffer);
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        vkBeginCommandBuffer(commandBuffer, &beginInfo);

        return commandBuffer;
    }

    void VulkanRenderer::EndSubmitFreeCommandBuffer(VkCommandBuffer commandBuffer) {
        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffer;

        vkEndCommandBuffer(commandBuffer);
        vkQueueSubmit(m_GraphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
        vkQueueWaitIdle(m_GraphicsQueue);
        vkFreeCommandBuffers(m_LogicalDevice, m_CommandPool, 1, &commandBuffer);
    }

    void VulkanRenderer::RecreateSwapChain(VulkanWindow& window) {
        WaitWhileMinimized();

        vkDeviceWaitIdle(m_LogicalDevice);

        vkDestroyImageView(m_LogicalDevice, m_DepthBufferImageView, nullptr);
        vmaDestroyImage(m_Allocator, m_DepthBuffer.Image, m_DepthBuffer.Allocation);

        CleanupSwapChain(window);

        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_PhysicalDevice, m_Surface, &m_SwapChainSupport.capabilities);
        CreateSwapChain(window);
        CreateDepthBuffer(window.Extent);
        CreateFramebuffers(window);
    }

    void VulkanRenderer::WaitWhileMinimized() {
        int width = 0, height = 0;
        glfwGetFramebufferSize(m_Window, &width, &height);
        while (width == 0 || height == 0) {
            glfwGetFramebufferSize(m_Window, &width, &height);
            glfwWaitEvents();
        }
    }

    VkShaderModule VulkanRenderer::MakeShaderModule(const char* path) {
        std::ifstream f(path, std::ios::ate | std::ios::binary);
        ASSERT(f.is_open());
        std::size_t size = f.tellg();
        std::vector<char> buffer(size);
        f.seekg(0);
        f.read(buffer.data(), size);

        VkShaderModuleCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        createInfo.codeSize = buffer.size();
        createInfo.pCode = reinterpret_cast<const uint32_t*>(buffer.data());

        VkShaderModule shaderModule;
        VkResult result = vkCreateShaderModule(m_LogicalDevice, &createInfo, nullptr, &shaderModule);
        ASSERT(result == VK_SUCCESS);

        return shaderModule;
    }

    void VulkanRenderer::ValidatePhysicalDeviceExtentions() {
        uint32_t extensionCount;
        vkEnumerateDeviceExtensionProperties(m_PhysicalDevice, nullptr, &extensionCount, nullptr);
        std::vector<VkExtensionProperties> availableExtensions(extensionCount);
        vkEnumerateDeviceExtensionProperties(m_PhysicalDevice, nullptr, &extensionCount, availableExtensions.data());

        uint32_t requiredExtentionsFound{};
        for (auto& requiredEx : m_RequiredDeviceExtentions) {
            bool extentionFound = false;
            for (auto availableEx : availableExtensions) {
                if (strcmp(availableEx.extensionName, requiredEx) == 0) {
                    requiredExtentionsFound++;
                    extentionFound = true;
                    break;
                }
            }
            if (!extentionFound) {
                LOG("Extention not found: ");
                LOG(requiredEx);
                LOG("\n");
            }
        }
        ASSERT(requiredExtentionsFound == m_RequiredDeviceExtentions.size());
    }

    void VulkanRenderer::ValidatePhysicalDeviceSurfaceCapabilities() {
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_PhysicalDevice, m_Surface, &m_SwapChainSupport.capabilities);

        uint32_t formatCount;
        vkGetPhysicalDeviceSurfaceFormatsKHR(m_PhysicalDevice, m_Surface, &formatCount, nullptr);
        if (formatCount != 0) {
            m_SwapChainSupport.formats.resize(formatCount);
            vkGetPhysicalDeviceSurfaceFormatsKHR(m_PhysicalDevice, m_Surface, &formatCount, m_SwapChainSupport.formats.data());
        }

        uint32_t presentModeCount;
        vkGetPhysicalDeviceSurfacePresentModesKHR(m_PhysicalDevice, m_Surface, &presentModeCount, nullptr);
        if (presentModeCount != 0) {
            m_SwapChainSupport.presentModes.resize(presentModeCount);
            vkGetPhysicalDeviceSurfacePresentModesKHR(m_PhysicalDevice, m_Surface, &presentModeCount, m_SwapChainSupport.presentModes.data());
        }

        bool swapChainAdequate = false;
        swapChainAdequate = !m_SwapChainSupport.formats.empty() && !m_SwapChainSupport.presentModes.empty();
        ASSERT(swapChainAdequate);
    }

    VkSurfaceFormatKHR VulkanRenderer::GetSurfaceFormat(VkFormat format, VkColorSpaceKHR colorSpace) {
        for (const auto& availableFormat : m_SwapChainSupport.formats) {
            if (availableFormat.format == format && availableFormat.colorSpace == colorSpace) {
                return availableFormat;
            }
        }
        return m_SwapChainSupport.formats[0];
    }

    VkPresentModeKHR VulkanRenderer::GetPresentMode(VkPresentModeKHR presentMode) {
        for (const auto& availablePresentMode : m_SwapChainSupport.presentModes) {
            if (availablePresentMode == presentMode) {
                return availablePresentMode;
            }
        }
        return VK_PRESENT_MODE_FIFO_KHR;
    }

    VkExtent2D VulkanRenderer::GetWindowExtent() {
        if (m_SwapChainSupport.capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
            return m_SwapChainSupport.capabilities.currentExtent;
        } else {
            int width, height;
            glfwGetFramebufferSize(m_Window, &width, &height);

            VkExtent2D actualExtent = {
                static_cast<uint32_t>(width),
                static_cast<uint32_t>(height)
            };

            actualExtent.width = std::clamp(actualExtent.width,
                m_SwapChainSupport.capabilities.minImageExtent.width,
                m_SwapChainSupport.capabilities.maxImageExtent.width);
            actualExtent.height = std::clamp(actualExtent.height,
                m_SwapChainSupport.capabilities.minImageExtent.height,
                m_SwapChainSupport.capabilities.maxImageExtent.height);

            return actualExtent;
        }
    }

    VkFormat VulkanRenderer::FindDepthFormat() {
        std::vector<VkFormat> formats;
        formats.push_back(VK_FORMAT_D32_SFLOAT);
        formats.push_back(VK_FORMAT_D32_SFLOAT_S8_UINT);
        formats.push_back(VK_FORMAT_D24_UNORM_S8_UINT);

        return FindSupportedFormat(
            formats,
            VK_IMAGE_TILING_OPTIMAL,
            VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
    }

    VkFormat VulkanRenderer::FindSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features) {
        for (VkFormat format : candidates) {
            VkFormatProperties props;
            vkGetPhysicalDeviceFormatProperties(m_PhysicalDevice, format, &props);

            if ((tiling == VK_IMAGE_TILING_LINEAR) &&
                ((props.linearTilingFeatures & features) == features)) {
                return format;
            } else if ((tiling == VK_IMAGE_TILING_OPTIMAL) &&
                ((props.optimalTilingFeatures & features) == features)) {
                return format;
            }
        }
        return VK_FORMAT_UNDEFINED;
    }

    VkDescriptorSet VulkanRenderer::AllocateTextureDescriptorSet() {
        VkDescriptorSet textureDescriptorSet{};
        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = m_TextureDescriptorPool;
        allocInfo.descriptorSetCount = 1;
        allocInfo.pSetLayouts = &m_DescriptorSetLayouts[RenderPipelineKeys::Basic]; // should probably be gotten from pipeline abstraction.
        VkResult result = vkAllocateDescriptorSets(m_LogicalDevice, &allocInfo, &textureDescriptorSet);
        ASSERT(result == VK_SUCCESS);
        return textureDescriptorSet;
    }

    void VulkanRenderer::FreeTextureDescriptorSet(VkDescriptorSet desctriptorSet) {
        vkFreeDescriptorSets(m_LogicalDevice, m_TextureDescriptorPool, 1, &desctriptorSet);
    }

    void VulkanRenderer::PopulateQueueFamilyIndices(QueueFamilyIndices& QFI) {
        uint32_t queueFamilyCount{};
        vkGetPhysicalDeviceQueueFamilyProperties2(m_PhysicalDevice, &queueFamilyCount, nullptr);
        std::vector<VkQueueFamilyProperties2> qfp(queueFamilyCount);
        for (auto& qp : qfp) {
            qp.sType = VK_STRUCTURE_TYPE_QUEUE_FAMILY_PROPERTIES_2;
        }
        vkGetPhysicalDeviceQueueFamilyProperties2(m_PhysicalDevice, &queueFamilyCount, qfp.data());
        VkBool32 presentSupport = false;
        for (int i = 0; i < qfp.size(); i++) {
            if (qfp[i].queueFamilyProperties.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                QFI.Graphics = i;
            }
            if (qfp[i].queueFamilyProperties.queueFlags & VK_QUEUE_COMPUTE_BIT) {
                QFI.Compute = i;
            }
            if (qfp[i].queueFamilyProperties.queueFlags & VK_QUEUE_TRANSFER_BIT) {
                QFI.Transfer = i;
            }
            if (qfp[i].queueFamilyProperties.queueFlags & VK_QUEUE_SPARSE_BINDING_BIT) {
                QFI.SparseBinding = i;
            }
            vkGetPhysicalDeviceSurfaceSupportKHR(m_PhysicalDevice, i, m_Surface, &presentSupport);
            if (presentSupport && QFI.PresentIndex == -1) {
                QFI.PresentIndex = i;
            }
        }
    }
}
