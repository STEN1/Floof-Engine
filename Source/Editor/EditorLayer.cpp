#include "EditorLayer.h"

#include "../Application.h"
#include <filesystem>
#include "EditorPanels/ApplicationPanel.h"
#include "EditorPanels/SceneGraphPanel.h"
#include "EditorPanels/ComponentsPanel.h"
#include "EditorPanels/PhysicsPanel.h"
#include "EditorPanels/RendererPanel.h"
#include "EditorPanels/SoundSettingsPanel.h"

namespace FLOOF {
    EditorLayer::EditorLayer()
    {
        m_EditorPanels.try_emplace("ApplicationPanel", std::make_unique<ApplicationPanel>());
        m_EditorPanels.try_emplace("SceneGraphPanel", std::make_unique<SceneGraphPanel>());
        m_EditorPanels.try_emplace("ComponentsPanel", std::make_unique<ComponentsPanel>());
        m_EditorPanels.try_emplace("PhysicsPanel", std::make_unique<PhysicsPanel>());
        m_EditorPanels.try_emplace("RendererPanel", std::make_unique<RendererPanel>());
        m_EditorPanels.try_emplace("EnginePanel", std::make_unique<EnginePanel>());
        m_EditorPanels.try_emplace("SoundSettingsPanel", std::make_unique<SoundSettingsPanel>());
    }

    void EditorLayer::OnUpdate(float deltaTime)
	{
	}
	void EditorLayer::OnImGuiUpdate(float deltaTime)
	{
        auto& app = Application::Get();
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

        // Ends Dockspace window
        ImGui::End();

        if (showDemoWindow)
            ImGui::ShowDemoWindow(&showDemoWindow);

        for (auto& [key, editorPanel] : m_EditorPanels) {
            editorPanel->DrawPanel();
        }

        //set last entity after ui stuff is done
        app.m_Scene->m_LastSelectedEntity = app.m_Scene->m_SelectedEntity;
	}
}