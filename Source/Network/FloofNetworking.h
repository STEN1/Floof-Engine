#ifndef FLOOF_FLOOFNETWORKING_H
#define FLOOF_FLOOFNETWORKING_H

#include <cstdint>
#include "Client.h"
#include "Server.h"
#include <chrono>
#include "../Scene.h"
#include "../NativeScripts/MonsterTruckScript.h"

#include "OlcNet.h"

namespace FLOOF {
    enum class FloofMsgHeaders : uint32_t {
        ServerGetStatus,
        ServerGetPing,

        ClientAccepted,
        ClientAssignID,
        ClientRegisterWithServer,
        ClientUnRegisterWithServer,

        GameAddPlayer,
        GameRemovePlayer,
        GameUpdatePlayer,
        GameUpdateWorld
    };

    struct PlayerData{
        TransformComponent Transform;
        uint32_t id;
    };
    struct CarData{
        TransformComponent MainTform;
        TransformComponent WheelTformFL;
        TransformComponent WheelTformFR;
        TransformComponent WheelTformBL;
        TransformComponent WheelTformBR;
        
        uint32_t id;
    };
    struct LevelData{
        Scene scene;
    };

    class FloofClient : public olc::net::client_interface<FloofMsgHeaders> {
    public:
        FloofClient() : client_interface() {

        };

        bool Initialized{false};

        void PingServer() {
            olc::net::message<FloofMsgHeaders> msg;
            msg.header.id = FloofMsgHeaders::ServerGetPing;

            auto timeNow = std::chrono::system_clock::now();
            msg << timeNow;

            Send(msg);
        }

        void Update(FLOOF::Scene* scene) {

            while (!Incoming().empty()) {
                auto msg = Incoming().pop_front().msg;

                switch (msg.header.id) {
                    case FloofMsgHeaders::ServerGetStatus:
                        break;
                    case FloofMsgHeaders::ServerGetPing: {
                        auto timeNow = std::chrono::system_clock::now();
                        std::chrono::system_clock::time_point timeThen;
                        msg >> timeThen;
                        scene->ping =std::chrono::duration<double>(timeNow - timeThen).count();
                        //PingServer();
                        break;
                    }
                    case FloofMsgHeaders::ClientAccepted: {
                        Initialized = true;
                        olc::net::message<FloofMsgHeaders> sendMsg;
                        sendMsg.header.id = FloofMsgHeaders::ClientRegisterWithServer;
                        PlayerData data;
                        data.Transform.Position = glm::vec3(0.f, -40.f, 10.f * (GetID() - 999.f));
                        data.id = GetID();
                        sendMsg << data;
                        Send(sendMsg);
                        //PingServer();
                        break;
                    }
                    case FloofMsgHeaders::ClientRegisterWithServer:
                        break;
                    case FloofMsgHeaders::ClientUnRegisterWithServer:
                        break;

                    case FloofMsgHeaders::ClientAssignID:
                        break;
                    case FloofMsgHeaders::GameAddPlayer:
                        break;
                    case FloofMsgHeaders::GameRemovePlayer:
                        break;
                    case FloofMsgHeaders::GameUpdatePlayer:
                        break;
                    case FloofMsgHeaders::GameUpdateWorld:
                        break;
                }

            }

        }

    };

    class FloofServer : public olc::net::server_interface<FloofMsgHeaders> {
    public:
        FloofServer(uint16_t port) : olc::net::server_interface<FloofMsgHeaders>(port) {

        };
        Scene * mScene;
    protected:
        virtual bool OnClientConnect(std::shared_ptr<olc::net::connection<FloofMsgHeaders>> client) override {
            return true;
        }

    public:
        std::vector<std::shared_ptr<olc::net::connection<FloofMsgHeaders>>> ActivePlayers;
        std::vector<uint32_t> MarkedPlayerForRemove;
        std::vector<std::shared_ptr<olc::net::connection<FloofMsgHeaders>>> MarkedPlayerForInitialize;

        size_t ConnectionCount(){
            return ActivePlayers.size();
        }
        void PingClients() {
            olc::net::message<FloofMsgHeaders> msg;
            msg.header.id = FloofMsgHeaders::ServerGetPing;

            auto timeNow = std::chrono::system_clock::now();
            msg << timeNow;

            MessageAllClients(msg);
        }

        virtual void OnClientValidated(std::shared_ptr<olc::net::connection<FloofMsgHeaders>> client) override {
            std::cout << "Added Client [" << client->GetID() << "]" << std::endl;

            olc::net::message<FloofMsgHeaders> msg;
            msg.header.id = FloofMsgHeaders::ClientAccepted;
            MessageClient(client, msg);
        }

    protected:
        virtual void OnClientDisconnect(std::shared_ptr<olc::net::connection<FloofMsgHeaders>> client) override {
            std::cout << "Removing Client [" << client->GetID() << "]" << std::endl;

        }

        virtual void OnMessage(std::shared_ptr<olc::net::connection<FloofMsgHeaders>> client,
                               olc::net::message<FloofMsgHeaders> &msg) override {
            //std::cout << "OnMessage call " << msg << std::endl;
            switch (msg.header.id) {
                case FloofMsgHeaders::ServerGetStatus:
                    break;
                case FloofMsgHeaders::ServerGetPing: {
                    MessageClient(client, msg);
                    break;
                }
                case FloofMsgHeaders::ClientAccepted:
                    break;

                case FloofMsgHeaders::ClientRegisterWithServer: {
                    PlayerData data;
                    msg >> data;
                    data.id = client->GetID();

                    olc::net::message<FloofMsgHeaders> sendMsg;
                    sendMsg.header.id = FloofMsgHeaders::ClientAssignID;
                    sendMsg << data;
                    MessageClient(client, sendMsg);

                    sendMsg.header.id = FloofMsgHeaders::GameAddPlayer;
                    sendMsg.body.clear();
                    sendMsg << data;
                    MessageAllClients(sendMsg);

                    //add old players
                    auto v = mScene->GetRegistry().view<PlayerControllerComponent, TransformComponent>();
                    for(auto[ent, player, trans]: v.each()){
                        if(player.mPlayer != client->GetID()){
                            data.Transform = trans;
                            sendMsg.body.clear();
                            sendMsg << data;
                            MessageClient(client,sendMsg);
                        }
                    }

                    MarkedPlayerForInitialize.emplace_back(client); // todo find better sollution for server representation
                    break;
                }
                case FloofMsgHeaders::ClientUnRegisterWithServer:
                    break;
                case FloofMsgHeaders::ClientAssignID:
                    break;
                case FloofMsgHeaders::GameAddPlayer:
                    break;
                case FloofMsgHeaders::GameRemovePlayer:
                    break;
                case FloofMsgHeaders::GameUpdatePlayer:{
                    MessageAllClients(msg,client);

                    //update server
                    CarData data;
                    msg >> data;
                    auto v = mScene->GetRegistry().view<PlayerControllerComponent, NativeScriptComponent>();
                    for (auto [ent, player, script]: v.each()) {
                        if (player.mPlayer == client->GetID()) {
                            auto car = dynamic_cast<MonsterTruckScript*>(script.Script.get());
                            car->SetTransformData(data);
                        }
                    }
                    break;
                }
                case FloofMsgHeaders::GameUpdateWorld:
                    break;
            }
        }
    };

}

#endif //FLOOF_FLOOFNETWORKING_H
