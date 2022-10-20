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

namespace FLOOF {
    Application::Application() {
        // Init glfw and create window
        glfwInit();
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        m_Window = glfwCreateWindow(1600, 900, "Floof    FPS: 0.0", nullptr, nullptr);

        IMGUI_CHECKVERSION();
        m_ImguiContext = ImGui::CreateContext();
        ImGui::SetCurrentContext(m_ImguiContext);
        ImGuiIO& io = ImGui::GetIO(); (void)io;
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;   // Enable Keyboard Controls
        //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;  // Enable Gamepad Controls

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
        CreateSceneRenderer();

        /*GameMode*/
        SetGameMode(new PhysicsGM(m_Scene));
    }

    Application::~Application() {
        ImGui_ImplVulkan_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext(m_ImguiContext);

        MeshComponent::ClearMeshDataCache();
        TextureComponent::ClearTextureDataCache();

        DestroySceneRenderer();
        delete m_Renderer;
        delete Utils::Logger::s_Logger;

        glfwDestroyWindow(m_Window);
        glfwTerminate();
    }

    int Application::Run() {
        DebugInit();

        {
            auto& m_Registry = m_Scene.GetCulledScene();
            m_CameraEntity = m_Registry.create();
            glm::vec3 cameraPos(0.3f, 0.2f, -1.3f);
            auto& camera = m_Registry.emplace<CameraComponent>(m_CameraEntity, cameraPos);
            camera.Pitch(0.5f);
        }

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

            DebugClearDebugData();

            if (deltaTime > 0.1f) {
                deltaTime = 0.1f;
            }

            ImGui_ImplVulkan_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();

            Update(deltaTime);

            if (m_DebugDraw) {
                DebugUpdateLineBuffer();
            }

            Draw();
        }

        m_Renderer->FinishAllFrames();
        m_Scene.Clear();

