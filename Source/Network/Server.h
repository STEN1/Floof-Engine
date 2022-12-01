#ifndef FLOOF_SERVER_H
#define FLOOF_SERVER_H

#define ASIO_STANDALONE

#include "asio.hpp"
#include "asio/ts/buffer.hpp"
#include "asio/ts/internet.hpp"

namespace FLOOF::Network{
    class Server {
    public:
        Server();
        ~Server();

        void listen();

        void handleData(asio::ip::tcp::socket& socket);

        asio::error_code  ec; // save errorcodes

        asio::io_context context;

        std::thread threadContext;

        asio::ip::tcp::socket socket{context};

    private:
        asio::ip::tcp::endpoint endpoint;

        std::vector<char> vBuffer; //buffer to read data

        //fake work
        asio::io_context::work idleWork{context};
    };
}



#endif //FLOOF_SERVER_H
