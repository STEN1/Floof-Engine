#ifndef FLOOF_SERVER_H
#define FLOOF_SERVER_H

#define ASIO_STANDALONE

#include "asio.hpp"
#include "asio/ts/buffer.hpp"
#include "asio/ts/internet.hpp"
#include "Queue.h"
#include "Connection.h"
#include <memory>
#include <iostream>

namespace FLOOF::Network {
    template<typename T>
    class Server {
    public:
        Server(uint16_t port)
                : mAcceptor(mContext, asio::ip::tcp::endpoint(asio::ip::tcp::v4(), port)) {


        };

        virtual ~Server() {
            Stop();
        };

        bool Start() {
            try {

                WaitForClientConnection();

                mThreadContext = std::thread([this]() { mContext.run(); });

            }
            catch (std::exception &e) {
                std::cerr << "[Server] Exception: " << e.what() << std::endl;
                return false;
            }

            std::cout << "[Server] Started" << std::endl;
            return true;
        }

        void Stop() {
            mContext.stop();

            if (mThreadContext.joinable())
                mThreadContext.join();

            std::cout << "[Server] Stoped" << std::endl;
        }

        //ASYNC - make asio wait for connection
        void WaitForClientConnection() {

            mAcceptor.async_accept(
                    [this](std::error_code ec, asio::ip::tcp::socket socket) {
                        if (!ec) {
                            std::cout << "[Server] New Connection: " << socket.remote_endpoint() << std::endl;

                            std::shared_ptr<Connection<T>> Conn =
                                    std::make_shared<Connection<T>>(Connection<T>::owner::server, mContext, std::move(socket), mQMessageIn);

                            if(OnClientConnect(Conn)){
                                mQConnections.push_back(std::move(Conn));
                                mQConnections.back()->ConnectToClient(IDCounter++);
                                std::cout << "[" << mQConnections.back()->GetID() << "]"<< " Connection Approved" << std::endl;
                            }
                            else{
                                std::cout << "[Server] Connection Denied" << std::endl;
                            }

                        } else {
                            std::cerr << "[Server] New Connection Error: " << ec.message() << std::endl;
                        }
                        WaitForClientConnection();
                    });

        }

        void MessageClient(std::shared_ptr<Connection<T>> client, const message<T> &msg) {

            if(client && client->IsConnected()){
                client->Send(msg);
            }
            else{
                OnClientDisconnect(client);
                client.reset();
                mQConnections.erase(std::remove(mQConnections.begin(), mQConnections.end(), client),mQConnections.end());
            }

        }

        void MessageAllClients(const message<T> &msg, std::shared_ptr<Connection<T>> IgnoreClient = nullptr) {
            bool InvalidClientFound{false};
            for(auto& client : mQConnections){
                if(client && client->IsConnected()){
                    if(client != IgnoreClient)
                        client->Send(msg);
                }
                else{
                    OnClientDisconnect(client);
                    client.reset();
                    InvalidClientFound = true;
                }
            }
            if(InvalidClientFound)
                mQConnections.erase(std::remove(mQConnections.begin(), mQConnections.end(), nullptr),mQConnections.end());

        }

        void Update(size_t MaxMessages = -1){
            size_t Messagecount = 0;
            while(Messagecount < MaxMessages && !mQMessageIn.empty()){
                auto msg = mQMessageIn.pop_front();

                OnMessage(msg.remote,msg.msg);

                Messagecount++;
            }
        }

        size_t ConnectionCount()const{
            return mQConnections.size();
        }
    protected:

        virtual bool OnClientConnect(std::shared_ptr<Connection<T>> client) {

            return false;
        }

        virtual void OnClientDisconnect(std::shared_ptr<Connection<T>> client) {

        }

        virtual void OnMessage(std::shared_ptr<Connection<T>> client, message<T> &msg) {

        }

        Queue<ownedMessage<T>> mQMessageIn;

        std::deque<std::shared_ptr<Connection<T>>> mQConnections;

        asio::io_context mContext;
        std::thread mThreadContext;

        asio::ip::tcp::acceptor mAcceptor;

        uint32_t IDCounter{1000}; // identifie clients

    };
}


#endif //FLOOF_SERVER_H
