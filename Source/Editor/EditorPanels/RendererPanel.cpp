#include "RendererPanel.h"
#include "../../Application.h"
#include "../../Renderer/SceneRenderer.h"

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
		auto& app = Application::Get();
		auto& sceneRenderer = *app.m_SceneRenderer.get();

		static int selectedDrawMode = static_cast<int>(app.m_DrawMode);

		ImGui::Begin("Renderer");

		if (ImGui::Combo("DrawMode",
			&selectedDrawMode,
			ApplicationDrawModes,
			IM_ARRAYSIZE(ApplicationDrawModes)))
		{
			app.SetDrawMode(static_cast<RenderPipelineKeys>(selectedDrawMode));
		}

		ImGui::End();
	}
}