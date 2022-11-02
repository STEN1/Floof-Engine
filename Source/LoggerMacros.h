//
// Created by Adrian Drevland on 08/09/2022.
//

#ifndef FLOOF_LOGGERMACROS_H
#define FLOOF_LOGGERMACROS_H
#include "Input.h"

#define LOG_WARNING(message) FLOOF::Utils::Logger::s_Logger->log(FLOOF::Utils::Logger::LogType::WARNING,message);
#define LOG_ERROR(message) FLOOF::Utils::Logger::s_Logger->log(FLOOF::Utils::Logger::LogType::ERROR,message);
#define LOG_CRITICAL(message) FLOOF::Utils::Logger::s_Logger->log(FLOOF::Utils::Logger::LogType::CRITICAL,message);
#define LOG_INFO(message) FLOOF::Utils::Logger::s_Logger->log(FLOOF::Utils::Logger::LogType::INFO,message);
#define LOG_VEC(message, vec) FLOOF::Utils::Logger::s_Logger->log(FLOOF::Utils::Logger::LogType::INFO, message, vec);

#endif //FLOOF_LOGGERMACROS_H
