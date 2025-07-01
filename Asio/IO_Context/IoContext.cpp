/**============================================================================
Name        : IoContext.cpp
Created on  : 28.06.2025
Author      : Andrei Tokmakov
Version     : 1.0
Copyright   : Your copyright notice
Description : IoContext.cpp
============================================================================**/

#include "IoContext.h"

#include "../Utils/Utilities.h"

#include <iostream>
#include <chrono>
#include <thread>

#include <boost/asio.hpp>

namespace
{
    using namespace Utilities;
}


namespace IoContext
{
    using boost::asio::ip::tcp;


    void example_1()
    {
        boost::asio::io_context io;

        io.post([]() {
            std::osyncstream { std::cout } << "Hello from io_context!" << std::endl;
        });

        io.run();
    }

    void example_MultipleThreads()
    {
        boost::asio::io_context io;

        for (int i = 0; i < 5; ++i) {
            io.post([i]() {
                std::osyncstream { std::cout } << getCurrentTime() << "Task " << i << " on thread "
                    << std::this_thread::get_id() << "\n";
            });
        }

        std::vector<std::jthread> threads;
        for (int i = 0; i < 3; ++i) {
            threads.emplace_back([&io]() { io.run(); });
        }
    }

    void getExecutor()
    {
        boost::asio::io_context io;

        const boost::asio::any_io_executor executor = io.get_executor();

        boost::asio::steady_timer timer(executor, std::chrono::seconds(2UL));
        timer.async_wait([](const boost::system::error_code& ec) {
            if (!ec) {
                std::osyncstream { std::cout } << "Timer fired!" << std::endl;
            }
        });

        io.run();
    }

    void getExecutor_AsynchSocket()
    {
        boost::asio::io_context io;
        const boost::asio::any_io_executor executor = io.get_executor();

        tcp::socket socket(executor);
        tcp::resolver resolver(executor);
        tcp::resolver::results_type endpoints = resolver.resolve("example.com", "80");

        boost::asio::async_connect(socket, endpoints,[](
                const boost::system::error_code& ec, const tcp::endpoint&) {
                    if (!ec)
                        std::osyncstream { std::cout } << "Connected!" << std::endl;
        });

        io.run();
    }

    void Strand_Example()
    {
        boost::asio::io_context io;
        boost::asio::strand<boost::asio::io_context::executor_type> strand(io.get_executor());

        auto handler = [] (int i) {
            std::osyncstream { std::cout } << "Handler " << i << " in thread " << std::this_thread::get_id() << "\n";
        };

        for (int i = 0; i < 5; ++i) {
            boost::asio::post(strand, [i, handler]() { handler(i); });
        }

        std::vector<std::jthread> threads;
        for (int i = 0; i < 3; ++i) {
            threads.emplace_back([&] { io.run(); });
        }

        // Even though io_context runs in 3 threads, handlers launched via strand are guaranteed not to run concurrently.

        // Handler 0 in thread 140549010921216
        // Handler 1 in thread 140549010921216
        // Handler 2 in thread 140549010921216
        // Handler 3 in thread 140549010921216
        // Handler 4 in thread 140549010921216
    }

    void Post_vs_Dispatch()
    {
        boost::asio::io_context io_context;

        io_context.post([] { DEBUG << "This will always run asynchronously.\n"; });
        io_context.dispatch([] { DEBUG << "This might run immediately or be queued.\n"; });

        io_context.run();
    }
}

void IoContext::TestAll()
{
    // IoContext::example_1();
    // IoContext::example_MultipleThreads();
    // IoContext::getExecutor();
    // IoContext::getExecutor_AsynchSocket();
    // IoContext::Strand_Example();
    IoContext::Post_vs_Dispatch();
}