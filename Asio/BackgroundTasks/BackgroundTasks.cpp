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

}

namespace BackgroundTasks::Simple
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
        boost::asio::executor_work_guard work_guard = boost::asio::make_work_guard(io_context);

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

namespace BackgroundTasks::Main_Thread
{
    void backgroundTask(boost::asio::io_context& io_context,
                           uint32_t task_duration)
    {
        DEBUG << "Background task started: Duration = " << task_duration << " seconds.\n";
        std::this_thread::sleep_for(std::chrono::seconds(task_duration));
        io_context.post([&io_context]() {
            DEBUG<< "Background task completed.\n";
            io_context.stop();
        });
    }

    void run()
    {
        boost::asio::io_context ioCtx;
        boost::asio::executor_work_guard work_guard = boost::asio::make_work_guard(ioCtx);

        ioCtx.post([&ioCtx]() {
            std::thread t(backgroundTask, std::ref(ioCtx), 2);
            DEBUG << "Detaching thread" << std::endl;
            t.detach();
        });

        DEBUG << "Running io_context...\n";
        ioCtx.run();
        DEBUG << "io_context exit.\n";
    }
}

namespace BackgroundTasks::Separate_Threads
{
    void backgroundTask(uint32_t idx)
    {
        DEBUG << "Thread " << idx << ": Starting...\n";
        boost::asio::io_context io_context;
        auto work_guard = boost::asio::make_work_guard(io_context);

        DEBUG << "Thread " << idx << ": Setup timer...\n";
        boost::asio::steady_timer timer(io_context, 1s);
        timer.async_wait([&](const boost::system::error_code& ec) {
            if (!ec) {
                DEBUG << "Timer expired successfully!\n";
            } else {
                DEBUG << "Timer error: " << ec.message() << '\n';
            }
            work_guard.reset();
        });

        DEBUG << "Thread " << idx << ": Running io_context...\n";
        io_context.run();
    }

    void run()
    {
        const int num_threads = 4;
        std::vector<std::jthread> threads;
        for (auto i = 0; i < num_threads; ++i) {
            threads.emplace_back(backgroundTask, i);
        }
    }
}

namespace BackgroundTasks::SingleContext_MultipleThreads
{
    void backgroundTask(uint32_t taskId)
    {
        boost::asio::post([taskId]() {
            DEBUG << "Task " << taskId << " is being handled in thread " << std::this_thread::get_id() << std::endl;
            std::this_thread::sleep_for(2s);
            DEBUG << "Task " << taskId << " complete.\n";
        });
    }

    void run()
    {
        boost::asio::io_context ioCtx;
        boost::asio::executor_work_guard work_guard = boost::asio::make_work_guard(ioCtx);

        std::jthread io_context_thread([&ioCtx]() { ioCtx.run(); });

        constexpr int num_threads = 4;
        std::vector<std::jthread> threads;
        for (int i = 0; i < num_threads; ++i) {
            backgroundTask(i);
        }

        std::this_thread::sleep_for(5s);
        work_guard.reset();
    }
}


namespace BackgroundTasks::Chained_Asynchronous_Tasks
{
    void run()
    {
        boost::asio::io_context io_context;
        boost::asio::steady_timer timer(io_context, 3s);

        std::function<void(const boost::system::error_code&)> timer_handler;
        timer_handler = [&timer, &timer_handler](const boost::system::error_code& ec) {
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

void BackgroundTasks::TestAll()
{
    // Simple::runBackgroundTask();
    // Main_Thread::run();
    // Separate_Threads::run();
    SingleContext_MultipleThreads::run();
    // Chained_Asynchronous_Tasks::run();
}