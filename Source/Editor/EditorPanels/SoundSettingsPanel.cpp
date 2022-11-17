#include "SoundSettingsPanel.h"
#include "../../Application.h"
#include "../../Scene.h"

namespace FLOOF {
    void SoundSettingsPanel::DrawPanel() {
        auto& app = Application::Get();

        ImGui::Begin("Sounds Settings");
        ImGui::Text("Sound Settings");
        ImGui::End();

    }


}