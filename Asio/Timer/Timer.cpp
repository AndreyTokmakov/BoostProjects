/**============================================================================
Name        : Timer.cpp
Created on  : 30.01.2025
Author      : Andrei Tokmakov
Version     : 1.0
Copyright   : Your copyright notice
Description : Timer.cpp
============================================================================**/

#include "Timer.h"
#include "../Utils/Utilities.h"


#include <boost/asio.hpp>
#include <chrono>
#include <functional>
#include <iostream>

namespace
{
    using namespace Utilities;
    using namespace std::chrono_literals;
}

namespace Timer
{
    void chained_asynchronous_tasks()
    {
        boost::asio::io_context io_context;
        boost::asio::steady_timer timer(io_context, 3s);

        std::function<void(const boost::system::error_code&)> timer_handler = [&timer, &timer_handler]
                (const boost::system::error_code& ec) {
            if (!ec) {
                DEBUG << "Handler: Timer expired." << std::endl;
                timer.expires_after(1s);
                timer.async_wait(timer_handler);
            } else {
                ERROR << "Handler error: " << ec.message() << std::endl;
            }
        };

        timer.async_wait(timer_handler);
        io_context.run();
    }
}

void Timer::TestAll()
{
    chained_asynchronous_tasks();
}