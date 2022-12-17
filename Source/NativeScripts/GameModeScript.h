#ifndef FLOOF_GAMEMODESCRIPT_H
#define FLOOF_GAMEMODESCRIPT_H

#include "../Scene.h"
#include "NativeScript.h"
#include "../Application.h"
#include "CarBaseScript.h"
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

            EditorUpdate(deltaTime);
            return;
            //send all players data
            auto &Server = Application::Get().server;
            auto &Client = Application::Get().client;
            if (Server) {
                olc::net::message<FloofMsgHeaders> msg;
                msg.header.id = FloofMsgHeaders::GameUpdatePlayer;
                CarData data;
                auto v = m_Scene->GetRegistry().view<PlayerControllerComponent, NativeScriptComponent>();
                for (auto [ent, player, script]: v.each()) {
                    if (player.mPlayer == 1) {
                        auto car = dynamic_cast<CarBaseScript *>(script.Script.get());
                        data = car->GetTransformData();
                        data.id = 1;
                        msg << data;
                        Server->MessageAllClients(msg);
                    }
                }
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
                        auto car = dynamic_cast<CarBaseScript *>(script.Script.get());
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
                        m_Scene->AddComponent<NativeScriptComponent>(ent, std::make_unique<CarBaseScript>(glm::vec3(0.f)), m_Scene, ent);
                        m_Scene->AddComponent<PlayerControllerComponent>(ent, client->GetID());
                        auto script = m_Scene->GetComponent<NativeScriptComponent>(ent).Script.get();
                        auto car = dynamic_cast<CarBaseScript *>(script);
                        car->AddToPhysicsWorld();
                        Server->ActivePlayers.emplace_back(client);

                        olc::net::message<FloofMsgHeaders> sendMsg;
                        sendMsg.header.id = FloofMsgHeaders::GameAddPlayer;
                        CarData cData;
                        cData = car->GetTransformData();
                        cData.id = client->GetID();
                        sendMsg << cData;
                        Server->MessageAllClients(sendMsg, client);
                    }
                }
                //cleanup initializevector
                Server->MarkedPlayerForInitialize.clear();
                //remove Players
                for (auto &client: Server->MarkedPlayerForRemove) {
                    //   //todo remove entity on disconnect

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
        }
    };
}

#endif //FLOOF_GAMEMODESCRIPT_H
