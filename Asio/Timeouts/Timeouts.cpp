/**============================================================================
Name        : Timeouts.cpp
Created on  : 28.01.2025
Author      : Andrei Tokmakov
Version     : 1.0
Copyright   : Your copyright notice
Description : Timeouts.cpp
============================================================================**/

#include "Timeouts.h"
#include "../Utils/Utilities.h"

#include <iostream>
#include <chrono>

#include <boost/asio.hpp>

namespace
{
    using namespace Utilities;
}

namespace Timeouts
{
    void on_timeout(const boost::system::error_code& ec)
    {
        if (!ec) {
            std::cout << getCurrentTime() << " Timer expired.\n";
        } else {
            std::cout << getCurrentTime() << " Error: " << ec.message() << '\n';
        }
    }

    void SimpleTest()
    {
        std::cout << getCurrentTime() << " SimpleTest() started\n";

        boost::asio::io_context io_context;
        boost::asio::steady_timer timer(io_context, std::chrono::seconds(3u));
        timer.async_wait(&on_timeout);
        io_context.run();
    }
}

void Timeouts::TestAll()
{
    Timeouts::SimpleTest();
}