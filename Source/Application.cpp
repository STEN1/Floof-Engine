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
#include "NativeScripts/MonsterTruckScript.h"
#include "Renderer/TextureManager.h"
#include "HeightField.h"

// Temp OpenAL includes
//#include <AL/al.h>
//#include "alc.h"
//#define DR_WAV_IMPLEMENTATION
//#include <dr_libs/dr_wav.h>

namespace FLOOF {
    Application::Application() : m_EditorCamera(glm::vec3(0.f, 30.f, -30.f)) {
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

        m_SceneRenderer = std::make_unique<SceneRenderer>();

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

        SelectDebugScene(DebugScenes::PhysicsPlayground);

        m_ApplicationLayers.emplace_back(std::make_unique<EditorLayer>());


    }

    void Application::CleanApplication() {
        m_Renderer->FinishAllFrames();

        m_Scene = nullptr;
        m_SceneRenderer = nullptr;

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

    void Application::UpdateImGui(float deltaTime) {
        for (auto &layer: m_ApplicationLayers) {
            layer->OnImGuiUpdate(deltaTime);
        }
    }

    void Application::UpdateCameraSystem(float deltaTime) {
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
        auto *vulkanWindow = m_Renderer->GetVulkanWindow();
        m_Renderer->NewFrame();
        auto &currentFrameData = vulkanWindow->Frames[vulkanWindow->FrameIndex];

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

        glm::vec2 sceneCanvasExtent{canvas_p1.x - canvas_p0.x, canvas_p1.y - canvas_p0.y};
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
            ImDrawList *draw_list = ImGui::GetWindowDrawList();
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
        renderPassInfo.renderArea.offset = {0, 0};
        renderPassInfo.renderArea.extent = vulkanWindow->Extent;
        VkClearValue clearColor[1]{};
        clearColor[0].color = {{0.0f, 0.0f, 0.0f, 1.0f}};
        renderPassInfo.clearValueCount = 1;
        renderPassInfo.pClearValues = clearColor;

        m_Renderer->ResetAndBeginCommandBuffer(currentFrameData.ImGuiCommandBuffer);
        m_Renderer->StartRenderPass(currentFrameData.ImGuiCommandBuffer, &renderPassInfo);
        // Render ImGui
        ImGui::Render();
        ImDrawData *drawData = ImGui::GetDrawData();
        ImGui_ImplVulkan_RenderDrawData(drawData, currentFrameData.ImGuiCommandBuffer);

        auto &io = ImGui::GetIO();
        // Update and Render additional Platform Windows
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
        }

        // End ImGui renderpass
        m_Renderer->EndRenderPass(currentFrameData.ImGuiCommandBuffer);

        VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};

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
        VulkanRenderer::Get()->FinishAllFrames();
        switch (type) {
            case DebugScenes::Physics: {
                MakePhysicsScene();
                break;
            }
            case DebugScenes::PhysicsPlayground:{
                MakePhysicsPlayGround();
                break;
            }
            case DebugScenes::RenderingDemo: {
                MakeRenderingDemoScene();
                break;
            }
            case DebugScenes::Audio: {
                MakeAudioTestScene();
                break;
            }
            case DebugScenes::Landscape: {
                MakeLandscapeScene();
                break;
            }
            default: {
                LOG("GameModeType is invalid\n");
                break;
            }
        }
        if (m_Scene)
            m_Scene->GetPhysicSystem()->UpdateDynamicWorld();
    }

    void Application::SetRenderCamera(CameraComponent &cam) {
        m_RenderCamera = &cam;
    }

    CameraComponent *Application::GetRenderCamera() {
        return m_RenderCamera;
    }

    void Application::MakePhysicsScene() {
        m_Scene = std::make_unique<Scene>();

        // TODO: make physics scene.

        //terrain
        {
            auto entity = m_Scene->CreateEntity("Terrain");
            auto &mesh = m_Scene->AddComponent<LandscapeComponent>(entity, "Assets/TerrainTextures/Terrain_Tough/heightMap.png", "Assets/TerrainTextures/Terrain_Tough/texture.png");

            auto &transform = m_Scene->GetComponent<TransformComponent>(entity);
            //transform.Position = glm::vec3(mesh.landscape->width/2.f,-100,mesh.landscape->height/2.f);
            //transform.Rotation = glm::vec3(0.f,glm::pi<float>(),0.f);

            auto* state = new btDefaultMotionState();
            btRigidBody::btRigidBodyConstructionInfo info(0,state,mesh.HeightFieldShape->mHeightfieldShape);
            auto* body = new btRigidBody(info);
            body->setFriction(1.f);
            m_Scene->GetPhysicSystem()->AddRigidBody(body);
        }
        // make monstertruck
        {
            auto ent = m_Scene->CreateEntity("MonsterTruck");
            m_Scene->AddComponent<NativeScriptComponent>(ent, std::make_unique<MonsterTruckScript>(), m_Scene, ent);
        }
        {
            auto music = m_Scene->CreateEntity("Background Music");
            auto& sound = m_Scene->AddComponent<SoundSourceComponent>(music, "pinchcliffe.wav");
            sound.Looping(true);
            sound.Volume(0.1f);
           // sound.Play();
        }
    }




    void Application::MakeRenderingDemoScene() {
        m_Scene = std::make_unique<Scene>();

        {
            auto ent = m_Scene->CreateEntity("Cerberus");
            auto &sm = m_Scene->AddComponent<StaticMeshComponent>(ent, "Assets/Cerberus_by_Andrew_Maximov/Cerberus_LP.FBX");
            sm.meshes[0].MeshMaterial.Diffuse = Texture("Assets/Cerberus_by_Andrew_Maximov/Textures/Cerberus_A.tga");
            sm.meshes[0].MeshMaterial.Normals = Texture("Assets/Cerberus_by_Andrew_Maximov/Textures/Cerberus_N.tga");
            sm.meshes[0].MeshMaterial.Metallic = Texture("Assets/Cerberus_by_Andrew_Maximov/Textures/Cerberus_M.tga");
            sm.meshes[0].MeshMaterial.Roughness = Texture("Assets/Cerberus_by_Andrew_Maximov/Textures/Cerberus_R.tga");
            sm.meshes[0].MeshMaterial.UpdateDescriptorSet();

            m_Scene->AddComponent<NativeScriptComponent>(ent, std::make_unique<TestScript>(), m_Scene, ent);
            auto &transform = m_Scene->GetComponent<TransformComponent>(ent);
            transform.Rotation.x = -1.f;
        }

        float extent = 150.f;
        float step = 80.f;

        for (float x = -extent; x < extent; x += step) {
            for (float y = -extent; y < extent; y += step) {
                for (float z = -extent; z < extent; z += step) {
                    auto entity = m_Scene->CreateEntity("PointLight ball");
                    auto &mesh = m_Scene->AddComponent<StaticMeshComponent>(entity, "Assets/Ball.obj");
                    mesh.meshes[0].MeshMaterial.Diffuse = Texture(TextureColor::Red);
                    mesh.meshes[0].MeshMaterial.Metallic = Texture(TextureColor::White);
                    mesh.meshes[0].MeshMaterial.Roughness = Texture(TextureColor::Black);
                    mesh.meshes[0].MeshMaterial.UpdateDescriptorSet();

                    m_Scene->AddComponent<PointLightComponent>(entity);

                    auto &transform = m_Scene->GetComponent<TransformComponent>(entity);
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
            auto &collision = m_Scene->AddComponent<RigidBodyComponent>(entity, location, extents, mass,
                                                                        bt::CollisionPrimitive::Box);
            auto &mesh = m_Scene->AddComponent<StaticMeshComponent>(entity, "Assets/IdentityCube.obj");
            mesh.meshes[0].MeshMaterial.Diffuse = Texture(texture);
            mesh.meshes[0].MeshMaterial.UpdateDescriptorSet();

            auto &transform = m_Scene->GetComponent<TransformComponent>(entity);
            transform.Position = glm::vec3(collision.Transform.getOrigin().getX(),
                                           collision.Transform.getOrigin().getY(),
                                           collision.Transform.getOrigin().getZ());
            transform.Scale = extents;

        }
        {
            auto entity = m_Scene->CreateEntity("Ground Ball");
            auto &mesh = m_Scene->AddComponent<StaticMeshComponent>(entity, "Assets/Ball.obj");
            mesh.meshes[0].MeshMaterial.Diffuse = Texture("Assets/LightBlue.png");
            mesh.meshes[0].MeshMaterial.UpdateDescriptorSet();

            auto &collision = m_Scene->AddComponent<RigidBodyComponent>(entity, glm::vec3(0.f, -150.f, 0.f),
                                                                        glm::vec3(75.f), 0.f,
                                                                        bt::CollisionPrimitive::Sphere);

            auto &transform = m_Scene->GetComponent<TransformComponent>(entity);
            transform.Position = glm::vec3(0.f, -150.f, 0.f);
            transform.Scale = glm::vec3(75.f);
            auto& sound = m_Scene->AddComponent<SoundSourceComponent>(entity, "TestSound_Stereo.wav");


        }


        {
            //spawning balls
            glm::vec3 location = glm::vec3(3.f, 25.f, 2.f);
            const float radius = 2.f;
            const glm::vec3 extents = glm::vec3(radius);
            const float mass = radius * 100.f;

            auto Ball = m_Scene->CreateEntity("Simulated Ball " + std::to_string(location.x + location.y + location.z));
            auto &mesh = m_Scene->AddComponent<StaticMeshComponent>(Ball, "Assets/Ball.obj");
            mesh.meshes[0].MeshMaterial.Diffuse = Texture("Assets/BallTexture.png");
            mesh.meshes[0].MeshMaterial.UpdateDescriptorSet();

            m_Scene->AddComponent<RigidBodyComponent>(Ball, location, glm::vec3(radius), mass,
                                                      bt::CollisionPrimitive::Sphere);

            auto &transform = m_Scene->GetComponent<TransformComponent>(Ball);
            transform.Position = location;
            transform.Scale = extents;

            auto &sound = m_Scene->AddComponent<SoundSourceComponent>(Ball, "TestSound_Mono.wav");
            sound.Play();
        }


    }

    void Application::MakeLandscapeScene() {
        m_Scene = std::make_unique<Scene>();
        {
            std::string name = "Heightmap";

            auto entity = m_Scene->CreateEntity(name);
            auto &mesh = m_Scene->AddComponent<LandscapeComponent>(entity,"Assets/TerrainTextures/Terrain_Tough/heightMap.png", "Assets/TerrainTextures/Terrain_Tough/texture.png");

        }
    }

    void Application::MakePhysicsPlayGround() {
        m_Scene = std::make_unique<Scene>();

        //make flooring
        {
            auto location = glm::vec3(0.f, -50.f, 0.f);
            auto extents = glm::vec3(1000.f, 5.f, 1000.f);
            auto mass = 0.f;

            auto entity = m_Scene->CreateEntity("flooring");
            auto &collision = m_Scene->AddComponent<RigidBodyComponent>(entity, location, extents, mass, bt::CollisionPrimitive::Box);
            auto &mesh = m_Scene->AddComponent<StaticMeshComponent>(entity, "Assets/Primitives/IdentityCube.fbx");
            mesh.meshes[0].MeshMaterial.Diffuse = Texture("Assets/crisscross-foam1-ue/crisscross-foam_albedo.png");
            mesh.meshes[0].MeshMaterial.AO = Texture("Assets/crisscross-foam1-ue/crisscross-foam_ao.png");
            mesh.meshes[0].MeshMaterial.Metallic = Texture("Assets/crisscross-foam1-ue/crisscross-foam_metallic.png");
            mesh.meshes[0].MeshMaterial.Normals = Texture("Assets/crisscross-foam1-ue/crisscross-foam_normal-dx.png");
            mesh.meshes[0].MeshMaterial.Roughness = Texture("Assets/crisscross-foam1-ue/crisscross-foam_roughness.png");
            mesh.meshes[0].MeshMaterial.UpdateDescriptorSet();

            auto &transform = m_Scene->GetComponent<TransformComponent>(entity);
            transform.Position = glm::vec3(collision.Transform.getOrigin().getX(),
                                           collision.Transform.getOrigin().getY(),
                                           collision.Transform.getOrigin().getZ());
            transform.Scale = extents;
            collision.RigidBody->setFriction(1.0f);

            //place random ramps
            {
                auto mass = 0.f;

                //blanket textures;
                const char *albedos[4]{
                        "Assets/soft-blanket-ue/soft-blanket_Blue_albedo.png",
                        "Assets/soft-blanket-ue/soft-blanket_Pink_albedo.png",
                        "Assets/soft-blanket-ue/soft-blanket_Red_albedo.png",
                        "Assets/soft-blanket-ue/soft-blanket_Yellow_albedo.png",
                };

                for (int i{0}; i < 10.f; i++) {
                    auto extents = glm::vec3(Math::RandFloat(5.f, 30.f), Math::RandFloat(2.f, 10.f), Math::RandFloat(5.f, 30.f));
                    auto location = glm::vec3(Math::RandFloat(-200.f, 200.f), -43.f + extents.y, Math::RandFloat(-200.f, 200.f));
                    auto rotation = glm::vec3(0.f, Math::RandFloat(0.f, 6.28f), 0.f);
                    std::string name = "Random ramp ";
                    name += std::to_string(i);

                    auto entity = m_Scene->CreateEntity(name);
                    auto &collision = m_Scene->AddComponent<RigidBodyComponent>(entity, location, extents, rotation, mass, "Assets/Primitives/IdentityRamp.fbx");
                    auto &mesh = m_Scene->AddComponent<StaticMeshComponent>(entity, "Assets/Primitives/IdentityRamp.fbx");
                    auto texture = Math::RandInt(0, 3);
                    mesh.meshes[0].MeshMaterial.Diffuse = Texture(albedos[texture]);
                    mesh.meshes[0].MeshMaterial.AO = Texture("Assets/soft-blanket-ue/soft-blanket_ao.png");
                    mesh.meshes[0].MeshMaterial.Metallic = Texture("Assets/soft-blanket-ue/soft-blanket_metallic.png");
                    mesh.meshes[0].MeshMaterial.Normals = Texture("Assets/soft-blanket-ue/soft-blanket_normal-dx.png");
                    mesh.meshes[0].MeshMaterial.Roughness = Texture("Assets/soft-blanket-ue/soft-blanket_roughness.png");

                    mesh.meshes[0].MeshMaterial.UpdateDescriptorSet();

                    auto &transform = m_Scene->GetComponent<TransformComponent>(entity);
                    transform.Position = glm::vec3(collision.Transform.getOrigin().getX(), collision.Transform.getOrigin().getY(), collision.Transform.getOrigin().getZ());
                    transform.Scale = extents;
                    collision.RigidBody->setFriction(0.9f);
                }
            }

        }
        // make monstertruck
        {
            auto ent = m_Scene->CreateEntity("MonsterTruck");
            m_Scene->AddComponent<NativeScriptComponent>(ent, std::make_unique<MonsterTruckScript>(), m_Scene, ent);
        }
    }
}
