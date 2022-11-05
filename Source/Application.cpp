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
#include "Renderer/DeferredSceneRenderer.h"
#include "GameMode/PhysicsGM.h"
#include "GameMode/SponzaGM.h"
#include "GameMode/AudioTestGM.h"

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
        //io.ConfigViewportsNoTaskBarIcon = true;  // Enable Gamepad Controls

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

        auto* imguiBackend = io.BackendRendererUserData;

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

        m_Scene = std::make_unique<Scene>();
        m_SoundManager = new SoundManager;

        /*SceneRenderer*/
        SetRendererType(SceneRendererType::Forward);
        
        /*GameMode*/
        SetGameModeType(GameModeType::Physics);
    }

    void Application::CleanApplication() {
        m_Renderer->FinishAllFrames();

        m_Scene = nullptr;
        m_GameMode = nullptr;
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

    void Application::MakeTreeNode(entt::entity entity, const char* tag, Relationship& rel) {
        static ImGuiTreeNodeFlags base_flags =
            ImGuiTreeNodeFlags_OpenOnArrow |
            ImGuiTreeNodeFlags_OpenOnDoubleClick |
            ImGuiTreeNodeFlags_SpanAvailWidth;

        ImGuiTreeNodeFlags node_flags = base_flags;

        if (entity == m_Scene->m_SelectedEntity)
            node_flags |= ImGuiTreeNodeFlags_Selected;

        if (rel.Children.empty()) {
            // Parent without children are just nodes.
            // using tree node since it allows for selection
            node_flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
            ImGui::TreeNodeEx((void*)&entity, node_flags, "%s\t\tEntity id: %d", tag, static_cast<uint32_t>(entity));
            if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen()){
                m_Scene->m_SelectedEntity = entity;
                if(m_Scene->m_LastSelectedEntity != m_Scene->m_SelectedEntity && m_Scene->m_LastSelectedEntity != entt::null){
                    auto* body = m_Scene->TryGetComponent<RigidBodyComponent>(m_Scene->m_LastSelectedEntity);
                    if(body){
                        body->wakeup();
                    }
                }
                m_Scene->m_LastSelectedEntity = entity;
            }

        } else {
            // is parent to children and has to make a tree
            bool node_open = ImGui::TreeNodeEx((void*)(intptr_t)entity, node_flags, "%s\t\tEntity id: %d", tag, static_cast<uint32_t>(entity));
            if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen()){
                m_Scene->m_SelectedEntity = entity;
                if(m_Scene->m_LastSelectedEntity != m_Scene->m_SelectedEntity && m_Scene->m_LastSelectedEntity != entt::null){
                        auto* body = m_Scene->TryGetComponent<RigidBodyComponent>(m_Scene->m_LastSelectedEntity);
                        if(body){
                            body->wakeup();
                        }
                }
                m_Scene->m_LastSelectedEntity = entity;
            }

            if (node_open) {
                for (auto& childEntity : rel.Children) {
                    auto& childTag = m_Scene->GetComponent<TagComponent>(childEntity);
                    auto& childRel = m_Scene->GetComponent<Relationship>(childEntity);
                    MakeTreeNode(childEntity, childTag.Tag.c_str(), childRel);
                }
                ImGui::TreePop();
            }
        }
    }

    void Application::UpdateImGui(float deltaTime)
    {
        // ImGui viewports
        static bool dockSpaceOpen = true;
        static bool showDemoWindow = false;

        ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->Pos);
        ImGui::SetNextWindowSize(viewport->Size);
        ImGui::SetNextWindowViewport(viewport->ID);

        ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDocking;
        window_flags |= ImGuiWindowFlags_MenuBar;
        window_flags |= ImGuiWindowFlags_NoTitleBar;
        window_flags |= ImGuiWindowFlags_NoResize;
        window_flags |= ImGuiWindowFlags_NoCollapse;
        window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus;
        window_flags |= ImGuiWindowFlags_NoNavFocus;
        window_flags |= ImGuiWindowFlags_NoBackground;

        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

        // No ImGui begin commands should come before this one.
        // The dock space needs to be created before everyting else to work.
        ImGui::Begin("Dock space", &dockSpaceOpen, window_flags);

        ImGui::PopStyleVar(3);

        auto dockSpaceID = ImGui::GetID("Dock space ID");
        ImGui::DockSpace(dockSpaceID, ImVec2(0, 0), ImGuiDockNodeFlags_PassthruCentralNode);

        if (ImGui::BeginMenuBar()) {
            if (ImGui::BeginMenu("Options")) {
                if (ImGui::MenuItem("Show/Hide ImGui demo")) {
                    showDemoWindow = !showDemoWindow;
                }
                ImGui::EndMenu();
            }
            ImGui::EndMenuBar();
        }

        if (showDemoWindow)
            ImGui::ShowDemoWindow(&showDemoWindow);

        static int selectedRenderType = static_cast<int>(m_SceneRendererType);
        static int selectedGameType = static_cast<int>(m_GameModeType);
        static int selectedDrawMode = static_cast<int>(m_DrawMode);

        ImGui::Begin("Application");
        if (ImGui::Combo("SceneRendererType", 
            &selectedRenderType,
            SceneRendererTypeStrings, 
            IM_ARRAYSIZE(SceneRendererTypeStrings)))
        {
            SetRendererType(static_cast<SceneRendererType>(selectedRenderType));
        }
        if (ImGui::Combo("DrawMode", 
            &selectedDrawMode, 
            ApplicationDrawModes, 
            IM_ARRAYSIZE(ApplicationDrawModes))) 
        {
            SetDrawMode(static_cast<RenderPipelineKeys>(selectedDrawMode));
        }
        ImGui::NewLine();
        ImGui::Separator();
        ImGui::NewLine();
        if (ImGui::Combo("GameMode",
            &selectedGameType,
            GameModeTypeStrings,
            IM_ARRAYSIZE(GameModeTypeStrings)))
        {
            SetGameModeType(static_cast<GameModeType>(selectedGameType));
        }
        ImGui::End();

        // Draw scene graph and component view
        if (m_Scene) {
            //ImGui::PushStyleVar()
            ImGui::Begin("Scene");
            ImGui::BeginChild("Scene graph", ImVec2(0.f, ImGui::GetWindowHeight() / 2.f));
            auto view = m_Scene->GetRegistry().view<TransformComponent, TagComponent, Relationship>();
            for (auto [entity, transform, tag, rel] : view.each()) {
                // only care about entitys without parent.
                // deal with child entitys later.
                if (rel.Parent != entt::null)
                    continue;

                MakeTreeNode(entity, tag.Tag.c_str(), rel);
            }
            ImGui::EndChild();
            ImGui::Separator();
            // Start component view
            ImGui::BeginChild("Components");
            ImGui::Text("Components");
            if (m_Scene && m_Scene->m_SelectedEntity != entt::null) {
                if (auto* transform = m_Scene->GetRegistry().try_get<TransformComponent>(m_Scene->m_SelectedEntity)) {
                    ImGui::Separator();
                    ImGui::Text("Transform component");
                    ImGui::DragFloat3("Position", &transform->Position[0],0.1f);
                    ImGui::DragFloat3("Rotation", &transform->Rotation[0]);

                    if(auto* body = m_Scene->TryGetComponent<RigidBodyComponent>(m_Scene->m_SelectedEntity)){
                        if(body->Primitive == bt::CollisionPrimitive::Sphere){
                            ImGui::DragFloat("Scale", &transform->Scale[0]);
                            transform->Scale[1] = transform->Scale[2] = transform->Scale[0];
                        }
                        else
                            ImGui::DragFloat3("Scale", &transform->Scale[0]);
                        //move physics body
                        body->transform(transform->Position,transform->Rotation, transform->Scale/2.f);
                    }


                }
                if (auto* rigidBody = m_Scene->GetRegistry().try_get<RigidBodyComponent>(m_Scene->m_SelectedEntity)) {
                    ImGui::Separator();
                    ImGui::Text("Rigid body component");
                    if (rigidBody->RigidBody) {
                        ImGui::Text("Position: %.3f, %.3f, %.3f",
                            rigidBody->RigidBody->getCenterOfMassPosition().getX(),
                            rigidBody->RigidBody->getCenterOfMassPosition().getY(),
                            rigidBody->RigidBody->getCenterOfMassPosition().getZ());

                        ImGui::Text("Local Scale: %.3f, %.3f, %.3f",
                                    rigidBody->CollisionShape->getLocalScaling().getX(),
                                    rigidBody->CollisionShape->getLocalScaling().getY(),
                                    rigidBody->CollisionShape->getLocalScaling().getZ());

                        ImGui::Text("Velocity: %.3f, %.3f, %.3f",
                            rigidBody->RigidBody->getLinearVelocity().getX(),
                            rigidBody->RigidBody->getLinearVelocity().getY(),
                            rigidBody->RigidBody->getLinearVelocity().getZ());

                        ImGui::Text("Velocity length: %.3f", rigidBody->RigidBody->getLinearVelocity().length());
                    }
                }
                if (auto* softBody = m_Scene->GetRegistry().try_get<SoftBodyComponent>(m_Scene->m_SelectedEntity)) {
                    ImGui::Separator();
                    ImGui::Text("Soft body component");
                }
                if (auto* meshComponent = m_Scene->GetRegistry().try_get<MeshComponent>(m_Scene->m_SelectedEntity)) {
                    ImGui::Separator();
                    ImGui::Text("Mesh component");
                    ImGui::Text(meshComponent->Data.Path.c_str());
                }
                if (auto* staticMeshComponent = m_Scene->GetRegistry().try_get<StaticMeshComponent>(m_Scene->m_SelectedEntity)) {
                    ImGui::Separator();
                    ImGui::Text("Static mesh component");
                }
                if (auto* texture = m_Scene->GetRegistry().try_get<TextureComponent>(m_Scene->m_SelectedEntity)) {
                    ImGui::Separator();
                    ImGui::Text("Texture component");
                    ImGui::Text(texture->Data.Path.c_str());
                    ImGui::Image(texture->Data.DesctriptorSet, ImVec2(50, 50));
                }
                if (auto* soundComponent = m_Scene->GetRegistry().try_get<SoundComponent>(m_Scene->m_SelectedEntity)) {
                    ImGui::Separator();
                    ImGui::Text("Sound component");
                }
                if(auto* scriptComponent = m_Scene->TryGetComponent<ScriptComponent>(m_Scene->m_SelectedEntity)){
                    ImGui::Separator();
                    ImGui::Text("Script Component");
                    std::string currentscript = scriptComponent->Script;
                    currentscript.erase(0,8);

                    //todo this is bad, should not read all files every frame
                    std::vector<std::string> scripts;
                    for (const auto & entry : std::filesystem::directory_iterator("Scripts")){
                        std::string cleanname = entry.path();
                        cleanname.erase(0,8);
                        scripts.emplace_back(cleanname);

                    }

                    if(ImGui::BeginCombo("Active Script",currentscript.c_str())){
                       for (int n = 0; n < scripts.size(); n++){
                           bool is_selected = (currentscript == scripts[n]);
                           if(is_selected){
                               ImGui::SetItemDefaultFocus();
                           }
                           if (ImGui::Selectable(scripts[n].c_str(), is_selected)) {
                               currentscript = scripts[n];
                               std::string path = "Scripts/";
                               path.append(currentscript);
                               scriptComponent->Script = path;
                               scriptComponent->updateScripts();
                           }

                       }
                       ImGui::EndCombo();
                   }

                    if(ImGui::Button("Refresh Script")){
                            scriptComponent->updateScripts();
                    }
                    if(ImGui::Button("Run Script once")){
                        scriptComponent->RunScript();
                    }
                }
            }
                                                                       
            ImGui::EndChild();
            ImGui::End();

            ImGui::Begin("Scripts");
            if(ImGui::Button("Run all Scripts once")){
                auto view = m_Scene->GetRegistry().view<ScriptComponent>();
                for (auto [entity, script]: view.each()) {
                    script.RunScript();
                }
            }
            if(ImGui::Button("Refresh all Scripts")){
                auto view = m_Scene->GetRegistry().view<ScriptComponent>();
                for (auto [entity, script]: view.each()) {
                    script.updateScripts();
                }
            }
            ImGui::End();
        }
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

        m_Scene->OnUpdate(deltaTime);

        if (m_GameMode) m_GameMode->OnUpdateEditor(deltaTime);
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

        // Ends Dockspace window
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

    void Application::SetRendererType(SceneRendererType type)
    {
        if (type == m_SceneRendererType && m_SceneRenderer) return;

        m_SceneRendererType = type;

        switch (m_SceneRendererType) {
            case SceneRendererType::Forward:
            {
                m_SceneRenderer = std::make_unique<ForwardSceneRenderer>();
                break;
            }
            case SceneRendererType::Deferred:
            {
                m_SceneRenderer = std::make_unique<DeferredSceneRenderer>();
                break;
            }
            default:
            {
                LOG("SceneRendererType is invalid\n");
                break;
            }
        }
    }

    void Application::SetGameModeType(GameModeType type) {
        if (type == m_GameModeType && m_GameMode) return;

        m_GameModeType = type;

        // TODO: Game mode should probably be chosen by the scene based
        // on data from a stored scene in JSON format. 
        // Then this function should be "LoadScene(string scenePath)".
        // That way we can load scene from a file browser of some kind.
        // 
        // Eventually the scene gamemode is just a path to a python script.

        switch (m_GameModeType) {
            case GameModeType::Physics:
            {
                MakePhysicsScene();
                m_GameMode = std::make_unique<PhysicsGM>(*m_Scene.get());
                break;
            }
            case GameModeType::Sponza:
            {
                MakeSponsaScene();
                m_GameMode = std::make_unique<SponzaGM>(*m_Scene.get());
                break;
            }
            case GameModeType::Audio:
            {
                MakeAudioTestScene();
                m_GameMode = std::make_unique<AudioTestGM>(*m_Scene.get());
                break;
            }
            default:
            {
                LOG("GameModeType is invalid\n");
                break;
            }
        }

        if (m_GameMode)
            m_GameMode->OnCreate();
        // Sets up the bullet physics world based on whats in scene.
        if (m_Scene)
            m_Scene->GetPhysicSystem()->UpdateDynamicWorld();
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

    GameModeType Application::GetGameModeType() const {
        return m_GameModeType;
    }

    void Application::MakePhysicsScene() {
        m_Scene = std::make_unique<Scene>();

        // TODO: make physics scene.

        {
            auto texture = "Assets/WaterTexture.png";
            auto location = glm::vec3(0.f, -150.f, 0.f);
            auto extents = glm::vec3(400.f, 10.f, 400.f);
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
        {
            auto entity = m_Scene->CreateEntity("Ground Ball");
            m_Scene->AddComponent<MeshComponent>(entity, "Assets/Ball.obj");
            m_Scene->AddComponent<TextureComponent>(entity, "Assets/LightBlue.png");
            auto& collision = m_Scene->AddComponent<RigidBodyComponent>(entity,glm::vec3(0.f,-150.f,0.f),glm::vec3(75.f),0.f,bt::CollisionPrimitive::Sphere);

            auto & transform = m_Scene->GetComponent<TransformComponent>(entity);
            transform.Position = glm::vec3(0.f,-150.f,0.f);
            transform.Scale = glm::vec3(75.f);

        }
        {
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
        }

        {
            auto rootEntity = m_Scene->CreateEntity("RootBall1");
            m_Scene->AddComponent<MeshComponent>(rootEntity, "Assets/Ball.obj");
            m_Scene->AddComponent<TextureComponent>(rootEntity, "Assets/BallTexture.png");

            auto leafEntity1 = m_Scene->CreateEntity("LeafBall1", rootEntity);
            m_Scene->AddComponent<MeshComponent>(leafEntity1, "Assets/Ball.obj");
            m_Scene->AddComponent<TextureComponent>(leafEntity1, "Assets/BallTexture.png");

            auto leafEntity2 = m_Scene->CreateEntity("LeafBall1", rootEntity);
            m_Scene->AddComponent<MeshComponent>(leafEntity2, "Assets/Ball.obj");
            m_Scene->AddComponent<TextureComponent>(leafEntity2, "Assets/BallTexture.png");
        }
        {
            auto rootEntity = m_Scene->CreateEntity("RootBall2");
            m_Scene->AddComponent<MeshComponent>(rootEntity, "Assets/Ball.obj");
            m_Scene->AddComponent<TextureComponent>(rootEntity, "Assets/BallTexture.png");

            auto leafEntity1 = m_Scene->CreateEntity("LeafBall1", rootEntity);
            m_Scene->AddComponent<MeshComponent>(leafEntity1, "Assets/Ball.obj");
            m_Scene->AddComponent<TextureComponent>(leafEntity1, "Assets/BallTexture.png");

            auto leafEntity2 = m_Scene->CreateEntity("LeafBall1", rootEntity);
            m_Scene->AddComponent<MeshComponent>(leafEntity2, "Assets/Ball.obj");
            m_Scene->AddComponent<TextureComponent>(leafEntity2, "Assets/BallTexture.png");
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

            //m_Scene->AddComponent<SoundComponent>(Ball, m_SoundManager, "Assets/Sounds/TestSound_Stereo.wav");


        }
        {
            //m_SoundManager->loadAssets();
        }

        m_SoundManager->testSound();

    }
}