#ifndef FLOOF_CONNECTION_H
#define FLOOF_CONNECTION_H

#include "asio.hpp"
#include "Message.h"
#include "Queue.h"

namespace FLOOF::Network {
    template<typename T>
    class Connection : public std::enable_shared_from_this<Connection<T>> {
    public:
        enum class owner {
            server,
            client
        };

        Connection(owner parent, asio::io_context &context, asio::ip::tcp::socket socket, Queue<ownedMessage<T>> &qIn) :
                mContext(context), mQMessageIn(qIn), mSocket(std::move(socket)), mOwnerType(parent) {

        }

        virtual ~Connection() {

        }

        void ConnectToClient(uint32_t uid = 0) {
            if (mOwnerType == owner::server) {
                if (mSocket.is_open()) {
                    id = uid;
                    ReadHeader();
                }
            }
        }

        void ConnectToServer(const asio::ip::tcp::resolver::results_type &endpoints) {
            if (mOwnerType == owner::client) {
                asio::async_connect(mSocket, endpoints,
                                    [this](std::error_code ec, asio::ip::tcp::endpoint endpoint) {
                                        if(!ec){
                                            ReadHeader();
                                        }
                                    });
            }
        }

        bool Disconnect() {
            asio::post(mContext, [this]() { mSocket.close(); });
        }

        bool IsConnected() const {
            return mSocket.is_open();
        }


        bool Send(const message<T> &msg) {
            asio::post(mContext,
                       [this, msg]() {
                           bool writing = !mQMessageOut.empty();
                           mQMessageOut.push_back(msg);
                           if (!writing)
                               WriteHeader();
                       });
        }

        uint32_t GetID() const {
            return id;
        }

    private:
        asio::ip::tcp::socket mSocket;

        asio::io_context &mContext;

        Queue<message<T>> mQMessageOut;

        Queue<ownedMessage<T>> &mQMessageIn;
        message<T> tmpMessage;

        owner mOwnerType;
        uint32_t id = 0;


        void ReadHeader() {
            asio::async_read(mSocket, asio::buffer(&tmpMessage.header, sizeof(messageHeader<T>)),
                             [this](std::error_code ec, std::size_t length) {
                                 if (!ec) {
                                     if (tmpMessage.header.size > 0) {
                                         tmpMessage.body.resize(tmpMessage.header.size);
                                         ReadBody();
                                     } else {
                                         AddToMsgInQueue();
                                     }

                                 } else {
                                     std::cerr << "[" << id << "] Read Header Fail" << std::endl;
                                     mSocket.close();
                                 }

                             });

        }

        void ReadBody() {
            asio::async_read(mSocket, asio::buffer(tmpMessage.body.data(), tmpMessage.body.size()),
                             [this](std::error_code ec, std::size_t length) {
                                 if (!ec) {
                                     AddToMsgInQueue();
                                 } else {
                                     std::cerr << "[" << id << "] Read Body Fail" << std::endl;
                                     mSocket.close();
                                 }

                             });

        }

        void WriteHeader() {
            asio::async_write(mSocket, asio::buffer(&mQMessageOut.front().header, sizeof(messageHeader<T>)),
                              [this](std::error_code ec, std::size_t length) {
                                  if (!ec) {
                                      if (mQMessageOut.front().body.size() > 0) {
                                          WriteBody();
                                      } else {
                                          mQMessageOut.pop_front();
                                          if (!mQMessageOut.empty()) {
                                              WriteHeader();
                                          }
                                      }
                                  } else {
                                      std::cerr << "[" << id << "] Write Header Fail" << std::endl;
                                      mSocket.close();
                                  }
                              });
        }

        void WriteBody() {
            asio::async_write(mSocket, asio::buffer(mQMessageOut.front().body.data(), mQMessageOut.front().body.size()),
                              [this](std::error_code ec, std::size_t length) {
                                  if (!ec) {
                                      mQMessageOut.pop_front();
                                      if (!mQMessageOut.empty()) {
                                          WriteHeader();
                                      }
                                  } else {
                                      std::cerr << "[" << id << "] Write Body Fail" << std::endl;
                                      mSocket.close();

                                  }
                              });
        }

        void AddToMsgInQueue() {
            if (mOwnerType == owner::server) {
                mQMessageIn.push_back({this->shared_from_this(), tmpMessage});
            } else {
                mQMessageIn.push_back({nullptr, tmpMessage});
            }
            ReadHeader();
        }
    };
}


#endif //FLOOF_CONNECTION_H
