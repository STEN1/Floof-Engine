#include "NetworkPanel.h"
#include "../EditorLayer.h"

void FLOOF::NetworkPanel::DrawPanel() {

    ImGui::Begin("NetworkPanel");


    if (ImGui::Button("Create Server")) {
        auto &Server = m_EditorLayer->GetScene()->server;
        Server = std::make_unique<CarServer>(25565);

        Server->Start();
    }


    if (ImGui::Button("Connect client")) {
        auto &cli = m_EditorLayer->GetScene()->client;

    }


    ImGui::End();


}
