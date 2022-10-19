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

    }

    Application::~Application() {
        ImGui_ImplVulkan_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext(m_ImguiContext);

        MeshComponent::ClearMeshDataCache();
        TextureComponent::ClearTextureDataCache();

        delete m_Renderer;

        delete Utils::Logger::s_Logger;

        glfwDestroyWindow(m_Window);
        glfwTerminate();
    }

    int Application::Run() {
        DebugInit();

        {
            LasLoader mapData("Assets/jotun.las");
            auto [vData, iData] = mapData.GetIndexedColorNormalVertexData();
            auto terrainData = mapData.GetTerrainData();

            m_TerrainEntity = m_Registry.create();
            m_Registry.emplace<PointCloudComponent>(m_TerrainEntity, mapData.GetPointData());
            m_Registry.emplace<MeshComponent>(m_TerrainEntity, vData, iData);
            m_Registry.emplace<TransformComponent>(m_TerrainEntity);
            auto& terrain = m_Registry.emplace<TerrainComponent>(m_TerrainEntity, terrainData);
            terrain.MinY = mapData.GetMinY();
        }

        {
            m_CameraEntity = m_Registry.create();
            glm::vec3 cameraPos(0.3f, 0.2f, -1.3f);
            auto& camera = m_Registry.emplace<CameraComponent>(m_CameraEntity, cameraPos);
            camera.Pitch(0.5f);
        }

        MakeHeightLines();

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
            Simulate(deltaTime);

            if (m_DebugDraw) {
                DebugUpdateLineBuffer();
            }
            Draw();
        }

        m_Renderer->FinishAllFrames();
        m_Registry.clear();

        return 0;
    }

    void Application::Update(double deltaTime) {
        // World axis
        if (m_BDebugLines[DebugLine::WorldAxis]) {
            DebugDrawLine(glm::vec3(0.f), glm::vec3(10.f, 0.f, 0.f), glm::vec3(1.f, 0.f, 0.f));
            DebugDrawLine(glm::vec3(0.f), glm::vec3(0.f, 10.f, 0.f), glm::vec3(0.f, 1.f, 0.f));
            DebugDrawLine(glm::vec3(0.f), glm::vec3(0.f, 0.f, 10.f), glm::vec3(0.f, 0.f, 1.f));
        }

        // Terrain triangles
        if (m_BDebugLines[DebugLine::TerrainTriangle]) {
            TerrainComponent& triangleSurface = m_Registry.get<TerrainComponent>(m_TerrainEntity);
            glm::vec3 surfaceTriangleColor{ 1.f, 0.f, 1.f };
            for (auto& triangle : triangleSurface.Triangles) {
                DebugDrawTriangle(triangle, surfaceTriangleColor);
            }
        }

        // Closest point on triangle to ball center
        if (m_BDebugLines[DebugLine::ClosestPointToBall]) {
            auto& terrain = m_Registry.get<TerrainComponent>(m_TerrainEntity);
            auto view = m_Registry.view<BallComponent>();
            static constexpr glm::vec3 pointColor = glm::vec3(1.f);
            for (auto [entity, ball] : view.each()) {
                for (auto& triangle : terrain.Triangles) {
                    glm::vec3 start = CollisionShape::ClosestPointToPointOnTriangle(ball.CollisionSphere.pos, triangle);
                    glm::vec3 end = start + (triangle.N * 0.1f);
                    DebugDrawLine(start, end, pointColor);
                }
            }
        }

        // Terrain triangles
        if (m_BDebugLines[DebugLine::TerrainTriangle]) {
            TerrainComponent& triangleSurface = m_Registry.get<TerrainComponent>(m_TerrainEntity);
            glm::vec3 surfaceTriangleColor{ 1.f, 0.f, 1.f };
            for (auto& triangle : triangleSurface.Triangles) {
                DebugDrawTriangle(triangle, surfaceTriangleColor);
            }
        }

        {	// Update camera.
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

        {	// UI
            if (m_ShowImguiDemo)
                ImGui::ShowDemoWindow(&m_ShowImguiDemo);

            ImGui::Begin("Utils");
            if (ImGui::Button("Spawn ball")) {
                const auto& camera = m_Registry.get<CameraComponent>(m_CameraEntity);
                SpawnBall(camera.Position, 2.f, 200.f, 0.9f, "Assets/BallTexture.png");
            }
            /*if(ImGui::Button("DrawNormals")){
                DebugToggleDrawNormals();
            }*/
            ImGui::SliderFloat("Deltatime Modifer", &m_DeltaTimeModifier, 0.f, 1.f);
            ImGui::SliderFloat("Camera Speed", &m_CameraSpeed, 50, 300);
            ImGui::End();


            ImGui::Begin("Toggle DebugLines");
            (ImGui::Checkbox(("All Debug Lines"), &m_DebugDraw));
            (ImGui::Checkbox(("Velocity Vector"), &m_BDebugLines[DebugLine::Velocity]));
            (ImGui::Checkbox(("Friction Vector"), &m_BDebugLines[DebugLine::Friction]));
            (ImGui::Checkbox(("Gravitational Pull"), &m_BDebugLines[DebugLine::GravitationalPull]));
            (ImGui::Checkbox(("Collision shapes"), &m_BDebugLines[DebugLine::CollisionShape]));
            (ImGui::Checkbox(("Terrain Triangles"), &m_BDebugLines[DebugLine::TerrainTriangle]));
            (ImGui::Checkbox(("Terrain Collision"), &m_BDebugLines[DebugLine::CollisionTriangle]));
            //shhhhh maybe crash program :)
            // (ImGui::Checkbox(("Closest triangle point"), &m_BDebugLines[DebugLine::ClosestPointToBall]));
            //(ImGui::Checkbox(("Path Trace"), &m_BDebugLines[DebugLine::Path]));
            (ImGui::Checkbox(("BSpline Trace"), &m_BDebugLines[DebugLine::BSpline]));
            (ImGui::Checkbox(("Oct Tree"), &m_BDebugLines[DebugLine::OctTree]));
            (ImGui::Checkbox(("World Axis"), &m_BDebugLines[DebugLine::WorldAxis]));
            (ImGui::Checkbox(("Point Cloud"), &m_BShowPointcloud));
            ImGui::End();

            ImGui::Begin("Oppgaver");
            static int raincount = 100;
            ImGui::SliderInt("Rain Ball Count", &raincount, 100, 5000);
            if (ImGui::Button("Spawn Rain"))
                SpawnRain(raincount);
            ImGui::Text("Balls In World = %i", m_BallCount);
            ImGui::End();
        }
    }

    void Application::Simulate(double deltaTime) {

        deltaTime *= m_DeltaTimeModifier;

        auto& terrain = m_Registry.get<TerrainComponent>(m_TerrainEntity);
        AABB worldExtents{};
        worldExtents.extent = glm::vec3(static_cast<float>(terrain.Width));
        worldExtents.pos = worldExtents.extent / 2.f;
        Octree octree(worldExtents);

        {
            auto view = m_Registry.view<TransformComponent, VelocityComponent, BallComponent>();
            for (auto [entity, transform, velocity, ball] : view.each()) {
                octree.Insert(std::make_shared<CollisionObject>(&ball.CollisionSphere, transform, velocity, ball));
            }


            if (m_BDebugLines[DebugLine::OctTree]) {
                std::vector<Octree*> leafNodes;
                octree.GetActiveLeafNodes(leafNodes);

                for (auto& node : leafNodes) {
                    auto aabb = node->GetAABB();
                    DebugDrawAABB(aabb.pos, aabb.extent);
                }
            }

        }

        {	// Calculate ball

            std::vector<std::pair<CollisionObject*, CollisionObject*>> collisionPairs;
            octree.GetCollisionPairs(collisionPairs);

            for (auto& [obj1, obj2] : collisionPairs) {
                Simulate::CalculateCollision(obj1, obj2);
                Simulate::BallBallOverlap(obj1, obj2);

            }

            glm::vec3 fri(0.f);

            auto view = m_Registry.view<TransformComponent, BallComponent, VelocityComponent, TimeComponent, BSplineComponent>();
            for (auto [entity, transform, ball, velocity, time, bSpline] : view.each()) {

                CollisionObject ballObject(&ball.CollisionSphere, transform, velocity, ball);

                velocity.Force = Math::GravitationalPull * ball.Mass;

                //ball Large terrain collision//
                auto collisions = terrain.GetOverlappingTriangles(&ball.CollisionSphere);
                for (auto& tri : collisions) {
                    if (ball.CollisionSphere.Intersect(tri)) {
                        Simulate::CalculateCollision(&ballObject, *tri, time, fri);
                        if (bSpline.empty()) {
                            std::vector<glm::vec3> first;
                            for (int i{ 0 }; i <= (BSplineComponent::D + 1); i++)
                                first.emplace_back(transform.Position);
                            bSpline.Update(first);
                        }

                    }
                    //Triangle checking collision with
                    if (m_BDebugLines[DebugLine::CollisionTriangle])
                        DebugDrawTriangle(*tri, glm::vec3(255.f, 0.f, 0.f));
                }

                //https://en.wikipedia.org/wiki/Verlet_integration
                transform.Position += (velocity.Velocity * static_cast<float>(deltaTime)) + (((velocity.Force) + fri) * (static_cast<float>(deltaTime) * static_cast<float>(deltaTime) * 0.5f));
                velocity.Velocity += (((velocity.Force / ball.Mass) + fri) * static_cast<float>(deltaTime) * 0.5f);

                //set collision sphere location
                ball.CollisionSphere.pos = transform.Position;

                const float pointIntervall{ 0.5f };

                // Save ball path and draw BSpline
                if (Timer::GetTimeSince(time.LastPoint) >= pointIntervall && !bSpline.empty()) {
                    time.LastPoint = Timer::GetTime();
                    if (bSpline.Isvalid() && bSpline.size() < m_MaxBSplineLines) {
                        bSpline.AddControllPoint(transform.Position);

                        if (m_BDebugLines[DebugLine::BSpline]) {
                            std::vector<ColorVertex> vBuffer(m_MaxBSplineLines);
                            float deltaT = (bSpline.TMax - bSpline.TMin) / (float)m_MaxBSplineLines;
                            glm::vec3 color{ 0.05f, 0.1f, 0.8f };
                            //glm::vec3 color{4,  Math::RandFloat(0.f,1.f),  Math::RandFloat(0.f,1.f) };
                            float currentT = bSpline.TMin;
                            for (auto& vertex : vBuffer) {
                                vertex.Pos = bSpline.EvaluateBSpline(currentT);
                                vertex.Color = color;
                                currentT += deltaT;
                            }

                            auto& lineMesh = m_Registry.get<LineMeshComponent>(entity);
                            lineMesh.UpdateBuffer(vBuffer);
                        }
                    }
                }

                if (m_BDebugLines[DebugLine::Friction])
                    DebugDrawLine(transform.Position, transform.Position + fri, glm::vec3(0.f, 125.f, 125.f));
                if (m_BDebugLines[DebugLine::CollisionShape])
                    DebugDrawSphere(ball.CollisionSphere.pos, ball.CollisionSphere.radius);
                if (m_BDebugLines[DebugLine::Velocity])
                    DebugDrawLine(transform.Position, transform.Position + velocity.Velocity, glm::vec3(0.f, 0.f, 255.f));
                if (m_BDebugLines[DebugLine::Force])
                    DebugDrawLine(transform.Position, transform.Position + velocity.Force, glm::vec3(255.f, 0.f, 0.f));
                if (m_BDebugLines[DebugLine::GravitationalPull])
                    DebugDrawLine(transform.Position, transform.Position + Math::GravitationalPull, glm::vec3(255.f, 255.f, 255.f));

                //reset force
                velocity.Force = glm::vec3(0.f);

                //move ball when they fall and reset path
                if (transform.Position.y <= terrain.MinY * 1.2f) {
                    const int minX{ 0 };
                    const int maxX{ terrain.Height };
                    const int minZ{ 0 };
                    const int maxZ{ terrain.Width };
                    glm::vec3 loc(Math::RandDouble(minX, maxX), 20.f, Math::RandDouble(minZ, maxZ));
                    transform.Position = loc;
                    velocity.Velocity = glm::vec3(0.f);
                    bSpline.clear();
                }

            }
        }
    }

    void Application::Draw() {
        auto commandBuffer = m_Renderer->StartRecording();

        // Camera setup
        auto extent = m_Renderer->GetExtent();
        CameraComponent& camera = m_Registry.get<CameraComponent>(m_CameraEntity);
        glm::mat4 vp = camera.GetVP(glm::radians(70.f), extent.width / (float)extent.height, 0.01f, 2000.f);

        if (!m_DrawNormals) {	// Geometry pass

            { // Draw height lines
                ColorPushConstants constants;
                constants.MVP = vp;
                auto pipelineLayout = m_Renderer->BindGraphicsPipeline(commandBuffer, RenderPipelineKeys::LineWithDepth);
                vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT,
                    0, sizeof(ColorPushConstants), &constants);

                auto& lineMesh = m_Registry.get<LineMeshComponent>(m_HeightLinesEntity);
                lineMesh.Draw(commandBuffer);
            }
            if (m_BDebugLines[DebugLine::BSpline]) {	// Draw BSplines
                ColorPushConstants constants;
                constants.MVP = vp;
                auto pipelineLayout = m_Renderer->BindGraphicsPipeline(commandBuffer, RenderPipelineKeys::LineStripWithDepth);
                vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT,
                    0, sizeof(ColorPushConstants), &constants);

                auto view = m_Registry.view<LineMeshComponent, BSplineComponent>();
                for (auto [entity, lineMesh, bSpline] : view.each()) {
                    lineMesh.Draw(commandBuffer);
                }
            }
            {	// Draw terrain
                MeshPushConstants constants;
                constants.MVP = vp;
                constants.InvModelMat = glm::mat4(1.f);
                auto pipelineLayout = m_Renderer->BindGraphicsPipeline(commandBuffer, RenderPipelineKeys::LitColor);
                vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT,
                    0, sizeof(MeshPushConstants), &constants);
                auto& terrain = m_Registry.get<MeshComponent>(m_TerrainEntity);
                terrain.Draw(commandBuffer);
            }
            {	// Draw models
                auto pipelineLayout = m_Renderer->BindGraphicsPipeline(commandBuffer, RenderPipelineKeys::Basic);
                auto view = m_Registry.view<TransformComponent, MeshComponent, TextureComponent>();
                for (auto [entity, transform, mesh, texture] : view.each()) {
                    MeshPushConstants constants;
                    //constants.MVP = vp * transform.GetTransform();
                    glm::mat4 modelMat = glm::translate(transform.Position);
                    modelMat = glm::scale(modelMat, transform.Scale);
                    constants.MVP = vp * modelMat;
                    constants.InvModelMat = glm::inverse(modelMat);
                    vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT,
                        0, sizeof(MeshPushConstants), &constants);

                    texture.Bind(commandBuffer);

                    mesh.Draw(commandBuffer);
                }
            }
        } else { // Debug drawing of normals for geometry
            auto pipelineLayout = m_Renderer->BindGraphicsPipeline(commandBuffer, RenderPipelineKeys::Normal);
            auto view = m_Registry.view<TransformComponent, MeshComponent, TextureComponent>();
            for (auto [entity, transform, mesh, texture] : view.each()) {
                MeshPushConstants constants;
                //constants.MVP = vp * transform.GetTransform();
                glm::mat4 modelMat = glm::translate(transform.Position);
                modelMat = glm::scale(modelMat, transform.Scale);
                constants.MVP = vp * modelMat;
                constants.InvModelMat = glm::inverse(modelMat);
                vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT,
                    0, sizeof(MeshPushConstants), &constants);

                texture.Bind(commandBuffer);

                mesh.Draw(commandBuffer);
            }
        }

        if (m_BShowPointcloud) {	// Draw point cloud
            auto pipelineLayout = m_Renderer->BindGraphicsPipeline(commandBuffer, RenderPipelineKeys::Point);
            auto view = m_Registry.view<PointCloudComponent>();
            for (auto [entity, cloud] : view.each()) {
                ColorPushConstants constants;
                constants.MVP = vp;
                vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT,
                    0, sizeof(ColorPushConstants), &constants);
                cloud.Draw(commandBuffer);
            }
        }

        if (m_DebugDraw) { // Draw debug lines
            auto pipelineLayout = m_Renderer->BindGraphicsPipeline(commandBuffer, RenderPipelineKeys::Line);

            ColorPushConstants constants;
            constants.MVP = vp;
            vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT,
                0, sizeof(ColorPushConstants), &constants);

            auto& lineMesh = m_Registry.get<LineMeshComponent>(m_DebugLineEntity);
            lineMesh.Draw(commandBuffer);

            auto& sphereMesh = m_Registry.get<LineMeshComponent>(m_DebugSphereEntity);
            for (auto& transform : m_DebugSphereTransforms) {
                constants.MVP = vp * transform;
                vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT,
                    0, sizeof(ColorPushConstants), &constants);
                sphereMesh.Draw(commandBuffer);
            }

            auto& boxMesh = m_Registry.get<LineMeshComponent>(m_DebugAABBEntity);
            for (auto& transform : m_DebugAABBTransforms) {
                constants.MVP = vp * transform;
                vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT,
                    0, sizeof(ColorPushConstants), &constants);
                boxMesh.Draw(commandBuffer);
            }
        }

        {	// Draw ImGui
            ImGui::Render();
            ImDrawData* drawData = ImGui::GetDrawData();
            ImGui_ImplVulkan_RenderDrawData(drawData, commandBuffer);
        }

        m_Renderer->EndRecording();
        m_Renderer->SubmitAndPresent();
    }

    void Application::DebugInit() {
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

    void Application::MakeHeightLines() {
        std::vector<ColorVertex> heightLines;
        glm::vec3 color{ 1.f, 1.f, 1.f };
        auto& terrain = m_Registry.get<TerrainComponent>(m_TerrainEntity);
        float minY = std::numeric_limits<float>::max();
        float maxY = std::numeric_limits<float>::min();
        for (auto& triangle : terrain.Triangles) {
            if (triangle.A.y < minY)
                minY = triangle.A.y;
            if (triangle.B.y < minY)
                minY = triangle.B.y;
            if (triangle.C.y < minY)
                minY = triangle.C.y;

            if (triangle.A.y > maxY)
                maxY = triangle.A.y;
            if (triangle.B.y > maxY)
                maxY = triangle.B.y;
            if (triangle.C.y > maxY)
                maxY = triangle.C.y;
        }

        Plane p;
        p.pos = glm::vec3(0.f, minY, 0.f);
        p.normal = glm::vec3(0.f, 1.f, 0.f);
        for (float height = minY; height < maxY; height += 50.f) {
            p.pos.y = height;
            for (auto triangle : terrain.Triangles) {
                bool above = false;
                bool below = false;

                if (triangle.A.y > p.pos.y) {
                    above = true;
                } else {
                    below = true;
                }
                if (triangle.B.y > p.pos.y) {
                    above = true;
                } else {
                    below = true;
                }
                if (triangle.C.y > p.pos.y) {
                    above = true;
                } else {
                    below = true;
                }

                // Check if triangle is intersecting plane
                if (above && below) {
                    std::vector<glm::vec3> abovePositions;
                    std::vector<glm::vec3> belowPositions;

                    if (triangle.A.y > p.pos.y) {
                        abovePositions.push_back(triangle.A);
                    } else {
                        belowPositions.push_back(triangle.A);
                    }
                    if (triangle.B.y > p.pos.y) {
                        abovePositions.push_back(triangle.B);
                    } else {
                        belowPositions.push_back(triangle.B);
                    }
                    if (triangle.C.y > p.pos.y) {
                        abovePositions.push_back(triangle.C);
                    } else {
                        belowPositions.push_back(triangle.C);
                    }

                    for (auto& a : abovePositions) {
                        for (auto& b : belowPositions) {
                            glm::vec3 ab = b - a;
                            float d = glm::dot(p.normal, p.pos);
                            float t = (d - glm::dot(p.normal, a)) / glm::dot(p.normal, ab);
                            glm::vec3 intersectionPoint = a + t * ab;

                            ColorVertex v;
                            // Small offset to combat z-fighting
                            v.Pos = intersectionPoint + triangle.N * 0.005f;
                            v.Color = color;
                            heightLines.push_back(v);
                        }
                    }
                }
            }
        }
        m_HeightLinesEntity = m_Registry.create();
        m_Registry.emplace<LineMeshComponent>(m_HeightLinesEntity, heightLines);
    }

    const void Application::SpawnRain(const int count) {

        auto& terrain = m_Registry.get<TerrainComponent>(m_TerrainEntity);
        const int minX{ 0 };
        const int maxX{ terrain.Height };
        const int minZ{ 0 };
        const int maxZ{ terrain.Width };

        for (int i = 0; i < count; i++) {
            float rad = Math::RandFloat(0.2f, 0.7f);
            float mass = rad * 10.f;
            glm::vec3 loc(Math::RandDouble(minX, maxX), 20.f, Math::RandDouble(minZ, maxZ));
            SpawnBall(loc, rad, mass, 0.10f);
        }
    }

    const void Application::SpawnBall(glm::vec3 location, const float radius, const float mass, const float elasticity, const std::string& texture) {
        const auto ballEntity = m_Registry.create();
        auto& transform = m_Registry.emplace<TransformComponent>(ballEntity);
        auto& ball = m_Registry.emplace<BallComponent>(ballEntity);
        auto& time = m_Registry.emplace<TimeComponent>(ballEntity);
        auto& spline = m_Registry.emplace<BSplineComponent>(ballEntity);
        std::vector<ColorVertex> tempBuffer(m_MaxBSplineLines);
        auto& lineMesh = m_Registry.emplace<LineMeshComponent>(ballEntity, tempBuffer);
        tempBuffer.clear();
        lineMesh.UpdateBuffer(tempBuffer);

        ball.Radius = radius;
        ball.Mass = mass;
        ball.Elasticity = elasticity;
        time.CreationTime = Timer::GetTime();
        time.LastPoint = time.CreationTime;

        auto& velocity = m_Registry.emplace<VelocityComponent>(ballEntity);
        m_Registry.emplace<MeshComponent>(ballEntity, "Assets/Ball.obj");
        m_Registry.emplace<TextureComponent>(ballEntity, texture);

        transform.Position = location;
        transform.Scale = glm::vec3(ball.Radius);

        ball.CollisionSphere.radius = ball.Radius;
        ball.CollisionSphere.pos = transform.Position;

        m_BallCount++;
    }

    void Application::DebugDrawPath(std::vector<glm::vec3>& path) {
        for (int i{ 1 }; i < path.size(); i++) {
            DebugDrawLine(path[i - 1], path[i], glm::vec3(255.f, 255.f, 255.f));
        }

    }

}