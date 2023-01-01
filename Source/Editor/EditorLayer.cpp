#include "EditorLayer.h"

#include "../Application.h"
#include <filesystem>
#include "EditorPanels/ApplicationPanel.h"
#include "EditorPanels/SceneGraphPanel.h"
#include "EditorPanels/ComponentsPanel.h"
#include "EditorPanels/PhysicsPanel.h"
#include "EditorPanels/RendererPanel.h"
#include "EditorPanels/SoundSettingsPanel.h"
#include "EditorPanels/SoundClipPanel.h"
#include "EditorPanels/SoundSourcesPanel.h"
#include "../NativeScripts/LightSwarm.h"
#include "../NativeScripts/GameModeScript.h"
#include "imgui_internal.h"
#include "EditorPanels/NetworkPanel.h"
#include "../SoundComponent.h"
#include "../NativeScripts/CheckPointScript.h"
#include "../NativeScripts/RaceTrackScript.h"
#include "../NativeScripts/CarScripts/MonsterTruckScript.h"
#include "../NativeScripts/CarScripts/RaceCarScript.h"

namespace FLOOF {
    EditorLayer::EditorLayer() {
        m_EditorRenderer = std::make_unique<SceneRenderer>();
        m_PlayRenderer = std::make_unique<SceneRenderer>();

        m_EditorPanels.try_emplace("ApplicationPanel", std::make_unique<ApplicationPanel>(this));
        m_EditorPanels.try_emplace("SceneGraphPanel", std::make_unique<SceneGraphPanel>(this));
        m_EditorPanels.try_emplace("ComponentsPanel", std::make_unique<ComponentsPanel>(this));
        m_EditorPanels.try_emplace("PhysicsPanel", std::make_unique<PhysicsPanel>(this));
        m_EditorPanels.try_emplace("RendererPanel", std::make_unique<RendererPanel>(this));
        m_EditorPanels.try_emplace("SoundSettingsPanel", std::make_unique<SoundSettingsPanel>(this));
        m_EditorPanels.try_emplace("SoundSourcesPanel", std::make_unique<SoundSourcesPanel>(this));
        m_EditorPanels.try_emplace("SoundClipPanel", std::make_unique<SoundClipPanel>(this));
        m_EditorPanels.try_emplace("NetworkPanel", std::make_unique<NetworkPanel>(this));

        SelectDebugScene(DebugScenes::PhysicsPlayground);
    }

    EditorLayer::~EditorLayer() {
    }

    void EditorLayer::OnUpdate(double deltaTime) {
        auto &Server = Application::Get().server;
        if (Server) {
            Server->Update(); // todo should set a max nmb
        }
        auto &Client = Application::Get().client;
        if (Client && Client->IsConnected()) {
            Client->Update(m_Scene.get());
        }

        m_Scene->GetPhysicSystem()->OnEditorUpdate(deltaTime);
        if (m_EditorViewFocused) {
            UpdateEditorCamera(deltaTime);
        }

        if (IsPlaying())
            m_Scene->OnUpdate(deltaTime);
        else {
            m_Scene->OnEditorUpdate(deltaTime);
        }
    }

    void EditorLayer::OnImGuiUpdate(double deltaTime) {
        // ImGui viewports
        static bool dockSpaceOpen = true;
        static bool showDemoWindow = false;

        ImGuiViewport *viewport = ImGui::GetMainViewport();
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

        // TODO: Only allow docking to editor layer with id?
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

        // Ends Dockspace window
        ImGui::End();

        if (showDemoWindow)
            ImGui::ShowDemoWindow(&showDemoWindow);

        for (auto &[key, editorPanel]: m_EditorPanels) {
            editorPanel->DrawPanel();
        }

        //set last entity after ui stuff is done
        m_Scene->m_LastSelectedEntity = m_Scene->m_SelectedEntity;
    }

