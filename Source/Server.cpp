#include "Server.h"
#include <iostream>

FLOOF::Server::Server() {

    vBuffer.resize(20 * 1024);

    endpoint = asio::ip::tcp::endpoint(asio::ip::make_address("127.0.0.1", ec), 80); //todo find ip and set to host ip

    socket.connect(endpoint, ec);

    if (!ec) {
        std::cout << "Socket connected" << std::endl;
    } else {
        std::cout << "Failed to conned to address : " << std::endl << ec.message() << std::endl;
    }


}

FLOOF::Server::~Server() {

}

void FLOOF::Server::listen() {

    if (socket.is_open()) {


    }

}

void FLOOF::Server::handleData(asio::ip::tcp::socket &socket) {

    socket.async_read_some(asio::buffer(vBuffer.data(), vBuffer.size()),
                           [&](std::error_code ec, std::size_t length) {
                               if (!ec) {
                                   //handle data when ariving
                                   std::cout << "Reading " << length << "bytes" << std::endl;
                                   for(int i =0; i < length; i++){
                                       std::cout << vBuffer[i];
                                   }
                                   std::cout << std::endl;
                                   handleData(socket);
                               }
                           });

}
