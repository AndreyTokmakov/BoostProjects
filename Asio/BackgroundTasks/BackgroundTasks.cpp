/**============================================================================
Name        : BackgroundTasks.cpp
Created on  : 28.01.2025
Author      : Andrei Tokmakov
Version     : 1.0
Copyright   : Your copyright notice
Description : BackgroundTasks.cpp
============================================================================**/

#include "BackgroundTasks.h"
#include "../Utils/Utilities.h"

#include <boost/asio.hpp>
#include <chrono>
#include <iostream>
#include <thread>

namespace
{
    using namespace Utilities;
    using namespace std::chrono_literals;

#define DEBUG  std::cout << getCurrentTime() << " "
}

namespace BackgroundTasks
{
    void background_task(boost::asio::io_context& io_context)
    {
        // Waiting for 1 second before posting work
        std::this_thread::sleep_for(1s);
        DEBUG << "Posting a background task.\n";
        io_context.post([]() {
            std::this_thread::sleep_for(1s);
            DEBUG << "Background task completed!\n";
        });
    }

    void runBackgroundTask()
    {
        boost::asio::io_context io_context;
        auto work_guard = boost::asio::make_work_guard(io_context);

        // Work guard avoids run() to return immediately
        std::thread io_thread([&io_context]() {
            DEBUG << "--> Running io_context.\n";
            io_context.run();
            DEBUG << "<-- io_context stopped.\n";
        });

        // Creating a thread and posting some work after 2 seconds.
        std::thread worker(background_task, std::ref(io_context));

        // Main thread doing some work.
        std::this_thread::sleep_for(3s);

        // Removing work guard to let the io_context stop.
        DEBUG << "Removing work_guard.\n";
        work_guard.reset();

        // Joining threads
        worker.join();
        io_thread.join();
    }
}

void BackgroundTasks::TestAll()
{
    runBackgroundTask();
}