    VkSemaphore EditorLayer::OnDraw(double deltaTime, VkSemaphore waitSemaphore) {
        ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoCollapse;

        if (IsPlaying()) {
            ImGui::PushStyleVar(ImGuiStyleVar_WindowMinSize, ImVec2(300.f, 300.f));
            ImGui::Begin("Play view", nullptr, windowFlags);
            ImGui::PopStyleVar();
            auto viewport = ImGui::GetWindowViewport();
            viewport->Flags &= (~ImGuiViewportFlags_NoDecoration);

            m_PlayViewFocused = ImGui::IsWindowFocused();
            m_Scene->m_IsActiveScene = m_PlayViewFocused;

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

            SceneRenderFinishedData sceneRenderData{};
            if (!ImGui::GetCurrentWindow()->Hidden) {
                sceneRenderData = m_PlayRenderer->RenderToTexture(m_Scene.get(), sceneCanvasExtent, m_Scene->GetActiveCamera(),
                                                                  m_PlayDrawMode, m_Scene->GetPhysicsDebugDrawer(), waitSemaphore);
            }
            if (sceneRenderData.Texture != VK_NULL_HANDLE) {
                ImDrawList *draw_list = ImGui::GetWindowDrawList();
                draw_list->AddImage(sceneRenderData.Texture, canvas_p0, canvas_p1, ImVec2(0, 0), ImVec2(1, 1));
            }
            if (sceneRenderData.SceneRenderFinishedSemaphore != VK_NULL_HANDLE)
                waitSemaphore = sceneRenderData.SceneRenderFinishedSemaphore;

            ImGui::End();
        }

        {
            ImGui::PushStyleVar(ImGuiStyleVar_WindowMinSize, ImVec2(300.f, 300.f));
            ImGui::Begin("Editor view", nullptr, windowFlags);
            ImGui::PopStyleVar();
            auto viewport = ImGui::GetWindowViewport();
            viewport->Flags &= (~ImGuiViewportFlags_NoDecoration);

            m_EditorViewFocused = ImGui::IsWindowFocused();

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

            SceneRenderFinishedData sceneRenderData{};
            if (!ImGui::GetCurrentWindow()->Hidden) {
                sceneRenderData = m_EditorRenderer->RenderToTexture(m_Scene.get(), sceneCanvasExtent, m_Scene->GetEditorCamera(),
                                                                    m_EditorDrawMode, m_Scene->GetPhysicsDebugDrawer(), waitSemaphore, true);
            }

            if (sceneRenderData.Texture != VK_NULL_HANDLE) {
                ImDrawList *draw_list = ImGui::GetWindowDrawList();
                draw_list->AddImage(sceneRenderData.Texture, canvas_p0, canvas_p1, ImVec2(0, 0), ImVec2(1, 1));
            }
            if (sceneRenderData.SceneRenderFinishedSemaphore != VK_NULL_HANDLE)
                waitSemaphore = sceneRenderData.SceneRenderFinishedSemaphore;

            ImGui::End();
        }

        return waitSemaphore;
    }

    void EditorLayer::StartPlay() {
        m_PlayModeActive = true;
    }

    void EditorLayer::StopPlay() {
        m_PlayModeActive = false;
    }

    void EditorLayer::UpdateEditorCamera(double deltaTime) {
        auto *camera = m_Scene->GetEditorCamera();
        auto moveAmount = static_cast<float>(m_CameraSpeed * deltaTime);
        if (ImGui::IsKeyDown(ImGuiKey_LeftShift)) {
            moveAmount *= 8;
        }
        if (ImGui::IsKeyDown(ImGuiKey_W)) {
            camera->MoveForward(moveAmount);
        }
        if (ImGui::IsKeyDown(ImGuiKey_S)) {
            camera->MoveForward(-moveAmount);
        }
        if (ImGui::IsKeyDown(ImGuiKey_D)) {
            camera->MoveRight(moveAmount);
        }
        if (ImGui::IsKeyDown(ImGuiKey_A)) {
            camera->MoveRight(-moveAmount);
        }
        if (ImGui::IsKeyDown(ImGuiKey_Q)) {
            camera->MoveUp(-moveAmount);
        }
        if (ImGui::IsKeyDown(ImGuiKey_E)) {
            camera->MoveUp(moveAmount);
        }
        static glm::vec2 oldMousePos = glm::vec2(0.f);
        auto imguiMousePos = ImGui::GetMousePos();
        glm::vec2 mousePos(imguiMousePos.x, imguiMousePos.y);
        glm::vec2 mouseDelta = mousePos - oldMousePos;
        oldMousePos = mousePos;
        static constexpr float mouseSpeed = 0.002f;
        if (ImGui::IsMouseDown(ImGuiMouseButton_Right)
            || ImGui::IsKeyDown(ImGuiKey_LeftCtrl)) {
            camera->Yaw(mouseDelta.x * mouseSpeed);
            camera->Pitch(mouseDelta.y * mouseSpeed);
        }
    }

