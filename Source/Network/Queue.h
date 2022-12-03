#ifndef FLOOF_QUEUE_H
#define FLOOF_QUEUE_H

#include "asio.hpp"
#include <deque>
#include <mutex>

//threadsafe queue since asio is asyncronised
namespace FLOOF::Network {
    template<typename T>
    class Queue {
    public:
        Queue() = default;
        Queue(const Queue<T>& ) = delete; // cant bo copied. becuase of mutex
        virtual ~Queue(){clear();};

        const T& front(){
            std::scoped_lock lock(muxQueue);
            return deqQueue.front();
        }
        const T& back(){
            std::scoped_lock lock(muxQueue);
            return deqQueue.back();
        }
        void push_back(const T& item){
            std::scoped_lock lock(muxQueue);
            deqQueue.emplace_back(std::move(item));
        }
        void push_front(const T& item){
            std::scoped_lock lock(muxQueue);
            deqQueue.emplace_front(std::move(item));
        }
        size_t size(){
            std::scoped_lock lock(muxQueue);
            return deqQueue.size();
        }
        void clear(){
            std::scoped_lock lock(muxQueue);
            deqQueue.clear();
        }
        T pop_front(){
            std::scoped_lock lock(muxQueue);
            auto t = std::move(deqQueue.front());
            deqQueue.pop_front();
            return t;
        }
        T pop_back(){
            std::scoped_lock lock(muxQueue);
            auto t = std::move(deqQueue.back());
            deqQueue.pop_back();
            return t;
        }
        bool empty(){
            std::scoped_lock lock(muxQueue);
            return deqQueue.empty();
        }

    private:
        std::mutex muxQueue;
        std::deque<T> deqQueue;
    };


}


#endif //FLOOF_QUEUE_H
