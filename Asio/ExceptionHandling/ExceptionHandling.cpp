/**============================================================================
Name        : ExceptionHandling.cpp
Created on  : 01.07.2025
Author      : Andrei Tokmakov
Version     : 1.0
Copyright   : Your copyright notice
Description : ExceptionHandling.cpp
============================================================================**/

#include "ExceptionHandling.h"

#include "../Utils/Utilities.h"

#include <iostream>
#include <chrono>
#include <thread>

#include <boost/asio.hpp>

namespace
{
    using namespace Utilities;
}


namespace ExceptionHandling
{

    void Future_Exception_Handling()
    {
        boost::asio::io_context io_context;
        boost::asio::steady_timer timer(io_context, std::chrono::seconds(4u));

        std::thread io_thread([&io_context]() { io_context.run(); });
        std::future<void> fut = timer.async_wait(boost::asio::use_future);

        std::this_thread::sleep_for(std::chrono::seconds(2u));

        // Cancel timer, throwing an exception that is caught by the future.
        timer.cancel();

        try {
            fut.get();
            DEBUG << "Timer expired successfully!\n";
        } catch (const boost::system::system_error& e) {
            ERROR << "Timer failed: " << e.code().message() << '\n';
        }

        io_thread.join();
    }

}

void ExceptionHandling::TestAll()
{
    ExceptionHandling::Future_Exception_Handling();
}