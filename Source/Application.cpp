#include "Application.h"
#include "Timer.h"
#include "Components.h"
#include "Input.h"
#include "Utils.h"
#include "Physics.h"
#include <string>
#include "stb_image.h"
#include "imgui_impl_glfw.h"
#include "LasLoader.h"
#include "Octree.h"
#include "Simulate.h"

#include "Renderer/ForwardSceneRenderer.h"
#include "Renderer/DeferredSceneRenderer.h"

#include "GameMode/PhysicsGM.h"
#include "GameMode/SponzaGM.h"

namespace FLOOF {
    Application::Application() : m_EditorCamera(glm::vec3(0.f, 30.f, -30.f)) {
        // Init glfw and create window
        glfwInit();
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        m_Window = glfwCreateWindow(1600, 900, "Floof    FPS: 0.0", nullptr, nullptr);

        IMGUI_CHECKVERSION();
        m_ImguiContext = ImGui::CreateContext();
        ImGui::SetCurrentContext(m_ImguiContext);
        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;       // Enable Keyboard Controls
        //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;           // Enable Docking
        //io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;         // Enable Multi-Viewport / Platform Windows
        //io.ConfigViewportsNoAutoMerge = true;
        //io.ConfigViewportsNoTaskBarIcon = true;  // Enable Gamepad Controls

        ImGui::StyleColorsDark();

        // Init Renderer and Imgui
        ImGui_ImplGlfw_InitForVulkan(m_Window, true);
        m_Renderer = new VulkanRenderer(m_Window);
        auto ImguiInitInfo = m_Renderer->GetImguiInitInfo();
        auto ImguiRenderPass = m_Renderer->GetImguiRenderPass();
        ImGui_ImplVulkan_Init(&ImguiInitInfo, ImguiRenderPass);
        auto commandBuffer = m_Renderer->AllocateBeginOneTimeCommandBuffer();
        ImGui_ImplVulkan_CreateFontsTexture(commandBuffer);
        m_Renderer->EndSubmitFreeCommandBuffer(commandBuffer);
        ImGui_ImplVulkan_DestroyFontUploadObjects();

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

        // Register key callbacks
        Input::Init(m_Window);
        // Init Logger. Writes to specified log file.
        Utils::Logger::s_Logger = new Utils::Logger("Floof.log");

        /*SceneRenderer*/
        SetRendererType(SceneRendererType::Forward);
        
        /*GameMode*/
        SetGameModeType(GameModeType::Physics);
    }

    Application::~Application() {
        ImGui_ImplVulkan_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext(m_ImguiContext);

        MeshComponent::ClearMeshDataCache();
        TextureComponent::ClearTextureDataCache();

        DestroyGameMode();
        DestroySceneRenderer();
        delete m_Renderer;
        delete Utils::Logger::s_Logger;

        glfwDestroyWindow(m_Window);
        glfwTerminate();
    }

    int Application::Run() {
        {
            glm::vec3 cameraPos(0.3f, 0.2f, -1.3f);
        }

        SetRenderCamera(m_EditorCamera);

        if (m_GameMode) m_GameMode->OnCreate();

        Timer timer;
        float titleBarUpdateTimer{};
        float titlebarUpdateRate = 0.1f;
        float frameCounter{};

        while (!glfwWindowShouldClose(m_Window)) {
            glfwPollEvents();

            double deltaTime = timer.Delta();
            frameCounter++;
            titleBarUpdateTimer += static_cast<float>(deltaTime);

            if (titleBarUpdateTimer > titlebarUpdateRate) {
                float avgDeltaTime = titleBarUpdateTimer / frameCounter;
                float fps;
                fps = 1.0f / avgDeltaTime;
                std::string title = "Floof FPS: " + std::to_string(fps);
                glfwSetWindowTitle(m_Window, title.c_str());
                titleBarUpdateTimer = 0.f;
                frameCounter = 0.f;
            }

            if (deltaTime > 0.1f) {
                deltaTime = 0.1f;
            }

            ImGui_ImplVulkan_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();

            Update(deltaTime);
            Draw();
        }

        m_Renderer->FinishAllFrames();
        m_Scene.Clear();

        return 0;
    }

    void Application::UpdateImGui(float deltaTime)
    {
        static int selectedRenderType = 0;
        static int selectedGameType = 0;

        ImGui::Begin("Application");
        if (ImGui::Combo("SceneRendererType", 
            &selectedRenderType,
            SceneRendererTypeStrings, 
            IM_ARRAYSIZE(SceneRendererTypeStrings)))
        {
            SetRendererType(static_cast<SceneRendererType>(selectedRenderType));
        }
        if (ImGui::Combo("GameMode",
            &selectedGameType,
            GameModeTypeStrings,
            IM_ARRAYSIZE(GameModeTypeStrings)))
        {
            SetGameModeType(static_cast<GameModeType>(selectedGameType));
        }
        ImGui::End();
    }

