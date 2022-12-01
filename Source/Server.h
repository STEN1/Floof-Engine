#ifndef FLOOF_SERVER_H
#define FLOOF_SERVER_H

#define ASIO_STANDALONE

#include "asio.hpp"
#include "asio/ts/buffer.hpp"
#include "asio/ts/internet.hpp"

namespace FLOOF{
    class Server {
    public:
        Server();
        ~Server();

        void listen();

        void handleData(asio::ip::tcp::socket& socket);

        asio::error_code  ec; // save errorcodes

        asio::io_context context;

        asio::ip::tcp::socket socket{context};

    private:
        asio::ip::tcp::endpoint endpoint;

        std::vector<char> vBuffer; //buffer to read data
    };
}



#endif //FLOOF_SERVER_H
