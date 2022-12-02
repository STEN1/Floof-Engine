#include "Application.h"
#include "Timer.h"
#include "Components.h"
#include "Input.h"
#include "Utils.h"
#include <string>
#include "stb_image/stb_image.h"
#include "imgui_impl_glfw.h"
#include "LasLoader.h"
#include "HeightmapLoader.h"
#include "Renderer/ModelManager.h"
#include "SoundManager.h"
#include "NativeScripts/TestScript.h"
#include <filesystem>
#include "Editor/EditorLayer.h"
#include "Renderer/TextureManager.h"
#include "HeightField.h"

// Temp OpenAL includes
//#include <AL/al.h>
//#include "alc.h"
//#define DR_WAV_IMPLEMENTATION
//#include <dr_libs/dr_wav.h>

namespace FLOOF {
    Application::Application() {
        // Init glfw and create window
        SoundManager::InitOpenAL();
        glfwInit();
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        m_Window = glfwCreateWindow(1600, 900, "Floof    FPS: 0.0", nullptr, nullptr);

        IMGUI_CHECKVERSION();
        m_ImguiContext = ImGui::CreateContext();
        ImGui::SetCurrentContext(m_ImguiContext);
        ImGuiIO &io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;       // Enable Keyboard Controls
        //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;           // Enable Docking
        io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;         // Enable Multi-Viewport / Platform Windows
#ifdef WIN32
        io.ConfigFlags |= ImGuiConfigFlags_DpiEnableScaleViewports;
        io.ConfigFlags |= ImGuiConfigFlags_DpiEnableScaleFonts;
#endif

        //io.ConfigViewportsNoAutoMerge = true;
        //io.ConfigViewportsNoTaskBarIcon = true;

        ImGui::StyleColorsDark();

        // Init Renderer and Imgui
        ImGui::GetDrawData();
        ImGui::GetDrawListSharedData();
        ImGui_ImplGlfw_InitForVulkan(m_Window, true);
        m_Renderer = new VulkanRenderer(m_Window);
        auto ImguiInitInfo = m_Renderer->GetImguiInitInfo();
        auto ImguiRenderPass = m_Renderer->GetImguiRenderPass();
        ImGui_ImplVulkan_Init(&ImguiInitInfo, ImguiRenderPass);
        auto commandBuffer = m_Renderer->BeginSingleUseCommandBuffer();
        ImGui_ImplVulkan_CreateFontsTexture(commandBuffer);
        m_Renderer->EndSingleUseCommandBuffer(commandBuffer);
        ImGui_ImplVulkan_DestroyFontUploadObjects();

        stbi_set_flip_vertically_on_load(false);
        // Upload icons for windows and taskbar
        GLFWimage images[3]{};
        int channels{};
        images[0].pixels = stbi_load("Assets/Icon16x16.png", &images[0].width, &images[0].height, &channels, 4);
        ASSERT(channels == 4);
        images[1].pixels = stbi_load("Assets/Icon32x32.png", &images[1].width, &images[1].height, &channels, 4);
        ASSERT(channels == 4);
        images[2].pixels = stbi_load("Assets/Icon48x48.png", &images[2].width, &images[2].height, &channels, 4);
        ASSERT(channels == 4);
        glfwSetWindowIcon(m_Window, 3, images);
        for (uint32_t i = 0; i < 3; i++) {
            stbi_image_free(images[i].pixels);
        }
        m_ApplicationLayers.emplace_back(std::make_unique<EditorLayer>());
    }

    void Application::CleanApplication() {
        m_Renderer->FinishAllFrames();

        m_ApplicationLayers.clear();

        ImGui_ImplVulkan_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext(m_ImguiContext);

        ModelManager::DestroyAll();
        TextureManager::DestroyAll();

        delete m_Renderer;

        glfwDestroyWindow(m_Window);
        glfwTerminate();

        SoundManager::CleanOpenAL();
    }

    int Application::Run() {
        Timer timer;
        double titleBarUpdateTimer{};
        double titlebarUpdateRate = 0.1f;
        double frameCounter{};

        while (!glfwWindowShouldClose(m_Window)) {
            glfwPollEvents();

            double deltaTime = timer.Delta();
            frameCounter++;
            titleBarUpdateTimer += deltaTime;

            if (titleBarUpdateTimer > titlebarUpdateRate) {
                double avgDeltaTime = titleBarUpdateTimer / frameCounter;
                double fps;
                fps = 1.0 / avgDeltaTime;
                std::string title = "Floof FPS: " + std::to_string(fps);
                glfwSetWindowTitle(m_Window, title.c_str());
                titleBarUpdateTimer = 0.f;
                frameCounter = 0.f;
            }

            if (deltaTime > 0.1) {
                deltaTime = 0.1;
            }

            ImGui_ImplVulkan_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();

            UpdateImGui(deltaTime);
            Update(deltaTime);
            Draw(deltaTime);
        }
        CleanApplication();
        return 0;
    }

    void Application::UpdateImGui(double deltaTime) {
        for (auto &layer: m_ApplicationLayers) {
            layer->OnImGuiUpdate(deltaTime);
        }
    }

    void Application::Update(double deltaTime) {
        for (auto& layer : m_ApplicationLayers) {
            layer->OnUpdate(deltaTime);
        }
    }

    void Application::Draw(double deltaTime) {
        m_Renderer->NewFrame();

        auto *vulkanWindow = m_Renderer->GetVulkanWindow();
        auto &currentFrameData = vulkanWindow->Frames[vulkanWindow->FrameIndex];

        // Draw all layers
        VkSemaphore waitSemaphore = currentFrameData.ImageAvailableSemaphore;
        for (auto& layer : m_ApplicationLayers) {
            waitSemaphore = layer->OnDraw(deltaTime, waitSemaphore);
        }
        VkSemaphore signalSemaphore = currentFrameData.RenderFinishedSemaphore;

        // Start ImGui renderpass and draw ImGui
        VkRenderPassBeginInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = m_Renderer->GetImguiRenderPass();
        renderPassInfo.framebuffer = vulkanWindow->FrameBuffers[vulkanWindow->ImageIndex];
        renderPassInfo.renderArea.offset = {0, 0};
        renderPassInfo.renderArea.extent = vulkanWindow->Extent;
        VkClearValue clearColor[1]{};
        clearColor[0].color = {{0.0f, 0.0f, 0.0f, 1.0f}};
        renderPassInfo.clearValueCount = 1;
        renderPassInfo.pClearValues = clearColor;

        auto imGuiCommandBuffer = m_Renderer->AllocateCommandBuffer();
        m_Renderer->BeginSingleUseCommandBuffer(imGuiCommandBuffer);

        m_Renderer->StartRenderPass(imGuiCommandBuffer, &renderPassInfo);
        // Render ImGui
        ImGui::Render();
        ImDrawData *drawData = ImGui::GetDrawData();
        ImGui_ImplVulkan_RenderDrawData(drawData, imGuiCommandBuffer);

        auto &io = ImGui::GetIO();
        // Update and Render additional Platform Windows
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
        }

        // End ImGui renderpass
        m_Renderer->EndRenderPass(imGuiCommandBuffer);

        VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = &waitSemaphore;
        submitInfo.pWaitDstStageMask = waitStages;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &imGuiCommandBuffer;
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = &signalSemaphore;

        m_Renderer->QueueSubmitGraphics(1, &submitInfo, currentFrameData.Fence);

        // Waits for render finished semaphore then presents
        m_Renderer->Present();
    }
}
