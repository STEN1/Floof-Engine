#ifndef FLOOF_FLOOFNETWORKING_H
#define FLOOF_FLOOFNETWORKING_H

#include <cstdint>
#include "Client.h"
#include "Server.h"
#include <chrono>

namespace FLOOF{
    enum class FloofMsgHeaders : uint32_t{
       ServerAccept,
       ServerDeny,
       ServerPing,
       MessageAll,
       ServerMessage,
       KeyInput
    };
   class FloofClient : public Network::Client<FloofMsgHeaders>{
   public:

       void PingServer(){
           Network::message<FloofMsgHeaders> msg;
           msg.header.id = FloofMsgHeaders::ServerPing;

           auto timeNow = std::chrono::system_clock::now();
           msg << timeNow;

           Send(msg);
       }

       bool Input(int key){
           Network::message<FloofMsgHeaders> msg;
           msg.header.id = FloofMsgHeaders::KeyInput;

           msg << key;
           Send(msg);

       };
   };
   class FloofServer : public Network::Server<FloofMsgHeaders>{
   public:
       FloofServer(uint16_t port): Network::Server<FloofMsgHeaders>(port){

       };
   protected:
       virtual bool OnClientConnect(std::shared_ptr<Network::Connection<FloofMsgHeaders>> client)override{

           return true;
       }
       virtual void OnClientDisconnect(std::shared_ptr<Network::Connection<FloofMsgHeaders>> client) override{

       }

       virtual void OnMessage(std::shared_ptr<Network::Connection<FloofMsgHeaders>> client, Network::message<FloofMsgHeaders> &msg) override{
           switch (msg.header.id) {
               case FloofMsgHeaders::ServerAccept:
                   break;
               case FloofMsgHeaders::ServerDeny:
                   break;
               case FloofMsgHeaders::ServerPing:
                   std::cout << "[" << client->GetID() << "] : Server Ping" <<std::endl;
                   client->Send(msg);
                   break;
               case FloofMsgHeaders::MessageAll:
                   break;
               case FloofMsgHeaders::ServerMessage:
                   break;
               case FloofMsgHeaders::KeyInput:
                   break;
           }
       }
   };

}

#endif //FLOOF_FLOOFNETWORKING_H
