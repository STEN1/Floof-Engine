#ifndef FLOOF_FLOOFNETWORKING_H
#define FLOOF_FLOOFNETWORKING_H

#include <cstdint>
#include "Client.h"
#include "Server.h"
#include <chrono>
#include "../Scene.h"
#include "../NativeScripts/CarBaseScript.h"
#include "OlcNet.h"
#include "../NativeScripts/CarScripts/RaceCarScript.h"
#include "../NativeScripts/CarScripts/MonsterTruckScript.h"

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
        GameUpdateWorld,

        ServerStartPlay
    };

    struct Tform{
        glm::vec3 Position = glm::vec3(0.f);
        glm::vec3 Rotation = glm::vec3(0.f);
        glm::vec3 Scale = glm::vec3(1.f);
    };

    struct CarData {

        Tform MainTform;
        Tform WheelTformFL;
        Tform WheelTformFR;
        Tform WheelTformBL;
        Tform WheelTformBR;

        btVector3 AvFrame = btVector3(0,0,0);
        btVector3 AvWheelFR = btVector3(0,0,0);
        btVector3 AvWheelFL = btVector3(0,0,0);
        btVector3 AvWheelBR = btVector3(0,0,0);
        btVector3 AvWheelBL = btVector3(0,0,0);

        uint32_t id{0};
        uint32_t CarType{0};
    };
    struct LevelData {
        Scene scene;
    };

    class FloofClient : public olc::net::client_interface<FloofMsgHeaders> {
    public:
        FloofClient() : client_interface() {

        };

        bool Initialized{false};
        bool Play{false};

        void PingServer() {
            olc::net::message<FloofMsgHeaders> msg;
            msg.header.id = FloofMsgHeaders::ServerGetPing;

            auto timeNow = std::chrono::system_clock::now();
            msg << timeNow;

            Send(msg);
        }

        void Update(FLOOF::Scene *scene) {

            while (!Incoming().empty()) {
                auto msg = Incoming().pop_front().msg;

                switch (msg.header.id) {
                    case FloofMsgHeaders::ServerGetStatus:
                        break;
                    case FloofMsgHeaders::ServerGetPing: {
                        auto timeNow = std::chrono::system_clock::now();
                        std::chrono::system_clock::time_point timeThen;
                        msg >> timeThen;
                        scene->ping = std::chrono::duration<double>(timeNow - timeThen).count();
                        break;
                    }
                    case FloofMsgHeaders::ClientAccepted: {
                        Initialized = true;
                        olc::net::message<FloofMsgHeaders> sendMsg;
                        sendMsg.header.id = FloofMsgHeaders::ClientRegisterWithServer;
                        CarData data;

                        auto v = scene->GetRegistry().view<PlayerControllerComponent, NativeScriptComponent>();
                        for (auto [ent, player, script]: v.each()) {
                            if (player.mPlayer == scene->ActivePlayer) {
                                auto car = dynamic_cast<CarBaseScript *>(script.Script.get());
                                data = car->GetTransformData();
                            }
                        }
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

                    case FloofMsgHeaders::ClientAssignID: {
                        CarData data;
                        msg >> data;
                        auto v = scene->GetRegistry().view<PlayerControllerComponent>();
                        for (auto [ent, player]: v.each()) {
                            if (player.mPlayer == scene->ActivePlayer) {
                                player.mPlayer = data.id;
                                scene->ActivePlayer = data.id;
                            }
                        }

                        break;
                    }
                    case FloofMsgHeaders::GameAddPlayer: {
                        CarData data;
                        msg >> data;
                        std::string Player = "Player : ";
                        Player += std::to_string(data.id);
                        auto ent = scene->CreateEntity(Player);

                        switch (data.CarType) {
                            case 1 : //racecar
                                scene->AddComponent<NativeScriptComponent>(ent, std::make_unique<RaceCarScript>(glm::vec3(0.f)), scene, ent);
                                break;
                            case 2: //cybertruck
                                scene->AddComponent<NativeScriptComponent>(ent, std::make_unique<MonsterTruckScript>(glm::vec3(0.f)), scene, ent);
                                break;
                        }
                        scene->AddComponent<PlayerControllerComponent>(ent, data.id);
                        auto script = scene->GetComponent<NativeScriptComponent>(ent).Script.get();
                        auto car = dynamic_cast<CarBaseScript *>(script);
                        car->SetTransformData(data);
                        car->AddToPhysicsWorld();
                        break;
                    }
                    case FloofMsgHeaders::GameRemovePlayer:
                        break;
                    case FloofMsgHeaders::GameUpdatePlayer: {
                        CarData data;
                        if(msg.size() == 0) break; // no message just break
                        msg >> data;
                        auto v = scene->GetRegistry().view<PlayerControllerComponent, NativeScriptComponent>();
                        for (auto [ent, player, script]: v.each()) {
                            if (player.mPlayer == data.id) {
                                auto car = dynamic_cast<CarBaseScript *>(script.Script.get());
                                car->SetTransformData(data);
                                std::cout << "updating Player " << std::to_string(data.id) << std::endl;
                            }
                        }
                        break;
                    }
                    case FloofMsgHeaders::GameUpdateWorld:
                        break;
                    case FloofMsgHeaders::ServerStartPlay: {
                        msg >> Play;
                    }
                }

            }

        }

    };

    class FloofServer : public olc::net::server_interface<FloofMsgHeaders> {
    public:
        FloofServer(uint16_t port) : olc::net::server_interface<FloofMsgHeaders>(port) {

        };
        Scene *mScene;
    protected:
        virtual bool OnClientConnect(std::shared_ptr<olc::net::connection<FloofMsgHeaders>> client) override {
            return true;
        }

    public:
        std::vector<std::pair<std::shared_ptr<olc::net::connection<FloofMsgHeaders>>,uint32_t>> ActivePlayers;
        std::vector<uint32_t> MarkedPlayerForRemove;
        std::vector<std::pair<std::shared_ptr<olc::net::connection<FloofMsgHeaders>>,uint32_t>> MarkedPlayerForInitialize;

        size_t ConnectionCount() {
            return ActivePlayers.size();
        }

        void StartPressed(bool Start) {
            olc::net::message<FloofMsgHeaders> msg;
            msg.header.id = FloofMsgHeaders::ServerStartPlay;

            msg << Start;

            MessageAllClients(msg);
        };

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
            MarkedPlayerForRemove.emplace_back(client->GetID());

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
                    CarData data;
                    msg >> data;
                    data.id = client->GetID();

                    MarkedPlayerForInitialize.emplace_back(client, data.CarType);

                    olc::net::message<FloofMsgHeaders> sendMsg;
                    sendMsg.header.id = FloofMsgHeaders::ClientAssignID;
                    sendMsg << data;
                    MessageClient(client, sendMsg);

                    //add old players
                    CarData cData;
                    sendMsg.header.id = FloofMsgHeaders::GameAddPlayer;

                    auto v = mScene->GetRegistry().view<PlayerControllerComponent, NativeScriptComponent>();
                    for (auto [ent, player, script]: v.each()) {
                        if (player.mPlayer == mScene->ActivePlayer) {
                                player.mPlayer = 1;
                                mScene->ActivePlayer = 1;
                        }
                        auto car = dynamic_cast<CarBaseScript *>(script.Script.get());
                        cData = car->GetTransformData();
                        cData.id = player.mPlayer;
                        sendMsg.body.clear();
                        sendMsg << cData;
                        MessageClient(client,sendMsg);
                    }

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
                case FloofMsgHeaders::GameUpdatePlayer: {
                    //update server
                    CarData data;
                    msg >> data;
                    //data.id = client->GetID();
                    auto v = mScene->GetRegistry().view<PlayerControllerComponent, NativeScriptComponent>();
                    for (auto [ent, player, script]: v.each()) {
                        if (player.mPlayer == client->GetID()) {
                            auto car = dynamic_cast<CarBaseScript *>(script.Script.get());
                            car->SetTransformData(data);
                        }
                    }
                    msg.body.clear();
                    msg << data;
                    MessageAllClients(msg, client);
                    break;
                }
                case FloofMsgHeaders::GameUpdateWorld:
                    break;
            }
        }
    };

}

#endif //FLOOF_FLOOFNETWORKING_H