        return 0;
    }

    void Application::UpdateImGui(float deltaTime)
    {
        static int selectedItem = 0;

        ImGui::Begin("Application");
        if (ImGui::Combo("SceneRendererType", 
            &selectedItem, 
            SceneRendererTypeStrings, 
            IM_ARRAYSIZE(SceneRendererTypeStrings)))
        {
            SetRendererType(static_cast<SceneRendererType>(selectedItem));
        }
        ImGui::End();
    }

    void Application::UpdateCameraSystem(float deltaTime)
    {
        {	// Update camera.
            auto& m_Registry = m_Scene.GetCulledScene();
            auto view = m_Registry.view<CameraComponent>();
            for (auto [entity, camera] : view.each()) {
                auto moveAmount = static_cast<float>(m_CameraSpeed * deltaTime);
                if (Input::Key(GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) {
                    moveAmount *= 8;
                }
                if (Input::Key(GLFW_KEY_W) == GLFW_PRESS) {
                    camera.MoveForward(moveAmount);
                }
                if (Input::Key(GLFW_KEY_S) == GLFW_PRESS) {
                    camera.MoveForward(-moveAmount);
                }
                if (Input::Key(GLFW_KEY_D) == GLFW_PRESS) {
                    camera.MoveRight(moveAmount);
                }
                if (Input::Key(GLFW_KEY_A) == GLFW_PRESS) {
                    camera.MoveRight(-moveAmount);
                }
                if (Input::Key(GLFW_KEY_Q) == GLFW_PRESS) {
                    camera.MoveUp(-moveAmount);
                }
                if (Input::Key(GLFW_KEY_E) == GLFW_PRESS) {
                    camera.MoveUp(moveAmount);
                }
                static glm::vec2 oldMousePos = glm::vec2(0.f);
                glm::vec2 mousePos = Input::MousePos();
                glm::vec2 mouseDelta = mousePos - oldMousePos;
                oldMousePos = mousePos;
                static constexpr float mouseSpeed = 0.002f;
                if (Input::MouseButton(GLFW_MOUSE_BUTTON_2) == GLFW_PRESS
                    || Input::Key(GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS) {
                    camera.Yaw(mouseDelta.x * mouseSpeed);
                    camera.Pitch(mouseDelta.y * mouseSpeed);
                }
            }
        }
    }

    void Application::Update(double deltaTime) {
        UpdateCameraSystem(deltaTime);
        UpdateImGui(deltaTime);  
        if (m_GameMode) m_GameMode->OnUpdateEditor(deltaTime);
    }

    void Application::DrawDebugLines()
    {
        // World axis
        if (m_BDebugLines[DebugLine::WorldAxis]) {
            DebugDrawLine(glm::vec3(0.f), glm::vec3(10.f, 0.f, 0.f), glm::vec3(1.f, 0.f, 0.f));
            DebugDrawLine(glm::vec3(0.f), glm::vec3(0.f, 10.f, 0.f), glm::vec3(0.f, 1.f, 0.f));
            DebugDrawLine(glm::vec3(0.f), glm::vec3(0.f, 0.f, 10.f), glm::vec3(0.f, 0.f, 1.f));
        }
    }

    void Application::Draw() {
        DrawDebugLines();

        if (m_SceneRenderer)
        {
            auto& m_Registry = m_Scene.GetCulledScene();
            m_SceneRenderer->Render(m_Registry);
        }    

        m_Renderer->SubmitAndPresent();
    }

    void Application::DebugInit() {
        auto& m_Registry = m_Scene.GetCulledScene();

        m_DebugLineBuffer.resize(m_DebugLineSpace);
        m_DebugLineEntity = m_Registry.create();
        m_Registry.emplace<LineMeshComponent>(m_DebugLineEntity, m_DebugLineBuffer);
        m_Registry.emplace<DebugComponent>(m_DebugLineEntity);

        m_DebugSphereEntity = m_Registry.create();
        m_Registry.emplace<LineMeshComponent>(m_DebugSphereEntity, Utils::LineVertexDataFromObj("Assets/Ball.obj"));
        m_Registry.emplace<DebugComponent>(m_DebugSphereEntity);

        m_DebugAABBEntity = m_Registry.create();
        m_Registry.emplace<LineMeshComponent>(m_DebugAABBEntity, Utils::MakeBox(glm::vec3(1.f), glm::vec3(1.f, 0.f, 0.f)));
        m_Registry.emplace<DebugComponent>(m_DebugAABBEntity);

        m_BDebugLines[DebugLine::Velocity] = false;
        m_BDebugLines[DebugLine::Friction] = false;
        m_BDebugLines[DebugLine::Acceleration] = false;
        m_BDebugLines[DebugLine::CollisionShape] = false;
        m_BDebugLines[DebugLine::WorldAxis] = true;
        m_BDebugLines[DebugLine::TerrainTriangle] = false;
        m_BDebugLines[DebugLine::ClosestPointToBall] = false;
        m_BDebugLines[DebugLine::GravitationalPull] = false;
        m_BDebugLines[DebugLine::Path] = false;
        m_BDebugLines[DebugLine::BSpline] = true;
        m_BDebugLines[DebugLine::OctTree] = false;
    }

    void Application::DebugClearLineBuffer() {
        m_DebugLineBuffer.clear();
        m_DebugLineBuffer.reserve(m_DebugLineSpace);
    }

    void Application::DebugClearSpherePositions() {
        m_DebugSphereTransforms.clear();
    }

    void Application::DebugClearAABBTransforms() {
        m_DebugAABBTransforms.clear();
    }

    void Application::DebugClearDebugData() {
        DebugClearLineBuffer();
        DebugClearSpherePositions();
        DebugClearAABBTransforms();
    }

    void Application::DebugUpdateLineBuffer() {
        auto& m_Registry = m_Scene.GetCulledScene();
        auto commandBuffer = m_Renderer->AllocateBeginOneTimeCommandBuffer();
        auto& lineMesh = m_Registry.get<LineMeshComponent>(m_DebugLineEntity);
        lineMesh.UpdateBuffer(m_DebugLineBuffer);
        m_Renderer->EndSubmitFreeCommandBuffer(commandBuffer);
    }

    void Application::DebugToggleDrawNormals() {
        m_DrawNormals = !m_DrawNormals;
    }

    void Application::DebugDrawLine(const glm::vec3& start, const glm::vec3& end, const glm::vec3 color) {
        ColorVertex v;
        v.Color = color;
        v.Pos = start;
        m_DebugLineBuffer.push_back(v);
        v.Pos = end;
        m_DebugLineBuffer.push_back(v);
    }

    void Application::DebugDrawTriangle(const Triangle& triangle, const glm::vec3& color) {
        DebugDrawLine(triangle.A, triangle.B, color);
        DebugDrawLine(triangle.B, triangle.C, color);
        DebugDrawLine(triangle.C, triangle.A, color);
        glm::vec3 midPoint = (triangle.A + triangle.B + triangle.C) / 3.f;
        DebugDrawLine(midPoint, midPoint + (triangle.N * 0.02f), color);
    }

    void Application::DebugDrawSphere(const glm::vec3& pos, float radius) {
        glm::mat4 transform = glm::translate(pos);
        transform = glm::scale(transform, glm::vec3(radius));
        m_DebugSphereTransforms.push_back(transform);
    }

    void Application::DebugDrawAABB(const glm::vec3& pos, const glm::vec3& extents) {
        glm::mat4 transform(1.f);
        transform[3].x = pos.x;
        transform[3].y = pos.y;
        transform[3].z = pos.z;

        transform[0].x = extents.x;
        transform[1].y = extents.y;
        transform[2].z = extents.z;
        m_DebugAABBTransforms.push_back(transform);
    }

    void Application::SetRendererType(SceneRendererType type)
    {
        if (type == m_SceneRendererType) return;

        m_SceneRendererType = type;
        UpdateSceneRenderer();
    }

    SceneRendererType Application::GetRendererType() const
    {
        return m_SceneRendererType;
    }

    void Application::SetGameMode(GameMode* gm)
    {
        if (gm == m_GameMode) return;

        delete m_GameMode;
        m_GameMode = gm;
        m_GameMode->OnCreate();
    }

    const GameMode* Application::GetGameMode() const
    {
        return m_GameMode;
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

    void Application::DebugDrawPath(std::vector<glm::vec3>& path) {
        for (int i{ 1 }; i < path.size(); i++) {
            DebugDrawLine(path[i - 1], path[i], glm::vec3(255.f, 255.f, 255.f));
        }
    }
}