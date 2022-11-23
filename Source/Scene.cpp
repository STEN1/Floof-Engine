#include "Scene.h"

#include "Application.h"
#include "Components.h"
#include "SoundManager.h"

namespace FLOOF {
    Scene::Scene() {
        static uint32_t sceneID = 0;
        m_SceneID = sceneID++;

        m_SceneRenderer = std::make_unique<SceneRenderer>();

        m_PhysicSystem = std::make_unique<PhysicsSystem>(m_Registry);
        m_PhysicsDebugDrawer = std::make_unique<PhysicsDebugDraw>();

        m_PhysicsDebugDrawer->setDebugMode(btIDebugDraw::DBG_NoDebug);
        auto world = m_PhysicSystem->GetWorld();
        world->setDebugDrawer(m_PhysicsDebugDrawer.get());
    }

    Scene::~Scene() {
    }

    entt::entity Scene::CreateEntity(const std::string& tag, entt::entity parent) {
        entt::entity entity = m_Registry.create();

        auto& transform = m_Registry.emplace<TransformComponent>(entity);
        auto& rel = m_Registry.emplace<Relationship>(entity);
        auto& tagComponent = m_Registry.emplace<TagComponent>(entity);

        tagComponent.Tag = tag;

        if (parent != entt::null) {
            rel.Parent = parent;

            auto& parentRel = m_Registry.get<Relationship>(parent);
            auto& parentTransform = m_Registry.get<TransformComponent>(parent);

            transform.Parent = &parentTransform;
            parentRel.Children.push_back(entity);
        }

        return entity;
    }

    entt::registry& Scene::GetCulledScene() {
        return m_Registry;
    }

    entt::registry& Scene::GetRegistry() {
        return m_Registry;
    }

    VkSemaphore Scene::OnDraw(float deltaTime)
    {
        ImGui::PushStyleVar(ImGuiStyleVar_WindowMinSize, ImVec2(300.f, 300.f));
        ImGui::Begin((std::string("Scene renderer") + std::to_string(m_SceneID)).c_str());
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

        auto sceneRenderData = m_SceneRenderer->RenderToTexture(this, sceneCanvasExtent);

        if (sceneRenderData.Texture != VK_NULL_HANDLE) {
            ImDrawList* draw_list = ImGui::GetWindowDrawList();
            draw_list->AddImage(sceneRenderData.Texture, canvas_p0, canvas_p1, ImVec2(0, 0), ImVec2(1, 1));
        }
        ImGui::End();

        return sceneRenderData.SceneRenderFinishedSemaphore;
    }

    void Scene::OnUpdate(float deltaTime) {
        auto nativeScriptView = m_Registry.view<NativeScriptComponent>();
        for (auto [entity, nativeScript] : nativeScriptView.each()) {
            nativeScript.Script->OnUpdate(deltaTime);
        }
        auto PythonScriptView = m_Registry.view<ScriptComponent>();
        for (auto [entity, Script] : PythonScriptView.each()) {
            Script.OnUpdate(deltaTime);
        }

        m_PhysicSystem->OnUpdate(deltaTime);
        SoundManager::Update();

    }

    void Scene::OnCreate() {
        auto PythonScriptView = m_Registry.view<ScriptComponent>();
        for (auto [entity, Script] : PythonScriptView.each()) {
            Script.OnCreate();
        }
    }
}
