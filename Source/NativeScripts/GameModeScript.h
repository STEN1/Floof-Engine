#ifndef FLOOF_GAMEMODESCRIPT_H
#define FLOOF_GAMEMODESCRIPT_H

#include "../Scene.h"
#include "NativeScript.h"
#include "../Application.h"
#include "MonsterTruckScript.h"
#include "../Timer.h"

namespace FLOOF {
    struct GameModeScript : public NativeScript {
    public:
        void OnCreate(Scene *scene, entt::entity entity) override {
            NativeScript::OnCreate(scene, entity);
        };
    private:
        std::chrono::high_resolution_clock::time_point lastPing = Timer::GetTime();
    public:
        void OnUpdate(float deltaTime) override {

            //send all players data
            auto &Server = Application::Get().server;
            auto &Client = Application::Get().client;
            if (Server) {

            }
            if (Client && Client->Initialized) {
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
                    if (player.mPlayer == Client->GetID()) {
                        auto car = dynamic_cast<MonsterTruckScript*>(script.Script.get());
                        data = car->GetTransformData();
                        data.id = Client->GetID();
                        msg << data;
                        Client->Send(msg);
                    }
                }
            }
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
                        Player += std::to_string(client->GetID());
                        auto ent = m_Scene->CreateEntity(Player);
                        m_Scene->AddComponent<NativeScriptComponent>(ent, std::make_unique<MonsterTruckScript>(glm::vec3(0.f, -40.f, 10.f * (client->GetID() - 999.f))), m_Scene, ent);
                        m_Scene->AddComponent<PlayerControllerComponent>(ent, client->GetID());

                        Server->ActivePlayers.emplace_back(client);
                    }
                }
                //cleanup initializevector
                Server->MarkedPlayerForInitialize.clear();
                //remove Players
                for (auto &client: Server->MarkedPlayerForRemove) {
                    //   //todo remove entity on disconnect

                }
                Server->MarkedPlayerForRemove.clear();

            } else if (Client && Client->Initialized) {
                //ping every half second
                if (Timer::GetTimeSince(lastPing) > 0.5f) {
                    Client->PingServer();
                    lastPing = Timer::GetTime();
                }
            }

        }
    };
}

#endif //FLOOF_GAMEMODESCRIPT_H
