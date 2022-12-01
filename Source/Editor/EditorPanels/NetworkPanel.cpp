#include "NetworkPanel.h"
#include "../EditorLayer.h"
void FLOOF::NetworkPanel::DrawPanel() {

    ImGui::Begin("NetworkPanel");


    if(ImGui::Button("Connect client")){
        auto& cli = m_EditorLayer->GetScene()->client;

        cli.Connect("127.0.0.1",25565);
    }


    ImGui::End();



}
