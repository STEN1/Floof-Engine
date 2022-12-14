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

		// Rendering pipeline options
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

		// Sun color option
		ImGui::Separator();
		glm::vec4 sunColor = m_EditorLayer->GetEditorRenderer()->m_SceneFrameData.SunColor;
		if (ImGui::ColorEdit3("Sun color", &sunColor[0])) {
			m_EditorLayer->GetEditorRenderer()->m_SceneFrameData.SunColor = sunColor;
			m_EditorLayer->GetPlayRenderer()->m_SceneFrameData.SunColor = sunColor;
		}

		// Sun intensity
		float sunIntensity = m_EditorLayer->GetEditorRenderer()->m_SceneFrameData.sunStrenght;
		if (ImGui::DragFloat("Sun intensity", &sunIntensity, 0.2f, 0.f, 1000.f)) {
			m_EditorLayer->GetEditorRenderer()->m_SceneFrameData.sunStrenght = sunIntensity;
			m_EditorLayer->GetPlayRenderer()->m_SceneFrameData.sunStrenght = sunIntensity;
		}

		// Ambient intensity
		float ambientIntensity = m_EditorLayer->GetEditorRenderer()->m_SceneFrameData.AmbientIntensity;
		if (ImGui::DragFloat("Ambient intensity", &ambientIntensity, 0.01f, 0.f, 2.f)) {
			m_EditorLayer->GetEditorRenderer()->m_SceneFrameData.AmbientIntensity = ambientIntensity;
			m_EditorLayer->GetPlayRenderer()->m_SceneFrameData.AmbientIntensity = ambientIntensity;
		}


		// Shadow options
		ImGui::Separator();
		static bool drawCameraLines = false;
		ImGui::Checkbox("Draw camera debug lines", &drawCameraLines);
		if (drawCameraLines) {
			m_EditorLayer->GetEditorRenderer()->DrawDebugCameraLines();
		}
		float shadowFarClip = m_EditorLayer->GetEditorRenderer()->m_ShadowFarClip;
		if (ImGui::DragFloat("Shadow far clip", &shadowFarClip, 1.f, 2500.f, 15000.f)) {
			m_EditorLayer->GetEditorRenderer()->m_ShadowFarClip = shadowFarClip;
			m_EditorLayer->GetPlayRenderer()->m_ShadowFarClip = shadowFarClip;
		}
		float shadowBias = m_EditorLayer->GetEditorRenderer()->m_SceneFrameData.Bias;
		if (ImGui::DragFloat("Shadow bias", &shadowBias, 0.000001f, 0.f, 0.1f, "%.7f")) {
			m_EditorLayer->GetEditorRenderer()->m_SceneFrameData.Bias = shadowBias;
			m_EditorLayer->GetPlayRenderer()->m_SceneFrameData.Bias = shadowBias;
		}
		float shadowZOffset = m_EditorLayer->GetEditorRenderer()->m_ShadowZExtentOffset;
		if (ImGui::DragFloat("Shadow z offset", &shadowZOffset, 1.f, 0.f, 10000.f)) {
			m_EditorLayer->GetEditorRenderer()->m_ShadowZExtentOffset = shadowZOffset;
			m_EditorLayer->GetPlayRenderer()->m_ShadowZExtentOffset = shadowZOffset;
		}

		// Shadowmap visualization. only showing first cascade.
		ImVec2 imageSize(200.f, 200.f);

		ImGui::Separator();
		ImGui::Text("Editor Shadowmap");
		ImGui::Image(m_EditorLayer->GetEditorRenderer()->m_ShadowDepthBuffers[frameIndex]->GetTexture().DesctriptorSet, imageSize);

		if (m_EditorLayer->IsPlaying()) {
			ImGui::Separator();
			ImGui::Text("Play Shadowmap");
			ImGui::Image(m_EditorLayer->GetPlayRenderer()->m_ShadowDepthBuffers[frameIndex]->GetTexture().DesctriptorSet, imageSize);
		}

		ImGui::End();
	}
}