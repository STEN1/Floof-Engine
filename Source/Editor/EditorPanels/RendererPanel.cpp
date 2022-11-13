#include "RendererPanel.h"
#include "../../Application.h"
#include "../../Renderer/SceneRenderer.h"

namespace FLOOF {
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

		ImGui::Separator();

		ImGui::DragFloat("Metallic", &sceneRenderer.m_SceneFrameData.metallic, 0.01f, 0.f, 1.f);
		ImGui::DragFloat("Roughness", &sceneRenderer.m_SceneFrameData.roughness, 0.01f, 0.f, 1.f);

		ImGui::End();
	}
}