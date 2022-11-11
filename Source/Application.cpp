#include "Application.h"
#include "Timer.h"
#include "Components.h"
#include "Input.h"
#include "Utils.h"
#include <string>
#include "stb_image/stb_image.h"
#include "imgui_impl_glfw.h"
#include "LasLoader.h"
#include "Renderer/ModelManager.h"
#include "SoundManager.h"
#include "Renderer/ForwardSceneRenderer.h"
#include "NativeScripts/TestScript.h"
#include <filesystem>
#include "Editor/EditorLayer.h"
#include "NativeScripts/MonsterTruckScript.h"

// Temp OpenAL includes
//#include <AL/al.h>
//#include "alc.h"
//#define DR_WAV_IMPLEMENTATION
//#include <dr_libs/dr_wav.h>

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
        io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;         // Enable Multi-Viewport / Platform Windows
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

        m_SceneRenderer = std::make_unique<ForwardSceneRenderer>();

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

        SelectDebugScene(DebugScenes::Physics);

        m_ApplicationLayers.emplace_back(std::make_unique<EditorLayer>());

        SoundManager::InitOpenAL();

    }

    void Application::CleanApplication() {
        m_Renderer->FinishAllFrames();

        m_Scene = nullptr;
        m_SceneRenderer = nullptr;

        ImGui_ImplVulkan_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext(m_ImguiContext);

        MeshComponent::ClearMeshDataCache();
        ModelManager::Get().DestroyAll();
        TextureComponent::ClearTextureDataCache();


        delete m_Renderer;

        glfwDestroyWindow(m_Window);
        glfwTerminate();

        SoundManager::CleanOpenAL();
    }

    int Application::Run() {
        {
            glm::vec3 cameraPos(0.3f, 0.2f, -1.3f);
        }

        SetRenderCamera(m_EditorCamera);

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
        CleanApplication();
        return 0;
    }

    void Application::UpdateImGui(float deltaTime)
    {
        for (auto& layer : m_ApplicationLayers) {
            layer->OnImGuiUpdate(deltaTime);
        }
    }

    void Application::UpdateCameraSystem(float deltaTime)
    {
        auto moveAmount = static_cast<float>(m_CameraSpeed * deltaTime);
        if (Input::Key(ImGuiKey_LeftShift)) {
            moveAmount *= 8;
        }
        if (Input::Key(ImGuiKey_W)) {
            m_EditorCamera.MoveForward(moveAmount);
        }
        if (Input::Key(ImGuiKey_S)) {
            m_EditorCamera.MoveForward(-moveAmount);
        }
        if (Input::Key(ImGuiKey_D)) {
            m_EditorCamera.MoveRight(moveAmount);
        }
        if (Input::Key(ImGuiKey_A)) {
            m_EditorCamera.MoveRight(-moveAmount);
        }
        if (Input::Key(ImGuiKey_Q)) {
            m_EditorCamera.MoveUp(-moveAmount);
        }
        if (Input::Key(ImGuiKey_E)) {
            m_EditorCamera.MoveUp(moveAmount);
        }
        static glm::vec2 oldMousePos = glm::vec2(0.f);
        glm::vec2 mousePos = Input::MousePos();
        glm::vec2 mouseDelta = mousePos - oldMousePos;
        oldMousePos = mousePos;
        static constexpr float mouseSpeed = 0.002f;
        if (Input::MouseButton(ImGuiMouseButton_Right)
            || Input::Key(ImGuiKey_LeftCtrl)) {
            m_EditorCamera.Yaw(mouseDelta.x * mouseSpeed);
            m_EditorCamera.Pitch(mouseDelta.y * mouseSpeed);
        }
    }

    void Application::Update(double deltaTime) {
        UpdateCameraSystem(deltaTime);
        UpdateImGui(deltaTime);
        m_Scene->OnUpdate(deltaTime);

        //if (m_GameMode) m_GameMode->OnUpdateEditor(deltaTime);
    }

    void Application::Draw() {
        auto* vulkanWindow = m_Renderer->GetVulkanWindow();
        m_Renderer->NewFrame();
        auto& currentFrameData = vulkanWindow->Frames[vulkanWindow->FrameIndex];

        VkSemaphore waitSemaphore = currentFrameData.MainPassEndSemaphore;
        VkSemaphore signalSemaphore = currentFrameData.RenderFinishedSemaphore;

        ImGui::PushStyleVar(ImGuiStyleVar_WindowMinSize, ImVec2(300.f, 300.f));
        ImGui::Begin("Scene renderer");
        ImGui::PopStyleVar();

        ImVec2 canvasOffset = ImGui::GetWindowPos();
        ImVec2 canvas_p0 = ImGui::GetWindowContentRegionMin();
        ImVec2 canvas_p1 = ImGui::GetWindowContentRegionMax();
        canvas_p0.x += canvasOffset.x;
        canvas_p0.y += canvasOffset.y;
        canvas_p1.x += canvasOffset.x;
        canvas_p1.y += canvasOffset.y;

        glm::vec2 sceneCanvasExtent{ canvas_p1.x - canvas_p0.x, canvas_p1.y - canvas_p0.y };
        if (sceneCanvasExtent.x < 2.f || sceneCanvasExtent.y < 2.f)
            sceneCanvasExtent = glm::vec2(0.f);

        VkDescriptorSet sceneTexture = VK_NULL_HANDLE;

        if (m_SceneRenderer) {
            sceneTexture = m_SceneRenderer->RenderToTexture(m_Scene, sceneCanvasExtent);
        } else {
            waitSemaphore = currentFrameData.ImageAvailableSemaphore;
            signalSemaphore = currentFrameData.RenderFinishedSemaphore;
        }

        if (sceneTexture != VK_NULL_HANDLE) {
            ImDrawList* draw_list = ImGui::GetWindowDrawList();
            draw_list->AddImage(sceneTexture, canvas_p0, canvas_p1, ImVec2(0, 0), ImVec2(1, 1));
        } else {
            waitSemaphore = currentFrameData.ImageAvailableSemaphore;
            signalSemaphore = currentFrameData.RenderFinishedSemaphore;
        }

        ImGui::End();

        // Start ImGui renderpass and draw ImGui
        VkRenderPassBeginInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = m_Renderer->GetImguiRenderPass();
        renderPassInfo.framebuffer = vulkanWindow->FrameBuffers[vulkanWindow->ImageIndex];
        renderPassInfo.renderArea.offset = { 0, 0 };
        renderPassInfo.renderArea.extent = vulkanWindow->Extent;
        VkClearValue clearColor[1]{};
        clearColor[0].color = { {0.0f, 0.0f, 0.0f, 1.0f} };
        renderPassInfo.clearValueCount = 1;
        renderPassInfo.pClearValues = clearColor;

        m_Renderer->StartRenderPass(currentFrameData.ImGuiCommandBuffer, &renderPassInfo);
        // Render ImGui
        ImGui::Render();
        ImDrawData* drawData = ImGui::GetDrawData();
        ImGui_ImplVulkan_RenderDrawData(drawData, currentFrameData.ImGuiCommandBuffer);

        auto& io = ImGui::GetIO();
        // Update and Render additional Platform Windows
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
        }

        // End ImGui renderpass
        m_Renderer->EndRenderPass(currentFrameData.ImGuiCommandBuffer);

        VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = &waitSemaphore;
        submitInfo.pWaitDstStageMask = waitStages;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &currentFrameData.ImGuiCommandBuffer;
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = &signalSemaphore;

        m_Renderer->QueueSubmitGraphics(1, &submitInfo, currentFrameData.Fence);

        // Waits for render finished semaphore then presents
        m_Renderer->Present();
    }

    void Application::SelectDebugScene(DebugScenes type) {
        m_CurrentDebugScene = type;
        switch (type) {
            case DebugScenes::Physics:
            {
                MakePhysicsScene();
                break;
            }
            case DebugScenes::Sponza:
            {
                MakeSponsaScene();
                break;
            }
            case DebugScenes::Audio:
            {
                MakeAudioTestScene();
                break;
            }
            case DebugScenes::Landscape:
            {
                MakeLandscapeScene();
                break;
            }
            default:
            {
                LOG("GameModeType is invalid\n");
                break;
            }
        }
        if (m_Scene)
            m_Scene->GetPhysicSystem()->UpdateDynamicWorld();
    }

    void Application::SetRenderCamera(CameraComponent& cam)
    {
        m_RenderCamera = &cam;
    }

    CameraComponent* Application::GetRenderCamera()
    {
        return m_RenderCamera;
    }

    void Application::MakePhysicsScene() {
        m_Scene = std::make_unique<Scene>();

        // TODO: make physics scene.

        {
            auto texture = "Assets/WaterTexture.png";
            auto location = glm::vec3(0.f, -150.f, 0.f);
            auto extents = glm::vec3(800.f, 10.f, 800.f);
            auto mass = 0.f;

            auto entity = m_Scene->CreateEntity("Ground Cube");
            auto &collision = m_Scene->AddComponent<RigidBodyComponent>(entity,location,extents,mass,bt::CollisionPrimitive::Box);
            m_Scene->AddComponent<MeshComponent>(entity, "Assets/IdentityCube.obj");
            m_Scene->AddComponent<TextureComponent>(entity, texture);

            auto & transform = m_Scene->GetComponent<TransformComponent>(entity);
            transform.Position = glm::vec3(collision.Transform.getOrigin().getX(),
                                           collision.Transform.getOrigin().getY(),
                                           collision.Transform.getOrigin().getZ());
            transform.Scale = extents;

        }
        if(false){
            auto entity = m_Scene->CreateEntity("Ground Ball");
            m_Scene->AddComponent<MeshComponent>(entity, "Assets/Ball.obj");
            m_Scene->AddComponent<TextureComponent>(entity, "Assets/LightBlue.png");
            auto& collision = m_Scene->AddComponent<RigidBodyComponent>(entity,glm::vec3(0.f,-150.f,0.f),glm::vec3(75.f),0.f,bt::CollisionPrimitive::Sphere);

            auto & transform = m_Scene->GetComponent<TransformComponent>(entity);
            transform.Position = glm::vec3(0.f,-150.f,0.f);
            transform.Scale = glm::vec3(75.f);
        }
        //make monstertruck
        {
            auto ent = m_Scene->CreateEntity("MonsterTruck");
            m_Scene->AddComponent<NativeScriptComponent>(ent, std::make_unique<MonsterTruckScript>(), m_Scene, ent);
        }

        if (false){
            int height = 5;
            int width = 5;
            float spacing = 5.f;

            bool lightAdded = false;

            for (int y = 0; y < height; y++)
            {
                for (int x = 0; x < width; x++)
                {
                    for(int z = 0; z < width; z++){

                        //spawning balls
                        glm::vec3 location =glm::vec3(x * spacing - (float(width) * spacing * 0.5f), y * spacing+(height*spacing), z * spacing - (float(width) * spacing * 0.5f));
                        const float radius = spacing/2.5f;
                        const glm::vec3 extents = glm::vec3(radius);
                        const float mass = radius*100.f;

                        auto Ball = m_Scene->CreateEntity("Simulated Ball " + std::to_string(x+y+z));
                        m_Scene->AddComponent<MeshComponent>(Ball, "Assets/Ball.obj");
                        m_Scene->AddComponent<TextureComponent>(Ball, "Assets/statue/textures/staue1Color.png");
                        m_Scene->AddComponent<RigidBodyComponent>(Ball,location,extents,mass,bt::CollisionPrimitive::Sphere);
                        m_Scene->AddComponent<PointLightComponent>(Ball);
                        //test python script
                        auto &script = m_Scene->AddComponent<ScriptComponent>(Ball,"Scripts/example.py");

                        auto & transform = m_Scene->GetComponent<TransformComponent>(Ball);
                        transform.Position = location;
                        transform.Scale = extents;

                        //spawn cubes
                        location = glm::vec3(x * spacing - (float(width) * spacing * 0.5f), y * spacing, z * spacing - (float(width) * spacing * 0.5f));

                        auto cube = m_Scene->CreateEntity("Simulated Cube " + std::to_string(x+y+z));
                        m_Scene->AddComponent<RigidBodyComponent>(cube,location,extents,mass,bt::CollisionPrimitive::Box);
                        m_Scene->AddComponent<MeshComponent>(cube, "Assets/IdentityCube.obj");
                        m_Scene->AddComponent<TextureComponent>(cube, "Assets/BallTexture.png");

                        auto & cubeTransform = m_Scene->GetComponent<TransformComponent>(cube);
                        cubeTransform.Position = location;
                        cubeTransform.Scale = extents;
                    }
                }
            }
        }
    }

    void Application::MakeSponsaScene() {
        m_Scene = std::make_unique<Scene>();

        {
            auto ent = m_Scene->CreateEntity("Sponza");
            auto& sm = m_Scene->AddComponent<StaticMeshComponent>(ent);
            m_Scene->AddComponent<TextureComponent>(ent, "Assets/BallTexture.png");
            sm.meshes = ModelManager::Get().LoadModelMesh("Assets/crytek-sponza-noflag/sponza.obj");
            m_Scene->AddComponent<NativeScriptComponent>(ent, std::make_unique<TestScript>(), m_Scene, ent);
        }

        for (float x = 0.f; x < 20.f; x += 5.f) {
            for (float y = 0.f; y < 20.f; y += 5.f) {
                for (float z = 0.f; z < 20.f; z += 5.f) {
                    auto entity = m_Scene->CreateEntity("PointLight ball");
                    m_Scene->AddComponent<MeshComponent>(entity, "Assets/Ball.obj");
                    m_Scene->AddComponent<TextureComponent>(entity, "Assets/BallTexture.png");
                    m_Scene->AddComponent<PointLightComponent>(entity);

                    auto& transform = m_Scene->GetComponent<TransformComponent>(entity);
                    transform.Position = glm::vec3(x, y, z);
                }
            }
        }
    }
    void Application::MakeAudioTestScene() {
        m_Scene = std::make_unique<Scene>();


        {
            auto texture = "Assets/LightBlue.png";
            auto location = glm::vec3(0.f, -150.f, 0.f);
            auto extents = glm::vec3(400.f, 10.f, 400.f);
            auto mass = 0.f;

            auto entity = m_Scene->CreateEntity("Ground Cube");
            auto& collision = m_Scene->AddComponent<RigidBodyComponent>(entity, location, extents, mass, bt::CollisionPrimitive::Box);
            m_Scene->AddComponent<MeshComponent>(entity, "Assets/IdentityCube.obj");
            m_Scene->AddComponent<TextureComponent>(entity, texture);

            auto& transform = m_Scene->GetComponent<TransformComponent>(entity);
            transform.Position = glm::vec3(collision.Transform.getOrigin().getX(),
                collision.Transform.getOrigin().getY(),
                collision.Transform.getOrigin().getZ());
            transform.Scale = extents;

        }
        {
            auto entity = m_Scene->CreateEntity("Ground Ball");
            m_Scene->AddComponent<MeshComponent>(entity, "Assets/Ball.obj");
            m_Scene->AddComponent<TextureComponent>(entity, "Assets/LightBlue.png");
            auto& collision = m_Scene->AddComponent<RigidBodyComponent>(entity, glm::vec3(0.f, -150.f, 0.f), glm::vec3(75.f), 0.f, bt::CollisionPrimitive::Sphere);

            auto& transform = m_Scene->GetComponent<TransformComponent>(entity);
            transform.Position = glm::vec3(0.f, -150.f, 0.f);
            transform.Scale = glm::vec3(75.f);

        }


        {
            //spawning balls
            glm::vec3 location = glm::vec3(3.f,25.f,2.f);
            const float radius = 2.f;
            const glm::vec3 extents = glm::vec3(radius);
            const float mass = radius * 100.f;

            auto Ball = m_Scene->CreateEntity("Simulated Ball " + std::to_string(location.x + location.y + location.z));
            m_Scene->AddComponent<MeshComponent>(Ball, "Assets/Ball.obj");
            m_Scene->AddComponent<TextureComponent>(Ball, "Assets/BallTexture.png");
            m_Scene->AddComponent<RigidBodyComponent>(Ball, location, glm::vec3(radius), mass, bt::CollisionPrimitive::Sphere);

            auto& transform = m_Scene->GetComponent<TransformComponent>(Ball);
            transform.Position = location;
            transform.Scale = extents;

			SoundManager::SetListener(glm::vec3(0.f), glm::vec3(0.f), glm::vec3(1.f, 0.f, 0.f), glm::vec3(0.f, 1.f, 0.f));


            m_Scene->AddComponent<SoundSourceComponent>(Ball, "TestSound_Stereo.wav");
            auto& sound = m_Scene->GetComponent<SoundSourceComponent>(Ball);
            sound.Play();
        }

        



    }
    void Application::MakeLandscapeScene()
    {
        m_Scene = std::make_unique<Scene>();

    }
}
