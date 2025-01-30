/**============================================================================
Name        : PostDispatchTasks.cpp
Created on  : 30.01.2025
Author      : Andrei Tokmakov
Version     : 1.0
Copyright   : Your copyright notice
Description : PostDispatchTasks.cpp
============================================================================**/

#include "PostDispatchTasks.h"
#include "../Utils/Utilities.h"

#include <boost/asio.hpp>
#include <iostream>
#include <chrono>

namespace
{
    using namespace Utilities;
    using namespace std::chrono_literals;

#define DEBUG  std::cout << getCurrentTime() << " "
#define ERROR  std::cerr << getCurrentTime() << " "
}

void PostDispatchTasks::TestAll()
{
    boost::asio::io_context io_context;

    io_context.post([] {
        std::this_thread::sleep_for(std::chrono::seconds(1u));
        DEBUG << "This will always run asynchronously.\n";
    });

    io_context.dispatch([] {
        DEBUG << "This might run immediately or be queued.\n";
    });

    io_context.run();
}