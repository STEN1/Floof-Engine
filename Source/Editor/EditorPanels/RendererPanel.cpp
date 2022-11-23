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

		ImGui::End();
	}
}