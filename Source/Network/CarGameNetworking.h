#ifndef FLOOF_CARGAMENETWORKING_H
#define FLOOF_CARGAMENETWORKING_H

#include <cstdint>
#include "Client.h"
#include "Server.h"

namespace FLOOF{
    enum class CarMsgHeaders : uint32_t{
        drive,
        brake,
        respawn,
        lights,
        input
    };
   class CarClient : public Network::Client<CarMsgHeaders>{
   public:
       bool brake(bool brake){
           Network::message<CarMsgHeaders> msg;
           msg.header.id = CarMsgHeaders::brake;

           msg << brake;

           //send(msg);
       };
       bool Input(int key){
           Network::message<CarMsgHeaders> msg;
           msg.header.id = CarMsgHeaders::input;

           msg << key;

       };
   };
   class CarServer : public Network::Server<CarMsgHeaders>{
   public:
       CarServer(uint16_t port): Network::Server<CarMsgHeaders>(port){

       };
   protected:
       virtual bool OnClientConnect(std::shared_ptr<Network::Connection<CarMsgHeaders>> client)override{

           return true;
       }
       virtual void OnClientDisconnect(std::shared_ptr<Network::Connection<CarMsgHeaders>> client) override{

       }

       virtual void OnMessage(std::shared_ptr<Network::Connection<CarMsgHeaders>> client, Network::message<CarMsgHeaders> &msg) override{

       }
   };

}

#endif //FLOOF_CARGAMENETWORKING_H
