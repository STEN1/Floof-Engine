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

		ImGui::NewLine();
		ImGui::Separator();
		ImGui::DragFloat("SunStrenght", &sceneRenderer.m_SceneFrameData.sunStrenght, 0.01f, 0.f, 100.f);
		ImGui::NewLine();
		ImGui::Separator();
		ImGui::DragFloat3("SunDirection", &sceneRenderer.m_SceneFrameData.SunDirection[0], 0.01f);
		ImGui::NewLine();
		ImGui::Separator();
		ImGui::ColorPicker3("SunColor", &sceneRenderer.m_SceneFrameData.SunColor[0], ImGuiColorEditFlags_DisplayRGB);
		ImGui::NewLine();

		ImGui::End();
	}
}