#include "SoundSettingsPanel.h"
#include "../../Application.h"
#include "../../Scene.h"

namespace FLOOF {
    void SoundSettingsPanel::DrawPanel() {
        auto& app = Application::Get();

        ImGui::Begin("Sounds Settings");

		std::vector<std::string> devices = SoundManager::GetAvailableDevices();

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