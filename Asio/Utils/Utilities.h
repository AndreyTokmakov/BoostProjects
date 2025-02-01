/**============================================================================
Name        : Utilities.h
Created on  : 28.01.2025
Author      : Andrei Tokmakov
Version     : 1.0
Copyright   : Your copyright notice
Description : Utilities.h
============================================================================**/

#ifndef BOOSTPROJECTS_UTILITIES_H
#define BOOSTPROJECTS_UTILITIES_H

#include <string>
#include <chrono>
#include <syncstream>

namespace Utilities
{
    [[nodiscard]]
    std::string getCurrentTime(const std::chrono::time_point<std::chrono::system_clock>& timestamp =
            std::chrono::system_clock::now()) noexcept;

#define DEBUG  std::osyncstream { std::cout } << getCurrentTime() << " "
#define ERROR  std::osyncstream { std::cerr } << getCurrentTime() << " "
#define FUNC_ENTER  std::osyncstream { std::cout } << getCurrentTime() << " " << __FUNCTION__ << "() entered\n";
#define FUNC_EXIT   std::osyncstream { std::cout } << getCurrentTime() << " " << __FUNCTION__ << "() exited\n";
}

#endif //BOOSTPROJECTS_UTILITIES_H