    void Application::UpdateCameraSystem(float deltaTime)
    {
        auto moveAmount = static_cast<float>(m_CameraSpeed * deltaTime);
        if (Input::Key(GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) {
            moveAmount *= 8;
        }
        if (Input::Key(GLFW_KEY_W) == GLFW_PRESS) {
            m_EditorCamera.MoveForward(moveAmount);
        }
        if (Input::Key(GLFW_KEY_S) == GLFW_PRESS) {
            m_EditorCamera.MoveForward(-moveAmount);
        }
        if (Input::Key(GLFW_KEY_D) == GLFW_PRESS) {
            m_EditorCamera.MoveRight(moveAmount);
        }
        if (Input::Key(GLFW_KEY_A) == GLFW_PRESS) {
            m_EditorCamera.MoveRight(-moveAmount);
        }
        if (Input::Key(GLFW_KEY_Q) == GLFW_PRESS) {
            m_EditorCamera.MoveUp(-moveAmount);
        }
        if (Input::Key(GLFW_KEY_E) == GLFW_PRESS) {
            m_EditorCamera.MoveUp(moveAmount);
        }
        static glm::vec2 oldMousePos = glm::vec2(0.f);
        glm::vec2 mousePos = Input::MousePos();
        glm::vec2 mouseDelta = mousePos - oldMousePos;
        oldMousePos = mousePos;
        static constexpr float mouseSpeed = 0.002f;
        if (Input::MouseButton(GLFW_MOUSE_BUTTON_2) == GLFW_PRESS
            || Input::Key(GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS) {
            m_EditorCamera.Yaw(mouseDelta.x * mouseSpeed);
            m_EditorCamera.Pitch(mouseDelta.y * mouseSpeed);
        }
    }

    void Application::Update(double deltaTime) {
        UpdateCameraSystem(deltaTime);
        UpdateImGui(deltaTime);

        m_Scene.OnUpdatePhysics(deltaTime);

        if (m_GameMode) m_GameMode->OnUpdateEditor(deltaTime);
    }

    void Application::Draw() {
        if (m_SceneRenderer)
        {
            auto& m_Registry = m_Scene.GetCulledScene();
            m_SceneRenderer->Render(m_Registry);
        }
        auto* vulkanWindow = m_Renderer->GetVulkanWindow();
        m_Renderer->EndAndSubmitGraphics(
            vulkanWindow->Frames[vulkanWindow->FrameIndex].CommandBuffer,
            vulkanWindow->Frames[vulkanWindow->FrameIndex].ImageAvailableSemaphore,
            vulkanWindow->Frames[vulkanWindow->FrameIndex].RenderFinishedSemaphore,
            vulkanWindow->Frames[vulkanWindow->FrameIndex].Fence);
        // Update and Render additional Platform Windows
        //ImGuiIO& io = ImGui::GetIO();
        //if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        //    ImGui::UpdatePlatformWindows();
        //    ImGui::RenderPlatformWindowsDefault();
        //}
        m_Renderer->Present(*vulkanWindow);
    }

    void Application::SetRendererType(SceneRendererType type)
    {
        if (type == m_SceneRendererType && m_SceneRenderer) return;

        m_SceneRendererType = type;
        UpdateSceneRenderer();
    }

    SceneRendererType Application::GetRendererType() const
    {
        return m_SceneRendererType;
    }

    void Application::SetRenderCamera(CameraComponent& cam)
    {
        m_RenderCamera = &cam;
    }

    CameraComponent* Application::GetRenderCamera()
    {
        return m_RenderCamera;
    }

    void Application::SetGameModeType(GameModeType type)
    {
        if (type == m_GameModeType && m_GameMode) return;
    
        m_GameModeType = type;

        UpdateGameMode();
    }

    GameModeType Application::GetGameModeType() const
    {
        return m_GameModeType;
    }

    void Application::CreateSceneRenderer()
    {
        if (m_SceneRenderer)
        {
            Utils::Logger::s_Logger->log(Utils::Logger::ERROR, "SceneRenderer exists! Delete previous instance to create a new one\n");
            return;
        }

        switch (m_SceneRendererType)
        {
        case SceneRendererType::Forward:
        {
            m_SceneRenderer = new ForwardSceneRenderer;
        }
        break;
        case SceneRendererType::Deferred:
        {
            m_SceneRenderer = new DeferredSceneRenderer;
        }
        break;
        default:
        {
            m_SceneRenderer = nullptr;
            LOG("SceneRendererType is invalid\n");
        }
        break;
        }
    }

    void Application::DestroySceneRenderer()
    {
        m_Renderer->FinishAllFrames();
        delete m_SceneRenderer; m_SceneRenderer = nullptr;
    }

    void Application::UpdateSceneRenderer()
    {
        DestroySceneRenderer();
        CreateSceneRenderer();
    }

    void Application::CreateGameMode()
    {
        if (m_GameMode)
        {
            Utils::Logger::s_Logger->log(Utils::Logger::ERROR, "GameMode exists! Delete previous instance to create a new one\n"); 
            return; 
        }
            
        switch (m_GameModeType)
        {
        case GameModeType::Physics:
        {
            m_GameMode = new PhysicsGM(m_Scene);
            m_GameMode->OnCreate();
            m_Scene.GetPhysicSystem()->UpdateDynamicWorld();
        }
        break;
        case GameModeType::Sponza:
        {
            m_GameMode = new SponzaGM(m_Scene);
            m_GameMode->OnCreate();
        }
        break;
        default:
        {
            m_GameMode = nullptr;
            LOG("GameModeType is invalid\n");
        }
        break;
        }
    }

    void Application::DestroyGameMode()
    {
        delete m_GameMode; m_GameMode = nullptr;
        m_Renderer->FinishAllFrames();
        m_Scene.Clear();
    }

    void Application::UpdateGameMode()
    {
        DestroyGameMode();
        CreateGameMode();
    }

}