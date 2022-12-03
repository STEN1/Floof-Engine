#include "NetworkPanel.h"
#include "../EditorLayer.h"
#include "../../Application.h"

void FLOOF::NetworkPanel::DrawPanel() {

    ImGui::Begin("NetworkPanel");

    if (!Active) {
        if (ImGui::Button("Host Game"))
            ImGui::OpenPopup("HostServer");

        // Always center this window when appearing
        ImVec2 center = ImGui::GetMainViewport()->GetCenter();

        ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
        if (ImGui::BeginPopupModal("HostServer", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::InputInt("Port", &Port, 1, 2, 0);
            if (ImGui::Button("Start Server")) {
                auto &Server = Application::Get().server;

                Server = std::make_unique<FloofServer>(Port);

                Server->Start();
                Active = true;
                con = Type::Server;

                ImGui::CloseCurrentPopup();
            }
            if (ImGui::Button("Close"))
                ImGui::CloseCurrentPopup();

            ImGui::EndPopup();
        }

        if (ImGui::Button("Join Game"))
            ImGui::OpenPopup("JoinGame");

        ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
        if (ImGui::BeginPopupModal("JoinGame", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::InputText("Ip", Ip.data(), Ip.size());
            ImGui::InputInt("Port", &Port, 1, 2, 0);
            if (ImGui::Button("Join Game")) {
                auto &cli = Application::Get().client;
                cli.Connect(Ip, Port);
                Active = true;
                con = Type::Client;

                ImGui::CloseCurrentPopup();
            }
            if (ImGui::Button("Close"))
                ImGui::CloseCurrentPopup();

            ImGui::EndPopup();
        }


    }
    if (Active) {
        auto &Server = Application::Get().server;
        if (ImGui::CollapsingHeader("Network Stats")) {
            std::string txt{"Active Network Connection"};
            ImGui::Text(txt.c_str());
            switch (con) {
                case Type::Server:
                    txt = "Server";
                    ImGui::Text(txt.c_str());
                    txt = "Port :";
                    txt += std::to_string(Port);
                    ImGui::Text(txt.c_str());
                    txt = "Clients Connected : ";
                    txt += std::to_string(Server->ConnectionCount());
                    ImGui::Text(txt.c_str());
                    break;
                case Type::Client:
                    txt = "Client";
                    ImGui::Text(txt.c_str());
                    txt = "Ip : ";
                    txt += Ip;
                    txt += " :  ";
                    txt += std::to_string(Port);
                    ImGui::Text(txt.c_str());
                    break;
            }
        }
        switch (con) {
            case Type::Server:
                if (ImGui::Button("Stop Server")) {
                    auto &Server = Application::Get().server;
                    Server->Stop();
                    Server.release();
                    Active = false;
                }
                break;
            case Type::Client:
                if (ImGui::Button("Stop Client")) {
                    auto &cli = Application::Get().client;
                    cli.Disconnect();
                    Active = false;
                }
                break;
        }
    }

    ImGui::End();


}
