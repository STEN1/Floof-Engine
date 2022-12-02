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
                }
            }
        }

        void ConnectToServer() {

        }

        bool Disconnect() {
            return false;
        }

        bool IsConnected() const {
            return mSocket.is_open();
        }


        bool Send(const message<T> &msg) {

        }

        uint32_t GetID() const {
            return id;
        }

    private:
        asio::ip::tcp::socket mSocket;

        asio::io_context &mContext;

        Queue<message<T>> mQMessageOut;

        Queue<ownedMessage<T>> &mQMessageIn;
        owner mOwnerType;
        uint32_t id = 0;

    };
}


#endif //FLOOF_CONNECTION_H
