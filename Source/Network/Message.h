#ifndef FLOOF_MESSAGE_H
#define FLOOF_MESSAGE_H

#include <cstdint>
#include "asio.hpp"

namespace FLOOF::Network{
        template <typename T>
        struct messageHeader{
            T id{};
            uint32_t size = 0;
        };

        template <typename T>
        struct message{
            messageHeader<T> header{};
            std::vector<uint8_t> body;

            size_t size() const{
                return sizeof(messageHeader<T>) + body.size();
            }

            //override std::cout operator << for debugging
            friend std::ostream& operator << (std::ostream& os, const message<T>& msg){
                os << "ID : " << int(msg.header.id) << " Size : " << msg.header.size;
                return os;
            }

            //push data to buffer
            template <typename DataType>
            friend message<T>& operator << (message<T>& msg, DataType& data){

                static_assert(std::is_standard_layout<DataType>::value, "Data is to complex");

               size_t i = msg.body.size(); // cache current size

                msg.body.resize(msg.body.size()+sizeof(DataType));

                std::memcpy(msg.body.data() + i, &data, sizeof(DataType)); // copy data to new sized vector

                msg.header.size = msg.size();

                return msg;
            }

            //Get data from buffer
            template <typename DataType>
            friend message<T>& operator >> (message<T>& msg, DataType& data){

                static_assert(std::is_standard_layout<DataType>::value, "Data is to complex");

                size_t i = msg.body.size() - sizeof(DataType);

                std::memcpy(&data, msg.body.data()+i, sizeof(DataType));

                msg.body.resize(i);
                msg.header.size = msg.size();

                return msg;
            }

        };

        //forward declare
        template<typename  T>
        class connection;

        template <typename T>
        struct ownedMessage{
            std::shared_ptr<connection<T>> remote = nullptr; // owner of message
            message<T> msg;


            //override std::cout operator << for debugging
            friend std::ostream& operator << (std::ostream& os, const ownedMessage<T>& msg){
                os << msg.msg;
                return os;
            }
        };
    }

#endif //FLOOF_MESSAGE_H

