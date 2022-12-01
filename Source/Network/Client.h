#ifndef FLOOF_CLIENT_H
#define FLOOF_CLIENT_H

#include "asio.hpp"
#include "Message.h"
#include "Connection.h"
#include <iostream>

namespace FLOOF::Network {
    template<typename T>
    class Client {
    public:
        Client(): mSocket(mContext) {

        };

        ~Client() {
            Disconnect();
        };

        bool Connect(const std::string& host, const uint16_t port){

            try{
               mConnection = std::make_unique<Connection<T>>();
               asio::ip::tcp::resolver  resolver(mContext);
               auto endpoints = resolver.resolve(host,std::to_string(port));

               mConnection->ConnectToServer(endpoints);

               threadContext = std::thread([this](){mContext.run();});

            }
            catch (std::exception& e){
                std::cerr << "Client Exception " << e.what() << std::endl;
                return false;
            }

        }

        void Disconnect(){
            if(IsConnected())
                mConnection->Disconnect();
            mContext.stop();
            if(threadContext.joinable())
                threadContext.join();

            mConnection.release();
        }

        bool IsConnected(){
            if(mConnection)
                return mConnection->IsConnected();
            else
                return false;
        }


       Queue<ownedMessage<T>> Incoming(){
            return mQMessageIn;
        }

    protected:

        asio::io_context mContext;
        std::thread threadContext;
        asio::ip::tcp::socket mSocket;

        std::unique_ptr<Connection<T>> mConnection;

        Queue<ownedMessage<T>> mQMessageIn;
    };
}


#endif //FLOOF_CLIENT_H
