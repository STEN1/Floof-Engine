#ifndef FLOOF_GAMEMODESCRIPT_H
#define FLOOF_GAMEMODESCRIPT_H

#include "../Scene.h"
#include "NativeScript.h"
#include "../Application.h"
#include "CarBaseScript.h"
#include "../Timer.h"
#include "CarScripts/RaceCarScript.h"
#include "CarScripts/MonsterTruckScript.h"
#include "RaceTrackScript.h"

namespace FLOOF {
    struct GameModeScript : public NativeScript {
    public:
        void OnCreate(Scene *scene, entt::entity entity) override {
            NativeScript::OnCreate(scene, entity);
        };

        entt::entity RaceTrack{entt::null};

        void MakeUi() {
            ImGui::Begin("GameMode");

            //TIMELAPSE

            if (RaceTrack != entt::null) {
                auto &script = GetComponent<NativeScriptComponent>(RaceTrack);
                auto cpScript = dynamic_cast<RaceTrackScript *>(script.Script.get());
                if (cpScript) {

                    std::string tmp{"Fastest Lap Time : "};
                    tmp += std::to_string(cpScript->GetFastetTime());
                    if (cpScript->GetFastetTime() != std::numeric_limits<double>::max()) {
                        ImGui::Text(tmp.c_str());
                        ImGui::Separator();
                    }
                    tmp = "LapCount Time : ";
                    tmp += std::to_string(cpScript->GetLapCount());
                    if (cpScript->GetFastetTime() != 0) {
                        ImGui::Text(tmp.c_str());
                        ImGui::Separator();
                    }

                    auto Times = cpScript->GetCheckPointTimes();
                    for (auto [nmb, time]: Times) {
                        tmp = "Check : ";
                        tmp += std::to_string(nmb);
                        tmp += " time : ";
                        tmp += std::to_string(time);
                        ImGui::Text(tmp.c_str());
                    }
                }
            }


            //CAR SELECTION
            if (ImGui::CollapsingHeader("Car Selection")) {
                if (ImGui::Button("Koenigsegg")) {
                    //get Player ent
                    auto view = m_Scene->GetRegistry().view<PlayerControllerComponent>();
                    for (auto [ent, player]: view.each()) {
                        if (player.mPlayer == m_Scene->ActivePlayer) {
                            PlayerEnt = ent;
                        }
                    }
                    if (PlayerEnt != entt::null)
                        m_Scene->DestroyEntity(PlayerEnt);
                    PlayerEnt = m_Scene->CreateEntity("Local Player");
                    auto &script = m_Scene->AddComponent<NativeScriptComponent>(PlayerEnt, std::make_unique<RaceCarScript>(glm::vec3(0.f, -40.f, 0.f)), m_Scene, PlayerEnt);
                    m_Scene->AddComponent<PlayerControllerComponent>(PlayerEnt, m_Scene->ActivePlayer);
                    auto cpScript = dynamic_cast<CarBaseScript *>(script.Script.get());
                    if (cpScript)
                        cpScript->AddToPhysicsWorld();
                }
                if (ImGui::Button("Cybertruck")) {
                    //get Player ent
                    auto view = m_Scene->GetRegistry().view<PlayerControllerComponent>();
                    for (auto [ent, player]: view.each()) {
                        if (player.mPlayer == m_Scene->ActivePlayer) {
                            PlayerEnt = ent;
                        }
                    }
                    if (PlayerEnt != entt::null)
                        m_Scene->DestroyEntity(PlayerEnt);
                    PlayerEnt = m_Scene->CreateEntity("Local Player");
                    auto &script = m_Scene->AddComponent<NativeScriptComponent>(PlayerEnt, std::make_unique<MonsterTruckScript>(glm::vec3(0.f, -40.f, 0.f)), m_Scene, PlayerEnt);
                    m_Scene->AddComponent<PlayerControllerComponent>(PlayerEnt, m_Scene->ActivePlayer);
                    auto cpScript = dynamic_cast<CarBaseScript *>(script.Script.get());
                    if (cpScript)
                        cpScript->AddToPhysicsWorld();
                }
            }
            ImGui::End();
        }

