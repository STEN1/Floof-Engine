#pragma once
#include "../ApplicationLayer.h"
#include "entt/entt.hpp"
#include <unordered_map>
#include <string>
#include "EditorPanels/EnginePanel.h"
#include "../Renderer/SceneRenderer.h"
#include "../Scene.h"

namespace FLOOF {
	class EditorLayer : public ApplicationLayer {
	public:
		EditorLayer();
		~EditorLayer();
		virtual void OnUpdate(double deltaTime) override;
		virtual void OnImGuiUpdate(double deltaTime) override;
		virtual VkSemaphore OnDraw(double deltaTime, VkSemaphore waitSemaphore) override;
		void SetEditorDrawMode(RenderPipelineKeys drawMode) { m_EditorDrawMode = drawMode; }
		void SetPlayDrawMode(RenderPipelineKeys drawMode) { m_PlayDrawMode = drawMode; }
		void SelectDebugScene(DebugScenes type);
		DebugScenes GetCurrentDebugScene() { return m_CurrentDebugScene; }
		Scene* GetScene() { return m_Scene.get(); }
		void StartPlay();
		void StopPlay();
		bool IsPlaying() { return m_PlayModeActive; }
	private:
		void UpdateEditorCamera(double deltaTime);

		void MakePhysicsScene();
		void MakePhysicsPlayGround();
		void MakeRenderingDemoScene();
		void MakeAudioTestScene();
		void MakeLandscapeScene();

		std::unordered_map<std::string, std::unique_ptr<EditorPanel>> m_EditorPanels;

		std::unique_ptr<SceneRenderer> m_EditorRenderer;
		std::unique_ptr<SceneRenderer> m_PlayRenderer;

		std::unique_ptr<Scene> m_Scene;

		double m_CameraSpeed{ 50.f };

		bool m_EditorViewFocused = false;
		bool m_PlayViewFocused = false;

		bool m_PlayModeActive = false;

		DebugScenes m_CurrentDebugScene;
		RenderPipelineKeys m_EditorDrawMode = RenderPipelineKeys::PBR;
		RenderPipelineKeys m_PlayDrawMode = RenderPipelineKeys::PBR;
	};
}