#ifndef FLOOF_FLOOFNETWORKING_H
#define FLOOF_FLOOFNETWORKING_H

#include <cstdint>
#include "Client.h"
#include "Server.h"
#include <chrono>
#include "../Scene.h"
#include "../NativeScripts/MonsterTruckScript.h"

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
    struct LevelData{
        Scene scene;
    };
    class FloofClient : public Network::Client<FloofMsgHeaders> {
    public:
        FloofClient() : Client() {

        };

        void PingServer() {
            Network::message<FloofMsgHeaders> msg;
            msg.header.id = FloofMsgHeaders::ServerGetPing;

            auto timeNow = std::chrono::system_clock::now();
            msg << timeNow;

            Send(msg);
        }

        void Update(Scene* scene){
            Client::Update();

            while (!Incoming().empty()) {
                auto msg = Incoming().pop_front().msg;

                std::cout << "Client recieved " << msg << std::endl;
                switch (msg.header.id) {
                    case FloofMsgHeaders::ServerGetStatus:
                        break;
                    case FloofMsgHeaders::ServerGetPing:{
                        auto timeNow = std::chrono::system_clock::now();
                        std::chrono::system_clock::time_point timeThen;
                        msg >> timeThen;
                        scene->ping = std::chrono::duration<double>(timeNow-timeThen).count();

                        timeNow = std::chrono::system_clock::now();
                        msg.body.clear();
                        msg << timeThen;
                        Send(msg);
                        break;
                    }
                    case FloofMsgHeaders::ClientAccepted: {
                        mConnection->IsValidated = true;
                        Network::message<FloofMsgHeaders> sendMsg;
                        sendMsg.header.id = FloofMsgHeaders::ClientRegisterWithServer;
                        //tood send spawnlocation
                        PlayerData data;
                        data.Transform.Position = glm::vec3(0.f, -40.f, 10.f * (GetID() - 999.f));
                        data.id = GetID();
                        sendMsg << data;
                        Send(sendMsg);

                        PingServer();
                        break;
                    }
                    case FloofMsgHeaders::ClientAssignID:{
                        PlayerData Data;
                        msg >> Data;
                        scene->ActivePlayer = Data.id;

                        Network::message<FloofMsgHeaders> msg;
                        msg.header.id = FloofMsgHeaders::ServerGetPing;

                        auto timeNow = std::chrono::system_clock::now();
                        msg << timeNow;

                        Send(msg);
                        break;
                    }

                    case FloofMsgHeaders::ClientRegisterWithServer:
                        break;
                    case FloofMsgHeaders::ClientUnRegisterWithServer:
                        break;
                    case FloofMsgHeaders::GameAddPlayer: {
                       PlayerData data;
                        msg >> data;
                        std::string Player = "Player : ";
                        Player += std::to_string(data.id);
                        auto ent = scene->CreateEntity(Player);
                        scene->AddComponent<NativeScriptComponent>(ent, std::make_unique<MonsterTruckScript>(data.Transform.Position),scene, ent);
                        scene->AddComponent<PlayerControllerComponent>(ent, data.id);
                        break;
                    }
                    case FloofMsgHeaders::GameRemovePlayer:
                        //todo cant remove players yet
                        break;
                    case FloofMsgHeaders::GameUpdatePlayer:{
                        PlayerData data;
                        msg >> data;
                        auto v = scene->GetRegistry().view<PlayerControllerComponent,TransformComponent>();
                        for(auto[ent, player,tform]: v.each()){
                            if(player.mPlayer == data.id){
                                tform = data.Transform;
                            }
                        }
                        break;
                    }
                    case FloofMsgHeaders::GameUpdateWorld:
                        break;
                }

            }


        }

    };

    class FloofServer : public Network::Server<FloofMsgHeaders> {
    public:
        FloofServer(uint16_t port) : Network::Server<FloofMsgHeaders>(port) {

        };
        Scene * mScene;
        //int ActivePlayers{1};
        std::vector<std::shared_ptr<Network::Connection<FloofMsgHeaders>>> ActivePlayers;
        std::vector<uint32_t> MarkedPlayerForRemove;
        std::vector<std::shared_ptr<Network::Connection<FloofMsgHeaders>>> MarkedPlayerForInitialize;
        //int InitializedPlayers{1};
    protected:
        virtual bool OnClientConnect(std::shared_ptr<Network::Connection<FloofMsgHeaders>> client) override {

            //make Player
            //MarkedPlayerForInitialize.emplace_back(client);

            return true;
        }
    public:
        void PingClients() {
            Network::message<FloofMsgHeaders> msg;
            msg.header.id = FloofMsgHeaders::ServerGetPing;

            auto timeNow = std::chrono::system_clock::now();
            msg << timeNow;

            MessageAllClients(msg);
        }
        virtual void OnClientValidated(std::shared_ptr<Network::Connection<FloofMsgHeaders>> client) override {
            Server::OnClientValidated(client);

            std::cout << "Added Client [" << client->GetID() << "]" << std::endl;

            Network::message<FloofMsgHeaders> msg;
            msg.header.id = FloofMsgHeaders::ClientAccepted;
            MessageClient(client, msg);
        }
    protected:
        virtual void OnClientDisconnect(std::shared_ptr<Network::Connection<FloofMsgHeaders>> client) override {
            Server::OnClientConnect(client);

            std::cout << "Removing Client [" << client->GetID() << "]" << std::endl;

            //remove player
            //MarkedPlayerForRemove.emplace_back(client->GetID());
        }

        virtual void OnMessage(std::shared_ptr<Network::Connection<FloofMsgHeaders>> client,
                               Network::message<FloofMsgHeaders> &msg) override {
            std::cout << "OnMessage call " << msg << std::endl;
            switch (msg.header.id) {
                case FloofMsgHeaders::ServerGetStatus:
                    break;
                case FloofMsgHeaders::ServerGetPing:{
                    MessageClient(client, msg);
                    break;
                }
                case FloofMsgHeaders::ClientAccepted:
                    break;
                case FloofMsgHeaders::ClientAssignID:
                    break;
                case FloofMsgHeaders::ClientRegisterWithServer:{
                    PlayerData data;
                    msg >> data;
                    data.id = client->GetID();

                    Network::message<FloofMsgHeaders> sendMsg;
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
                case FloofMsgHeaders::GameAddPlayer:
                    break;
                case FloofMsgHeaders::GameRemovePlayer:
                    break;
                case FloofMsgHeaders::GameUpdatePlayer:{
                    MessageAllClients(msg,client);
                    break;
                }
                case FloofMsgHeaders::GameUpdateWorld:
                    break;
            }
        }
    };

}

#endif //FLOOF_FLOOFNETWORKING_H
