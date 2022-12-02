#include "RendererPanel.h"
#include "../../Application.h"
#include "../../Renderer/SceneRenderer.h"
#include "../EditorLayer.h"

namespace FLOOF {
	// Matches indices from RenderPipelineKeys in VulkanRenderer.h
	static const char* ApplicationDrawModes[] = {
	"Wireframe",
	"UnLit",
	"PBR",
	"Normals",
	"UV",
	};
	void RendererPanel::DrawPanel()
	{
		static int editorDrawMode = static_cast<int>(RenderPipelineKeys::PBR);
		static int playDrawMode = static_cast<int>(RenderPipelineKeys::PBR);

		int frameIndex = VulkanRenderer::Get()->GetVulkanWindow()->FrameIndex;

		ImGui::Begin("Renderer");

		if (ImGui::Combo("Editor draw mode",
			&editorDrawMode,
			ApplicationDrawModes,
			IM_ARRAYSIZE(ApplicationDrawModes)))
		{
			m_EditorLayer->SetEditorDrawMode(static_cast<RenderPipelineKeys>(editorDrawMode));
		}

		if (ImGui::Combo("Play draw mode",
			&playDrawMode,
			ApplicationDrawModes,
			IM_ARRAYSIZE(ApplicationDrawModes)))
		{
			m_EditorLayer->SetPlayDrawMode(static_cast<RenderPipelineKeys>(playDrawMode));
		}

		static bool drawCameraLines = false;
		ImGui::Checkbox("Draw camera debug lines", &drawCameraLines);
		if (drawCameraLines) {
			float farClip = m_EditorLayer->GetPlayRenderer()->m_ShadowFarClip;
			m_EditorLayer->GetEditorRenderer()->DrawDebugCameraLines(m_EditorLayer->GetScene()->GetFirstSceneCamera(), farClip);
		}
		static float shadowFarClip = m_EditorLayer->GetEditorRenderer()->m_ShadowFarClip;
		if (ImGui::DragFloat("Shadow far clip", &shadowFarClip, 1.f, 2500.f, 15000.f)) {
			m_EditorLayer->GetEditorRenderer()->m_ShadowFarClip = shadowFarClip;
			m_EditorLayer->GetPlayRenderer()->m_ShadowFarClip = shadowFarClip;
		}

		ImVec2 imageSize(200.f, 200.f);

		ImGui::Separator();
		ImGui::Text("Editor Shadowmap");
		ImGui::NewLine();
		ImGui::Image(m_EditorLayer->GetEditorRenderer()->m_ShadowDepthBuffers[frameIndex]->GetTexture().DesctriptorSet, imageSize);

		if (m_EditorLayer->IsPlaying()) {
			ImGui::Separator();
			ImGui::Text("Play Shadowmap");
			ImGui::NewLine();
			ImGui::Image(m_EditorLayer->GetPlayRenderer()->m_ShadowDepthBuffers[frameIndex]->GetTexture().DesctriptorSet, imageSize);
		}

		ImGui::End();
	}
}