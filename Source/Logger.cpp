//
// Created by Adrian Drevland on 08/09/2022.
//
#include <iostream>
#include <fstream>
#include <ctime>
#include <chrono>
#include "Logger.h"
#include <sys/stat.h>
#include <filesystem>
#include "LoggerMacros.h"

namespace FLOOF {
    Utils::Logger::Logger(const char* logfile) :m_LogPath(logfile) {
        //Creating File
        struct stat buffer;
        if (stat("Logs", &buffer) != 0) {
            std::filesystem::create_directory("Logs");
        }
        //clear file when starting program
        std::string l{ "Logs/" };
        l.append(logfile);
        std::ofstream clearfile(l, std::ios::trunc);
        //time
        std::time_t now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
        std::string s(30, '\0');
        std::strftime(&s[0], s.size(), "%Y-%m-%d %H:%M:%S", std::localtime(&now));
        clearfile << s << "\n";
        clearfile.close();

    }

    Utils::Logger::~Logger() {

    }
    void Utils::Logger::log(Utils::Logger::LogType logtype, const char* message) {
        std::string output;

        //time
        std::time_t now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
        std::string s(30, '\0');
        // std::strftime(&s[0], s.size(), "%Y-%m-%d %H:%M:%S", std::localtime(&now));
        std::strftime(&s[0], s.size(), "%H:%M:%S", std::localtime(&now));
        output.append(s);
        //logtype
        switch (logtype) {
        case LogType::WARNING:
            output.append(" Warning");
            break;
        case LogType::ERROR:
            output.append(" Error\t");
            break;
        case LogType::CRITICAL:
            output.append(" Critical");
            break;
        case LogType::INFO:
            output.append(" Info\t");
            break;
        }
        output.append("\t");
        output.append(message);

        std::string logpath = "Logs/";
        logpath.append(m_LogPath);
        std::ofstream stream(logpath, std::ios::out | std::ios::app);
        if (stream.is_open()) {
            stream << output << "\n";
        }
        std::cout << output << "\n";
        stream.close();
    }

    void Utils::Logger::log(Utils::Logger::LogType logtype, const glm::vec3 vec) {
        std::string vecstring;
        vecstring += "X : ";
        vecstring += std::to_string(vec.x);
        vecstring += " \t Y : ";
        vecstring += std::to_string(vec.y);
        vecstring += "\t Z : ";
        vecstring += std::to_string(vec.z);

        log(logtype, vecstring.c_str());
    }

    void Utils::Logger::log(Utils::Logger::LogType logtype, const char* message, const glm::vec3 vec) {
        std::string vecstring = message;
        vecstring += "\t X : ";
        vecstring += std::to_string(vec.x);
        vecstring += " \t Y : ";
        vecstring += std::to_string(vec.y);
        vecstring += "\t Z : ";
        vecstring += std::to_string(vec.z);

        log(logtype, vecstring.c_str());
    }
}

std::ostream& operator << (std::ostream& out, const glm::vec3& v) {
    return out << "(x: " << v.x << " y: " << v.y << " z: " << v.z << ")";
}