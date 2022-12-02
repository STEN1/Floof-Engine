#ifndef FLOOF_CARGAMECLIENT_H
#define FLOOF_CARGAMECLIENT_H

#include <cstdint>
#include "Client.h"

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

}

#endif //FLOOF_CARGAMECLIENT_H
