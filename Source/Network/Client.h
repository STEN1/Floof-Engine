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
        Client() : mSocket(mContext) {

        };

        ~Client() {
            Disconnect();
        };

        bool Connect(const std::string &host, const uint16_t port) {

            try {

                asio::ip::tcp::resolver resolver(mContext);
                auto endpoints = resolver.resolve(host, std::to_string(port));

                mConnection = std::make_unique<Connection<T>>(Connection<T>::owner::client,mContext,asio::ip::tcp::socket(mContext),mQMessageIn);

                mConnection->ConnectToServer(endpoints);

                threadContext = std::thread([this]() { mContext.run(); });

            }
            catch (std::exception &e) {
                std::cerr << "Client Exception " << e.what() << std::endl;
                return false;
            }
            return true;
        }

        void Disconnect() {
            if (IsConnected())
                mConnection->Disconnect();
            mContext.stop();
            if (threadContext.joinable())
                threadContext.join();

            mConnection.release();
        }

        bool IsConnected() {
            if (mConnection)
                return mConnection->IsConnected();
            else
                return false;
        }

        void Send(const message<T>& msg){
            if(IsConnected()){
                mConnection->Send(msg);
            }
        }

        Queue<ownedMessage<T>>& Incoming() {
            return mQMessageIn;
        }

        uint32_t GetID(){
            return mConnection->GetID();
        }

        virtual void Update(){if(!IsConnected())return;};
    protected:

        asio::io_context mContext;
        std::thread threadContext;
        asio::ip::tcp::socket mSocket;

        std::unique_ptr<Connection<T>> mConnection;

        Queue<ownedMessage<T>> mQMessageIn;
    };
}


#endif //FLOOF_CLIENT_H
