#ifndef FLOOF_CONNECTION_H
#define FLOOF_CONNECTION_H

#include "asio.hpp"
#include "Message.h"
#include "Queue.h"

namespace FLOOF::Network {
    template<typename T>
    class Connection : public std::enable_shared_from_this<Connection<T>> {
    public:
        Connection(){

        }
        virtual ~Connection(){

        }

        bool ConnectToServer(){

        }
        bool Disconnect(){

        }
        bool IsConnected()const{

        }


        bool Send(const message<T>&msg){

        }
    private:
        asio::ip::tcp::socket  mSocket;

        asio::io_context& mContext;

        Queue<message<T>> mQMessageOut;

        Queue<ownedMessage<T>>& mQMessageIn;

    };
}


#endif //FLOOF_CONNECTION_H
