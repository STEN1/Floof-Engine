#pragma once
#include <chrono>

namespace FLOOF {
    class Timer {
    public:
#define TimePoint  std::chrono::high_resolution_clock::time_point

        Timer()
            : m_Creation{ std::chrono::high_resolution_clock::now() }
            , m_DeltaPoint{ m_Creation } {
        }

        double DeltaFromCreation() {
            return std::chrono::duration<double>(std::chrono::high_resolution_clock::now() - m_Creation).count();
        }

        double Delta() {
            auto now = std::chrono::high_resolution_clock::now();
            double delta = std::chrono::duration<double>(now - m_DeltaPoint).count();
            m_DeltaPoint = now;
            return delta;
        }

        static std::chrono::high_resolution_clock::time_point GetTime() {
            return std::chrono::high_resolution_clock::now();
        };

        static double GetTimeSince(std::chrono::high_resolution_clock::time_point time) {
            std::chrono::high_resolution_clock::time_point currentTime = GetTime();
            std::chrono::duration<double> elapsed_seconds = currentTime - time;
            return elapsed_seconds.count();
        };
        static double Delta(TimePoint t1, TimePoint t2){
            std::chrono::duration<double> elapsed_seconds = t1 - t2;
            return elapsed_seconds.count();
        }

    private:
        std::chrono::high_resolution_clock::time_point m_Creation;
        std::chrono::high_resolution_clock::time_point m_DeltaPoint;
    };
}