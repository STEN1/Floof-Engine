#ifndef FLOOF_CARGAMECLIENT_H
#define FLOOF_CARGAMECLIENT_H

#include <cstdint>
#include "Client.h"

namespace FLOOF{
    enum class CarMsgHeaders : uint32_t{
        drive,
        brake,
        respawn,
        lights
    };
   class CarClient : public Network::Client<CarMsgHeaders>{
   public:
       bool brake(bool brake){
           Network::message<CarMsgHeaders> msg;
           msg.header.id = CarMsgHeaders::brake;

           msg << brake;

           //send(msg);
       };
   };

}

#endif //FLOOF_CARGAMECLIENT_H
