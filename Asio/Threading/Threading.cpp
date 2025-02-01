/**============================================================================
Name        : Threading.cpp
Created on  : 30.01.2025
Author      : Andrei Tokmakov
Version     : 1.0
Copyright   : Your copyright notice
Description : Threading.cpp
============================================================================**/

#include "Threading.h"
#include "../Utils/Utilities.h"

#include <boost/asio.hpp>
#include <iostream>
#include <chrono>
#include <thread>

namespace
{
    using namespace Utilities;
    using namespace std::chrono_literals;
}

namespace Threading::Post_Dispatch
{
    void post_vs_dispatch_task()
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
}

namespace Threading::SingleThread
{
    void handle_timer_expiry(const boost::system::error_code& ec)
    {
        if (!ec) {
            DEBUG << "Timer expired!\n";
        } else {
            DEBUG << "Error in timer: " << ec.message() << std::endl;
        }
    }

    void run_single_thread()
    {
        FUNC_ENTER
        boost::asio::io_context io_context;
        boost::asio::steady_timer timer(io_context, std::chrono::seconds(1u));
        timer.async_wait(&handle_timer_expiry);
        io_context.run();
        FUNC_EXIT
    }
}

namespace Threading::MultithreadedThread
{
    void long_running_task(boost::asio::io_context& io_context, uint32_t task_duration)
    {
        DEBUG << "Background task started: Duration = " << task_duration << " seconds.\n";
        std::this_thread::sleep_for(std::chrono::seconds(task_duration));
        io_context.post([&io_context]() {
            DEBUG << "Background task completed.\n";
            io_context.stop();
        });
        FUNC_EXIT
    }

    void run_async_task()
    {
        FUNC_ENTER
        boost::asio::io_context io_context;

        auto work_guard = boost::asio::make_work_guard(io_context);
        io_context.post([&io_context]() {
            std::thread t(long_running_task, std::ref(io_context), 2);
            DEBUG << "Detaching thread" << std::endl;
            t.detach();

            for (int i = 0; i < 5; ++i) {
                DEBUG << "doing something else ..... " << std::endl;
                std::this_thread::sleep_for(std::chrono::milliseconds (450u));
            }
        });

        DEBUG << "------> Running io_context...\n";
        io_context.run();
        DEBUG << "<------ io_context exit.\n";

        /**
        2025-02-01 06:58:42.677729 run_async_task() entered
        2025-02-01 06:58:42.677806 ------> Running io_context...
        2025-02-01 06:58:42.677842 Detaching thread
        2025-02-01 06:58:42.677851 doing something else .....
        2025-02-01 06:58:42.677859 Background task started: Duration = 2 seconds.
        2025-02-01 06:58:43.127965 doing something else .....
        2025-02-01 06:58:43.578119 doing something else .....
        2025-02-01 06:58:44.028266 doing something else .....
        2025-02-01 06:58:44.478734 doing something else .....
        2025-02-01 06:58:44.678017 long_running_task() exited
        2025-02-01 06:58:44.929443 Background task completed.
        2025-02-01 06:58:44.929499 <------ io_context exit.
         **/
    }

    void background_task(int i)
    {
        FUNC_ENTER
        DEBUG << "Thread " << i << ": Starting...\n";
        boost::asio::io_context io_context;
        auto work_guard = boost::asio::make_work_guard(io_context);

        DEBUG << "Thread " << i << ": Setup timer...\n";
        boost::asio::steady_timer timer(io_context, 1s);
        timer.async_wait([&](const boost::system::error_code& ec) {
            if (!ec) {
                DEBUG << "Timer expired successfully!\n";
            } else {
                DEBUG << "Timer error: " << ec.message() << '\n';
            }
            work_guard.reset();
        });

        DEBUG << "Thread " << i << ": Running io_context...\n";
        io_context.run();
        FUNC_EXIT
    }

    void run_async_task_own_thread()
    {
        FUNC_ENTER
        const int num_threads = 4;
        std::vector<std::jthread> threads;

        for (auto i = 0; i < num_threads; ++i) {
            threads.emplace_back(background_task, i);
        }
        FUNC_EXIT

        /**
        2025-02-01 07:01:10.727781 run_async_task_own_thread() entered
        2025-02-01 07:01:10.727889 background_task() entered
        2025-02-01 07:01:10.727942 Thread 0: Starting...
        2025-02-01 07:01:10.727952 Thread 0: Setup timer...
        2025-02-01 07:01:10.727938 background_task() entered
        2025-02-01 07:01:10.727967 Thread 1: Starting...
        2025-02-01 07:01:10.727972 Thread 1: Setup timer...
        2025-02-01 07:01:10.727985 Thread 0: Running io_context...
        2025-02-01 07:01:10.727985 run_async_task_own_thread() exited
        2025-02-01 07:01:10.727994 Thread 1: Running io_context...
        2025-02-01 07:01:10.727977 background_task() entered
        2025-02-01 07:01:10.728012 Thread 2: Starting...
        2025-02-01 07:01:10.728018 Thread 2: Setup timer...
        2025-02-01 07:01:10.728001 background_task() entered
        2025-02-01 07:01:10.728049 Thread 3: Starting...
        2025-02-01 07:01:10.728054 Thread 3: Setup timer...
        2025-02-01 07:01:10.728058 Thread 2: Running io_context...
        2025-02-01 07:01:10.728074 Thread 3: Running io_context...
        2025-02-01 07:01:11.728090 Timer expired successfully!
        2025-02-01 07:01:11.728146 background_task() exited
        2025-02-01 07:01:11.728147 Timer expired successfully!
        2025-02-01 07:01:11.728170 Timer expired successfully!
        2025-02-01 07:01:11.728227 background_task() exited
        2025-02-01 07:01:11.728070 Timer expired successfully!
        2025-02-01 07:01:11.728199 background_task() exited
        2025-02-01 07:01:11.728297 background_task() exited
        **/
    }
}
void Threading::TestAll()
{
    // Post_Dispatch::post_vs_dispatch_task();
    // SingleThread::run_single_thread();


    // MultithreadedThread::run_async_task();
    MultithreadedThread::run_async_task_own_thread();
}