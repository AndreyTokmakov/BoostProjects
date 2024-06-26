/**============================================================================
Name        : CoroutineApps.cpp
Created on  : 23.05.2024
Author      : Andrei Tokmakov
Version     : 1.0
Copyright   : Your copyright notice
Description : CoroutineApps.cpp
============================================================================**/

#include "CoroutineApps.h"

#include <iostream>

#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/thread.hpp>
#include <boost/algorithm/hex.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/asio/spawn.hpp>
#include <boost/asio/experimental/as_single.hpp>

#include <boost/array.hpp>
#include <memory>
#include <source_location>

namespace
{
    namespace asio = boost::asio;
    namespace ip = asio::ip;
    namespace system = boost::system;

    using tcp = boost::asio::ip::tcp;


    [[maybe_unused]]
    constexpr size_t serverPort = 52525;

    [[maybe_unused]]
    constexpr std::string_view serverHost { "0.0.0.0" };
}


namespace CoroutineApps::AcceptServer
{
    boost::asio::awaitable<void> echo(tcp::socket socket)
    {
        std::cout << "echo entered\n";
        try
        {
            char data[1024];
            while (true)
            {
                std::size_t n = co_await socket.async_read_some(boost::asio::buffer(data), boost::asio::use_awaitable);
                co_await async_write(socket, boost::asio::buffer(data, n), boost::asio::use_awaitable);
            }
        }
        catch (std::exception& e)
        {
            std::printf("echo Exception: %s\n", e.what());
        }
    }

    void Test()
    {
        asio::io_context ioContext;
        tcp::acceptor acceptor(ioContext, tcp::endpoint(tcp::v4(), serverPort));

        //system::error_code error;
        while (true)
        {
            tcp::socket clientSocket(ioContext);
            acceptor.accept(clientSocket);

            std::cout << "Client connected" << std::endl;

            boost::asio::co_spawn(ioContext, echo(std::move(clientSocket)), boost::asio::detached);
        }
    }
}

namespace CoroutineApps::EchoServer_One
{
    void report_error(std::string_view component, const system::error_code& ec)
    {
        std::cerr << component << " failure: "<< ec << " ()" << ec.message() << ")\n";
    }

    asio::awaitable<void> session(tcp::socket socket)
    {
        char data[1024];
        try {
            while (true) {
                const std::size_t bytes = co_await socket.async_read_some(asio::buffer(data),
                                                                          asio::use_awaitable);
                std::cout << "session: " << bytes << " bytes read\n";
                co_await async_write(socket,
                                     asio::buffer(data, bytes),
                                     asio::use_awaitable);
            }
        } catch (const system::system_error& e) {
            if (e.code() == asio::error::eof)
                std::cerr << "Session done \n";
            else
                report_error("Session", e.code());
        }
    }

    asio::awaitable<void> listener(asio::io_context& context, unsigned short port)
    {
        tcp::acceptor acceptor(context, {tcp::v4(), port});

        try {
            while (true) {
                tcp::socket socket = co_await acceptor.async_accept(asio::use_awaitable);
                asio::co_spawn(context, session(std::move(socket)), asio::detached);
            }
        } catch (const system::system_error& e) {
            report_error("Listener", e.code());
        }
    }

    void runServer()
    {

        try {
            asio::io_context context;
            asio::signal_set signals(context, SIGINT, SIGTERM);
            signals.async_wait([&](auto, auto){ context.stop(); });

            auto listen = listener(context, serverPort);
            asio::co_spawn(context, std::move(listen), asio::detached);

            context.run();
            std::cerr << "Server done \n";
        }
        catch (std::exception& e)
        {
            std::cerr << "Server failure: " << e.what() << "\n";
        }
    }
}

namespace CoroutineApps::EchoServer_Refactor
{
    namespace coroutines = asio::this_coro;

    asio::awaitable<void> echo_once(tcp::socket& socket)
    {
        char buffer[128];
        const std::size_t bytes = co_await socket.async_read_some(asio::buffer(buffer), asio::use_awaitable);
        std::cout << bytes << " bytes received\n";

        co_await async_write(socket, asio::buffer(buffer, bytes), asio::use_awaitable);
    }

    asio::awaitable<void> echo(tcp::socket socket)
    {
        try
        {   while (true)
            {   // The asynchronous operations to echo a single chunk of data have been refactored into a separate function.
                // When this function is called, the operations are still performed in the context of the current coroutine,
                // and the behaviour is functionally equivalent.
                co_await echo_once(socket);
            }
        } catch (const std::exception& exc) {
            std::printf("echo Exception: %s\n", exc.what());
        }
    }

    asio::awaitable<void> listener()
    {
        asio::any_io_executor executor = co_await coroutines::executor;
        tcp::acceptor acceptor(executor, {tcp::v4(), serverPort});
        while (true)
        {
            tcp::socket socket = co_await acceptor.async_accept(asio::use_awaitable);
            asio::co_spawn(executor, echo(std::move(socket)), asio::detached);
        }
    }

    void runServer()
    {
        try
        {
            asio::io_context ioContext(1);

            asio::signal_set signals(ioContext, SIGINT, SIGTERM);
            signals.async_wait([&](auto, auto){ ioContext.stop(); });

            asio::co_spawn(ioContext, listener(), asio::detached);

            ioContext.run();
        }
        catch (std::exception& e)
        {
            std::printf("Exception: %s\n", e.what());
        }
    }
}

namespace CoroutineApps::EchoServer_Single
{
    namespace coroutines = asio::this_coro;
    using asio::experimental::as_single_t;
    using asio::use_awaitable_t;
    using default_token = as_single_t<use_awaitable_t<>>;
    using tcp_acceptor = default_token::as_default_on_t<tcp::acceptor>;
    using tcp_socket = default_token::as_default_on_t<tcp::socket>;

    asio::awaitable<void> echo_once(tcp_socket socket)
    {
        char buffer[128];
        while (true)
        {
            const auto [err, bytesRead] = co_await socket.async_read_some(boost::asio::buffer(buffer));
            if (0 == bytesRead)
                break;

            std::cout << bytesRead << " bytes read\n";

            const auto [e2, bytesWritten] = co_await async_write(socket, boost::asio::buffer(buffer, bytesRead));
            if (bytesWritten != bytesRead)
                break;
        }
    }

    asio::awaitable<void> listener()
    {
        asio::any_io_executor executor = co_await coroutines::executor;
        tcp_acceptor acceptor(executor, {tcp::v4(), serverPort});
        while (true)
        {
            if (auto [e, socket] = co_await acceptor.async_accept(); socket.is_open())
                asio::co_spawn(executor, echo_once(std::move(socket)), asio::detached);
        }
    }

    void runServer()
    {
        try
        {
            asio::io_context ioContext(1);
            boost::asio::signal_set signals(ioContext, SIGINT, SIGTERM);
            signals.async_wait([&](auto, auto){ ioContext.stop(); });
            asio::co_spawn(ioContext, listener(), asio::detached);
            ioContext.run();
        }
        catch (std::exception& e)
        {
            std::printf("Exception: %s\n", e.what());
        }
    }
}



void CoroutineApps::TestAll()
{
    // AcceptServer::Test();

    EchoServer_One::runServer();
    // EchoServer_Refactor::runServer();
    // EchoServer_Single::runServer();
}