    private:
        std::chrono::high_resolution_clock::time_point lastPing = Timer::GetTime();
        entt::entity PlayerEnt{entt::null};

    public:
        void OnUpdate(float deltaTime) override {
            EditorUpdate(deltaTime);
            return;
        }


        void LastUpdate(float deltaTime) override {

        };

        void EditorUpdate(float deltaTime) override {
            //create all players;
            auto &Server = Application::Get().server;
            auto &Client = Application::Get().client;
            if (Server) {
                for (auto &client: Server->MarkedPlayerForInitialize) {
                    //make Players
                    {
                        std::string Player = "Player : ";
                        Player += std::to_string(client.first->GetID());
                        auto ent = m_Scene->CreateEntity(Player);
                        switch (client.second) {
                            case 1 : //racecar
                                m_Scene->AddComponent<NativeScriptComponent>(ent, std::make_unique<RaceCarScript>(glm::vec3(0.f)), m_Scene, ent);
                                break;
                            case 2: //cybertruck
                                m_Scene->AddComponent<NativeScriptComponent>(ent, std::make_unique<MonsterTruckScript>(glm::vec3(0.f)), m_Scene, ent);
                                break;
                        }
                        m_Scene->AddComponent<PlayerControllerComponent>(ent, client.first->GetID());
                        auto script = m_Scene->GetComponent<NativeScriptComponent>(ent).Script.get();
                        auto car = dynamic_cast<CarBaseScript *>(script);
                        car->AddToPhysicsWorld();
                        Server->ActivePlayers.emplace_back(client);

                        olc::net::message<FloofMsgHeaders> sendMsg;
                        sendMsg.header.id = FloofMsgHeaders::GameAddPlayer;
                        CarData cData;
                        cData = car->GetTransformData();
                        cData.id = client.first->GetID();
                        sendMsg << cData;
                        Server->MessageAllClients(sendMsg, client.first);
                    }
                }
                //cleanup initializevector
                Server->MarkedPlayerForInitialize.clear();
                //remove Players
                for (auto &client: Server->MarkedPlayerForRemove) {
                    auto v = m_Scene->GetRegistry().view<PlayerControllerComponent, NativeScriptComponent>();
                    for (auto [ent, player, script]: v.each()) {
                        if (player.mPlayer == client) {
                            m_Scene->DestroyEntity(ent);
                        }
                    }
                }
                Server->MarkedPlayerForRemove.clear();

                //update location from hosters car
                olc::net::message<FloofMsgHeaders> msg;
                msg.header.id = FloofMsgHeaders::GameUpdatePlayer;
                CarData data;
                auto v = m_Scene->GetRegistry().view<PlayerControllerComponent, NativeScriptComponent>();
                for (auto [ent, player, script]: v.each()) {
                    if (player.mPlayer == m_Scene->ActivePlayer) {
                        auto car = dynamic_cast<CarBaseScript *>(script.Script.get());
                        data = car->GetTransformData();
                        data.id = player.mPlayer;
                        msg << data;
                        Server->MessageAllClients(msg);
                    }
                }
            } else if (Client && Client->Initialized) {

                //ping every half second
                if (Timer::GetTimeSince(lastPing) > 0.5f) {
                    Client->PingServer();
                    lastPing = Timer::GetTime();
                }

                olc::net::message<FloofMsgHeaders> msg;
                msg.header.id = FloofMsgHeaders::GameUpdatePlayer;
                CarData data;
                auto v = m_Scene->GetRegistry().view<PlayerControllerComponent, NativeScriptComponent>();
                for (auto [ent, player, script]: v.each()) {
                    if (player.mPlayer == m_Scene->ActivePlayer) {
                        auto car = dynamic_cast<CarBaseScript *>(script.Script.get());
                        data = car->GetTransformData();
                        data.id = player.mPlayer;
                        msg << data;
                        Client->Send(msg);
                    }
                }
            }

            MakeUi();
        }
    };
}

#endif //FLOOF_GAMEMODESCRIPT_H
