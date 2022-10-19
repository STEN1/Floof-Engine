#pragma once
#include "glm/glm.hpp"
#include <ostream>

namespace FLOOF {
    namespace Utils {
        class Logger {
        public:
            inline static Logger* s_Logger = nullptr;
            Logger(const char* logfile);
            ~Logger();
            enum LogType {
                INFO = 0,
                WARNING,
                ERROR,
                CRITICAL,
            };

            void log(LogType logtype, const char* message);
            void log(LogType logtype, const glm::vec3 vec);
            void log(LogType logtype, const char* message, const glm::vec3 vec);
        private:
            const char* m_LogPath;
        };
    }
}

std::ostream& operator << (std::ostream& out, const glm::vec3& v);