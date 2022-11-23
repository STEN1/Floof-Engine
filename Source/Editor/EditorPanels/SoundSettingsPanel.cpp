#include "SoundSettingsPanel.h"
#include "../../Application.h"
#include "../../Scene.h"
#include "../EditorLayer.h"

namespace FLOOF {
    void SoundSettingsPanel::DrawPanel() {
        ImGui::Begin("Sounds Settings");

		if (ImGui::SmallButton("Refresh Device List"))
			SoundManager::UpdateDeviceList();

		std::vector<std::string> devices = SoundManager::GetDeviceList();

		static int item_current_idx = 0; // Here we store our selection data as an index.
		const char* combo_preview_value = devices[item_current_idx].c_str();  // Pass in the preview value visible before opening the combo (it could be anything)
		if (ImGui::BeginCombo("Sound Device", combo_preview_value)) {
			for (int n = 0; n < devices.size(); n++) {
				const bool is_selected = (item_current_idx == n);
				if (ImGui::Selectable(devices[n].c_str(), is_selected)) {
					item_current_idx = n;
					SoundManager::SetNewDevice(devices[n]);
				}

				// Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
				if (is_selected)
					ImGui::SetItemDefaultFocus();
			}
			ImGui::EndCombo();
		}

    	ImGui::End();
    }
}