/**============================================================================
Name        : Utilities.cpp
Created on  : 18.01.2025
Author      : Andrei Tokmakov
Version     : 1.0
Copyright   : Your copyright notice
Description : Utilities.cpp
============================================================================**/

#include "Utilities.h"

#include <chrono>

namespace Utilities
{
    std::string getCurrentTime() noexcept
    {
        const std::chrono::time_point now { std::chrono::system_clock::now() };
        const time_t in_time_t { std::chrono::system_clock::to_time_t(now) };
        const std::chrono::duration nowMs = std::chrono::duration_cast<std::chrono::microseconds>(
                now.time_since_epoch()) % 1000000;
        std::stringstream ss;
        ss << std::put_time(std::localtime(&in_time_t), "%a %b %d %Y %T")
           << '.' << std::setfill('0') << std::setw(6) << nowMs.count();
        return ss.str();
    }
}