    void EditorLayer::SelectDebugScene(DebugScenes type) {
        m_CurrentDebugScene = type;
        VulkanRenderer::Get()->FinishAllFrames();
        switch (type) {
            case DebugScenes::Physics: {
                MakePhysicsScene();
                break;
            }
            case DebugScenes::PhysicsPlayground: {
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
            case DebugScenes::Sponza: {
                MakeSponzaScene();
                break;
            }
            case DebugScenes::SponzaRacing: {
                MakeSponzaRacing();
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

    void EditorLayer::MakePhysicsScene() {
        m_Scene = std::make_unique<Scene>();

        //terrain
        {
            auto entity = m_Scene->CreateEntity("Terrain");
            auto &mesh = m_Scene->AddComponent<LandscapeComponent>(entity, "Assets/Terrain/Terrain_Tough/heightMap.png", "Assets/Terrain/Terrain_Tough/texture.png");

            auto &transform = m_Scene->GetComponent<TransformComponent>(entity);
            //transform.Position = glm::vec3(mesh.landscape->width/2.f,-100,mesh.landscape->height/2.f);
            //transform.Rotation = glm::vec3(0.f,glm::pi<float>(),0.f);

            auto *state = new btDefaultMotionState();
            btRigidBody::btRigidBodyConstructionInfo info(0, state, mesh.HeightFieldShape->mHeightfieldShape);
            auto *body = new btRigidBody(info);
            body->setFriction(1.f);
            m_Scene->GetPhysicSystem()->AddRigidBody(body);
        }
        {
            auto music = m_Scene->CreateEntity("Background Music");
            auto& sound = m_Scene->AddComponent<SoundComponent>(music);
            sound.AddClip("pinchcliffe.wav");
            sound.GetClip("pinchcliffe.wav")->Looping(true);
            sound.GetClip("pinchcliffe.wav")->Volume(0.1f);
            sound.GetClip("pinchcliffe.wav")->Play();
        }
    }

    void EditorLayer::MakeRenderingDemoScene() {
        m_Scene = std::make_unique<Scene>();

        {
            auto location = glm::vec3(0.f, -50.f, 0.f);
            auto extents = glm::vec3(1000.f, 5.f, 1000.f);
            auto mass = 0.f;

            auto entity = m_Scene->CreateEntity("flooring");
            auto &collision = m_Scene->AddComponent<RigidBodyComponent>(entity, location, extents, glm::vec3(0.f), mass, bt::CollisionPrimitive::Box);
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
        }

        {
            auto ent = m_Scene->CreateEntity("Cerberus");
            auto &sm = m_Scene->AddComponent<StaticMeshComponent>(ent, "Assets/Cerberus_by_Andrew_Maximov/Cerberus_LP.FBX", false);
            sm.meshes[0].MeshMaterial.Diffuse = Texture("Assets/Cerberus_by_Andrew_Maximov/Textures/Cerberus_A.tga");
            sm.meshes[0].MeshMaterial.Normals = Texture("Assets/Cerberus_by_Andrew_Maximov/Textures/Cerberus_N.tga");
            sm.meshes[0].MeshMaterial.Metallic = Texture("Assets/Cerberus_by_Andrew_Maximov/Textures/Cerberus_M.tga");
            sm.meshes[0].MeshMaterial.Roughness = Texture("Assets/Cerberus_by_Andrew_Maximov/Textures/Cerberus_R.tga");
            sm.meshes[0].MeshMaterial.UpdateDescriptorSet();

            auto &transform = m_Scene->GetComponent<TransformComponent>(ent);
            transform.Rotation.x = -1.f;
            transform.Rotation.y += glm::pi<float>() / 2.f;
            transform.Position.y += 55.f;
            transform.Position.z += 40.f;
        }

        for (uint32_t i = 1; i < static_cast<uint32_t>(TextureColor::Size); i++) {
            auto color = static_cast<TextureColor>(i);
            static constexpr uint32_t yOffsetAmount = 3;
            static constexpr float xOffsetAmount = 3.f;
            float xOffset = 10.f;
            {
                const auto entity = m_Scene->CreateEntity();
                auto &mesh = m_Scene->AddComponent<StaticMeshComponent>(entity, "Assets/Ball.obj");
                auto &transform = m_Scene->GetComponent<TransformComponent>(entity);
                xOffset += xOffsetAmount;
                transform.Position.y = i * yOffsetAmount;
                transform.Position.x = xOffset;
                mesh.meshes[0].MeshMaterial.Diffuse = Texture(color);
                mesh.meshes[0].MeshMaterial.Metallic = Texture(TextureColor::White);
                mesh.meshes[0].MeshMaterial.Roughness = Texture(TextureColor::Black);
                mesh.meshes[0].MeshMaterial.UpdateDescriptorSet();
            }
            {
                const auto entity = m_Scene->CreateEntity();
                auto &mesh = m_Scene->AddComponent<StaticMeshComponent>(entity, "Assets/Ball.obj");
                auto &transform = m_Scene->GetComponent<TransformComponent>(entity);
                xOffset += xOffsetAmount;
                transform.Position.y = i * yOffsetAmount;
                transform.Position.x = xOffset;
                mesh.meshes[0].MeshMaterial.Diffuse = Texture(color);
                mesh.meshes[0].MeshMaterial.Metallic = Texture(TextureColor::White);
                mesh.meshes[0].MeshMaterial.Roughness = Texture(TextureColor::DarkGrey);
                mesh.meshes[0].MeshMaterial.UpdateDescriptorSet();
            }
            {
                const auto entity = m_Scene->CreateEntity();
                auto &mesh = m_Scene->AddComponent<StaticMeshComponent>(entity, "Assets/Ball.obj");
                auto &transform = m_Scene->GetComponent<TransformComponent>(entity);
                xOffset += xOffsetAmount;
                transform.Position.y = i * yOffsetAmount;
                transform.Position.x = xOffset;
                mesh.meshes[0].MeshMaterial.Diffuse = Texture(color);
                mesh.meshes[0].MeshMaterial.Metallic = Texture(TextureColor::White);
                mesh.meshes[0].MeshMaterial.Roughness = Texture(TextureColor::LightGrey);
                mesh.meshes[0].MeshMaterial.UpdateDescriptorSet();
            }
            {
                const auto entity = m_Scene->CreateEntity();
                auto &mesh = m_Scene->AddComponent<StaticMeshComponent>(entity, "Assets/Ball.obj");
                auto &transform = m_Scene->GetComponent<TransformComponent>(entity);
                xOffset += xOffsetAmount;
                transform.Position.y = i * yOffsetAmount;
                transform.Position.x = xOffset;
                mesh.meshes[0].MeshMaterial.Diffuse = Texture(color);
                mesh.meshes[0].MeshMaterial.Metallic = Texture(TextureColor::White);
                mesh.meshes[0].MeshMaterial.Roughness = Texture(TextureColor::White);
                mesh.meshes[0].MeshMaterial.UpdateDescriptorSet();
            }
            {
                const auto entity = m_Scene->CreateEntity();
                auto &mesh = m_Scene->AddComponent<StaticMeshComponent>(entity, "Assets/Ball.obj");
                auto &transform = m_Scene->GetComponent<TransformComponent>(entity);
                xOffset += xOffsetAmount;
                transform.Position.y = i * yOffsetAmount;
                transform.Position.x = xOffset;
                mesh.meshes[0].MeshMaterial.Diffuse = Texture(color);
                mesh.meshes[0].MeshMaterial.Metallic = Texture(TextureColor::Black);
                mesh.meshes[0].MeshMaterial.Roughness = Texture(TextureColor::Black);
                mesh.meshes[0].MeshMaterial.UpdateDescriptorSet();
            }
            {
                const auto entity = m_Scene->CreateEntity();
                auto &mesh = m_Scene->AddComponent<StaticMeshComponent>(entity, "Assets/Ball.obj");
                auto &transform = m_Scene->GetComponent<TransformComponent>(entity);
                xOffset += xOffsetAmount;
                transform.Position.y = i * yOffsetAmount;
                transform.Position.x = xOffset;
                mesh.meshes[0].MeshMaterial.Diffuse = Texture(color);
                mesh.meshes[0].MeshMaterial.Metallic = Texture(TextureColor::Black);
                mesh.meshes[0].MeshMaterial.Roughness = Texture(TextureColor::DarkGrey);
                mesh.meshes[0].MeshMaterial.UpdateDescriptorSet();
            }
            {
                const auto entity = m_Scene->CreateEntity();
                auto &mesh = m_Scene->AddComponent<StaticMeshComponent>(entity, "Assets/Ball.obj");
                auto &transform = m_Scene->GetComponent<TransformComponent>(entity);
                xOffset += xOffsetAmount;
                transform.Position.y = i * yOffsetAmount;
                transform.Position.x = xOffset;
                mesh.meshes[0].MeshMaterial.Diffuse = Texture(color);
                mesh.meshes[0].MeshMaterial.Metallic = Texture(TextureColor::Black);
                mesh.meshes[0].MeshMaterial.Roughness = Texture(TextureColor::LightGrey);
                mesh.meshes[0].MeshMaterial.UpdateDescriptorSet();
            }
            {
                const auto entity = m_Scene->CreateEntity();
                auto &mesh = m_Scene->AddComponent<StaticMeshComponent>(entity, "Assets/Ball.obj");
                auto &transform = m_Scene->GetComponent<TransformComponent>(entity);
                xOffset += xOffsetAmount;
                transform.Position.y = i * yOffsetAmount;
                transform.Position.x = xOffset;
                mesh.meshes[0].MeshMaterial.Diffuse = Texture(color);
                mesh.meshes[0].MeshMaterial.Metallic = Texture(TextureColor::Black);
                mesh.meshes[0].MeshMaterial.Roughness = Texture(TextureColor::White);
                mesh.meshes[0].MeshMaterial.UpdateDescriptorSet();
            }
        }
        for (uint32_t i = 1; i < static_cast<uint32_t>(TextureColor::Size); i++) {
            auto color = static_cast<TextureColor>(i);
            static constexpr uint32_t yOffsetAmount = 3;
            static constexpr float xOffsetAmount = 3.f;
            float xOffset = 100.f;
            auto opacityAmount = TextureColor::LightGrey;
            {
                const auto entity = m_Scene->CreateEntity();
                auto& mesh = m_Scene->AddComponent<StaticMeshComponent>(entity, "Assets/Ball.obj");
                auto& transform = m_Scene->GetComponent<TransformComponent>(entity);
                xOffset += xOffsetAmount;
                transform.Position.y = i * yOffsetAmount;
                transform.Position.x = xOffset;
                mesh.meshes[0].MeshMaterial.Diffuse = Texture(color);
                mesh.meshes[0].MeshMaterial.Metallic = Texture(TextureColor::White);
                mesh.meshes[0].MeshMaterial.Roughness = Texture(TextureColor::Black);
                mesh.meshes[0].MeshMaterial.Opacity = Texture(opacityAmount);
                mesh.meshes[0].MeshMaterial.HasOpacity = true;
                mesh.meshes[0].MeshMaterial.UpdateDescriptorSet();
            }
            {
                const auto entity = m_Scene->CreateEntity();
                auto& mesh = m_Scene->AddComponent<StaticMeshComponent>(entity, "Assets/Ball.obj");
                auto& transform = m_Scene->GetComponent<TransformComponent>(entity);
                xOffset += xOffsetAmount;
                transform.Position.y = i * yOffsetAmount;
                transform.Position.x = xOffset;
                mesh.meshes[0].MeshMaterial.Diffuse = Texture(color);
                mesh.meshes[0].MeshMaterial.Metallic = Texture(TextureColor::White);
                mesh.meshes[0].MeshMaterial.Roughness = Texture(TextureColor::DarkGrey);
                mesh.meshes[0].MeshMaterial.Opacity = Texture(opacityAmount);
                mesh.meshes[0].MeshMaterial.HasOpacity = true;
                mesh.meshes[0].MeshMaterial.UpdateDescriptorSet();
            }
            {
                const auto entity = m_Scene->CreateEntity();
                auto& mesh = m_Scene->AddComponent<StaticMeshComponent>(entity, "Assets/Ball.obj");
                auto& transform = m_Scene->GetComponent<TransformComponent>(entity);
                xOffset += xOffsetAmount;
                transform.Position.y = i * yOffsetAmount;
                transform.Position.x = xOffset;
                mesh.meshes[0].MeshMaterial.Diffuse = Texture(color);
                mesh.meshes[0].MeshMaterial.Metallic = Texture(TextureColor::White);
                mesh.meshes[0].MeshMaterial.Roughness = Texture(TextureColor::LightGrey);
                mesh.meshes[0].MeshMaterial.Opacity = Texture(opacityAmount);
                mesh.meshes[0].MeshMaterial.HasOpacity = true;
                mesh.meshes[0].MeshMaterial.UpdateDescriptorSet();
            }
            {
                const auto entity = m_Scene->CreateEntity();
                auto& mesh = m_Scene->AddComponent<StaticMeshComponent>(entity, "Assets/Ball.obj");
                auto& transform = m_Scene->GetComponent<TransformComponent>(entity);
                xOffset += xOffsetAmount;
                transform.Position.y = i * yOffsetAmount;
                transform.Position.x = xOffset;
                mesh.meshes[0].MeshMaterial.Diffuse = Texture(color);
                mesh.meshes[0].MeshMaterial.Metallic = Texture(TextureColor::White);
                mesh.meshes[0].MeshMaterial.Roughness = Texture(TextureColor::White);
                mesh.meshes[0].MeshMaterial.Opacity = Texture(opacityAmount);
                mesh.meshes[0].MeshMaterial.HasOpacity = true;
                mesh.meshes[0].MeshMaterial.UpdateDescriptorSet();
            }
            {
                const auto entity = m_Scene->CreateEntity();
                auto& mesh = m_Scene->AddComponent<StaticMeshComponent>(entity, "Assets/Ball.obj");
                auto& transform = m_Scene->GetComponent<TransformComponent>(entity);
                xOffset += xOffsetAmount;
                transform.Position.y = i * yOffsetAmount;
                transform.Position.x = xOffset;
                mesh.meshes[0].MeshMaterial.Diffuse = Texture(color);
                mesh.meshes[0].MeshMaterial.Metallic = Texture(TextureColor::Black);
                mesh.meshes[0].MeshMaterial.Roughness = Texture(TextureColor::Black);
                mesh.meshes[0].MeshMaterial.Opacity = Texture(opacityAmount);
                mesh.meshes[0].MeshMaterial.HasOpacity = true;
                mesh.meshes[0].MeshMaterial.UpdateDescriptorSet();
            }
            {
                const auto entity = m_Scene->CreateEntity();
                auto& mesh = m_Scene->AddComponent<StaticMeshComponent>(entity, "Assets/Ball.obj");
                auto& transform = m_Scene->GetComponent<TransformComponent>(entity);
                xOffset += xOffsetAmount;
                transform.Position.y = i * yOffsetAmount;
                transform.Position.x = xOffset;
                mesh.meshes[0].MeshMaterial.Diffuse = Texture(color);
                mesh.meshes[0].MeshMaterial.Metallic = Texture(TextureColor::Black);
                mesh.meshes[0].MeshMaterial.Roughness = Texture(TextureColor::DarkGrey);
                mesh.meshes[0].MeshMaterial.Opacity = Texture(opacityAmount);
                mesh.meshes[0].MeshMaterial.HasOpacity = true;
                mesh.meshes[0].MeshMaterial.UpdateDescriptorSet();
            }
            {
                const auto entity = m_Scene->CreateEntity();
                auto& mesh = m_Scene->AddComponent<StaticMeshComponent>(entity, "Assets/Ball.obj");
                auto& transform = m_Scene->GetComponent<TransformComponent>(entity);
                xOffset += xOffsetAmount;
                transform.Position.y = i * yOffsetAmount;
                transform.Position.x = xOffset;
                mesh.meshes[0].MeshMaterial.Diffuse = Texture(color);
                mesh.meshes[0].MeshMaterial.Metallic = Texture(TextureColor::Black);
                mesh.meshes[0].MeshMaterial.Roughness = Texture(TextureColor::LightGrey);
                mesh.meshes[0].MeshMaterial.Opacity = Texture(opacityAmount);
                mesh.meshes[0].MeshMaterial.HasOpacity = true;
                mesh.meshes[0].MeshMaterial.UpdateDescriptorSet();
            }
            {
                const auto entity = m_Scene->CreateEntity();
                auto& mesh = m_Scene->AddComponent<StaticMeshComponent>(entity, "Assets/Ball.obj");
                auto& transform = m_Scene->GetComponent<TransformComponent>(entity);
                xOffset += xOffsetAmount;
                transform.Position.y = i * yOffsetAmount;
                transform.Position.x = xOffset;
                mesh.meshes[0].MeshMaterial.Diffuse = Texture(color);
                mesh.meshes[0].MeshMaterial.Metallic = Texture(TextureColor::Black);
                mesh.meshes[0].MeshMaterial.Roughness = Texture(TextureColor::White);
                mesh.meshes[0].MeshMaterial.Opacity = Texture(opacityAmount);
                mesh.meshes[0].MeshMaterial.HasOpacity = true;
                mesh.meshes[0].MeshMaterial.UpdateDescriptorSet();
            }
        }
    }

    void EditorLayer::MakeAudioTestScene() {
        m_Scene = std::make_unique<Scene>();
        {
            auto texture = "Assets/LightBlue.png";
            auto location = glm::vec3(0.f, -150.f, 0.f);
            auto extents = glm::vec3(400.f, 10.f, 400.f);
            auto mass = 0.f;

            auto entity = m_Scene->CreateEntity("Ground Cube");
            auto &collision = m_Scene->AddComponent<RigidBodyComponent>(entity, location, extents, glm::vec3(0.f), mass,
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
                                                                        glm::vec3(75.f), glm::vec3(0.f), 0.f,
                                                                        bt::CollisionPrimitive::Sphere);

            auto &transform = m_Scene->GetComponent<TransformComponent>(entity);
            transform.Position = glm::vec3(0.f, -150.f, 0.f);
            transform.Scale = glm::vec3(75.f);
            auto& sound = m_Scene->AddComponent<SoundComponent>(entity, "TestSound_Stereo.wav");

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

            m_Scene->AddComponent<RigidBodyComponent>(Ball, location, glm::vec3(radius), glm::vec3(0.f), mass,
                                                      bt::CollisionPrimitive::Sphere);

            auto &transform = m_Scene->GetComponent<TransformComponent>(Ball);
            transform.Position = location;
            transform.Scale = extents;



            auto& sound = m_Scene->AddComponent<SoundComponent>(Ball, std::vector<std::string>{ "TestSound_Mono.wav", "TestSound_Stereo.wav" });

            //sound.GetClip("TestSound_Stereo.wav")->Play();

            //sound.AddClip("TestSound_Stereo.wav");
            //sound.GetClip("TestSound_Stereo.wav");

        }


    }

    void EditorLayer::MakeLandscapeScene() {
        m_Scene = std::make_unique<Scene>();
        {
            std::string name = "Heightmap";

            auto entity = m_Scene->CreateEntity(name);
            auto &mesh = m_Scene->AddComponent<LandscapeComponent>(entity, "Assets/Terrain/Terrain_Tough/heightMap.png", "Assets/Terrain/Terrain_Tough/texture.png");
        }
        {
            const auto lightSwarmEntity = m_Scene->CreateEntity();
            m_Scene->AddComponent<NativeScriptComponent>(lightSwarmEntity, std::make_unique<LightSwarm>(), m_Scene.get(), lightSwarmEntity);
            m_Scene->GetComponent<TransformComponent>(lightSwarmEntity).Position.y += 300.f;
        }
    }

    void EditorLayer::MakeSponzaScene() {
        m_Scene = std::make_unique<Scene>();
        {
            const auto entity = m_Scene->CreateEntity();
            m_Scene->AddComponent<StaticMeshComponent>(entity, "Assets/Sponza/sponza.obj");
        }
        {
            const auto lightSwarmEntity = m_Scene->CreateEntity();
            m_Scene->AddComponent<NativeScriptComponent>(lightSwarmEntity, std::make_unique<LightSwarm>(), m_Scene.get(), lightSwarmEntity);
            m_Scene->GetComponent<TransformComponent>(lightSwarmEntity).Position.y += 300.f;
        }
    }

    void EditorLayer::MakePhysicsPlayGround() {
        m_Scene = std::make_unique<Scene>();


        //Gamemode Script
        {
            auto ent = m_Scene->CreateEntity("GameMode");
           auto & script =  m_Scene->AddComponent<NativeScriptComponent>(ent, std::make_unique<GameModeScript>(), m_Scene.get(), ent);

           const auto entity = m_Scene->CreateEntity("RaceTrack");
           m_Scene->AddComponent<NativeScriptComponent>(entity, std::make_unique<RaceTrackScript>(),m_Scene.get(),entity);

            auto cpScript = dynamic_cast<GameModeScript *>(script.Script.get());
            if (cpScript)
                cpScript->RaceTrack = entity;
        }

        {
            const auto entity = m_Scene->CreateEntity();
            auto &transform = m_Scene->GetComponent<TransformComponent>(entity);
            transform.Position = glm::vec4(0.f, 10.f, -0.5f, 1.f);
            m_Scene->AddComponent<StaticMeshComponent>(entity, "Assets/Ball.obj");
        }

        //make race track
        {

        }

        //make flooring
        {
            auto location = glm::vec3(0.f, -50.f, 0.f);
            auto extents = glm::vec3(1000.f, 5.f, 1000.f);
            auto mass = 0.f;

            auto entity = m_Scene->CreateEntity("flooring");
            auto &collision = m_Scene->AddComponent<RigidBodyComponent>(entity, location, extents, glm::vec3(0.f), mass, bt::CollisionPrimitive::Box);
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
                    transform.Rotation = rotation;
                    collision.RigidBody->setFriction(0.9f);
                }
            }

        }
        // make monstertruck
        /*
        {
            auto ent = m_Scene->CreateEntity("Local Player");
            m_Scene->AddComponent<NativeScriptComponent>(ent, std::make_unique<RaceCarScript>(glm::vec3(0.f, -40.f, 0.f)), m_Scene.get(), ent);
            //m_Scene->AddComponent<NativeScriptComponent>(ent, std::make_unique<MonsterTruckScript>(glm::vec3(0.f, -40.f, 0.f)), m_Scene.get(), ent);
            m_Scene->AddComponent<PlayerControllerComponent>(ent, 0);
        }
         */
        {
            const auto lightSwarmEntity = m_Scene->CreateEntity();
            m_Scene->AddComponent<NativeScriptComponent>(lightSwarmEntity, std::make_unique<LightSwarm>(), m_Scene.get(), lightSwarmEntity);
            m_Scene->GetComponent<TransformComponent>(lightSwarmEntity).Position.y += 300.f;
        }
    }

    void EditorLayer::MakeSponzaRacing() {
        m_Scene = std::make_unique<Scene>();

        //Gamemode Script
        {
            auto ent = m_Scene->CreateEntity("GameMode");
            m_Scene->AddComponent<NativeScriptComponent>(ent, std::make_unique<GameModeScript>(), m_Scene.get(), ent);
        }

        //make Collision
        {
            auto Ground = m_Scene->CreateEntity("Ground Collision");
            auto &collision = m_Scene->AddComponent<RigidBodyComponent>(Ground, glm::vec3(0.f, -12.f, 0.f), glm::vec3(1900.f, 10.f, 1150.f), glm::vec3(0.f), 0.f, bt::CollisionPrimitive::Box);
            //auto &mesh = m_Scene->AddComponent<StaticMeshComponent>(entity, "Assets/IdentityCube.obj");
            //mesh.meshes[0].MeshMaterial.Diffuse = Texture("Assets/LightBlue.png");
            //mesh.meshes[0].MeshMaterial.UpdateDescriptorSet();

            //auto &transform = m_Scene->GetComponent<TransformComponent>(entity);
            //transform.Position = glm::vec3(collision.Transform.getOrigin().getX(),collision.Transform.getOrigin().getY(),collision.Transform.getOrigin().getZ());
            //transform.Scale = glm::vec3(1900.f,10.f,1150.f);
            //collision.RigidBody->setFriction(1.f);
            auto Longwall1 = m_Scene->CreateEntity("LongWall 1 Collision");
            m_Scene->AddComponent<RigidBodyComponent>(Longwall1, glm::vec3(6.f, 450.f, 588.f), glm::vec3(1900.f, 500.f, 20.f), glm::vec3(0.f), 0.f, bt::CollisionPrimitive::Box);

            auto Longwall2 = m_Scene->CreateEntity("LongWall 2 Collision");
            m_Scene->AddComponent<RigidBodyComponent>(Longwall2, glm::vec3(6.f, 450.f, -658.f), glm::vec3(1900.f, 500.f, 20.f), glm::vec3(0.f), 0.f, bt::CollisionPrimitive::Box);

            auto Shortwall1 = m_Scene->CreateEntity("Short Wall 1 Collision");
            m_Scene->AddComponent<RigidBodyComponent>(Shortwall1, glm::vec3(1318.f, 450.f, 0.f), glm::vec3(20.f, 500.f, 1000.f), glm::vec3(0.f), 0.f, bt::CollisionPrimitive::Box);

            auto Shortwall2 = m_Scene->CreateEntity("Short Wall 1 Collision");
            m_Scene->AddComponent<RigidBodyComponent>(Shortwall2, glm::vec3(-1444.f, 450.f, 0.f), glm::vec3(20.f, 500.f, 1000.f), glm::vec3(0.f), 0.f, bt::CollisionPrimitive::Box);

        }
        //Other Collision
        {
            for (int i{0}; i < 10; i++) {
                std::string name = "Collision Volume";
                name += std::to_string(i);

                auto a = glm::vec3(6.f, 450.f, 588.f); //location wall
                auto a2 = glm::vec3(6.f, 450.f, -658.f); //location wall opposit side
                auto b = glm::vec3(1900.f, 500.f, 20.f); //scale both walls

                auto c = glm::vec3(1318.f, 450.f, 0.f); // wall short side
                auto c1 = glm::vec3(-1444.f, 450.f, 0.f); // wall short side other side
                auto d = glm::vec3(20.f, 500.f, 1000.f); // Scale wall



                auto entity = m_Scene->CreateEntity(name);
                auto &collision = m_Scene->AddComponent<RigidBodyComponent>(entity, glm::vec3(0.f), glm::vec3(1.f), glm::vec3(0.f), 0.f, bt::CollisionPrimitive::Box);
                auto &mesh = m_Scene->AddComponent<StaticMeshComponent>(entity, "Assets/IdentityCube.obj");
                mesh.meshes[0].MeshMaterial.Diffuse = Texture("Assets/FloofHeader.png");
                mesh.meshes[0].MeshMaterial.UpdateDescriptorSet();

                auto &transform = m_Scene->GetComponent<TransformComponent>(entity);
                transform.Position = glm::vec3(collision.Transform.getOrigin().getX(), collision.Transform.getOrigin().getY(), collision.Transform.getOrigin().getZ());
                transform.Scale = glm::vec3(1.f);
            }
        }

        //make sponza graphics
        {

            const auto entity = m_Scene->CreateEntity();
            m_Scene->AddComponent<StaticMeshComponent>(entity, "Assets/Sponza/sponza.obj");
            const auto lightSwarmEntity = m_Scene->CreateEntity();
            m_Scene->AddComponent<NativeScriptComponent>(lightSwarmEntity, std::make_unique<LightSwarm>(), m_Scene.get(), lightSwarmEntity);
        }
    }
}