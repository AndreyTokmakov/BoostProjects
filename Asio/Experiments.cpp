/**============================================================================
Name        : Experiments.cpp
Created on  : 08.10.2023
Author      : Andrei Tokmakov
Version     : 1.0
Copyright   : Your copyright notice
Description : Experiments.cpp
============================================================================**/

#include "Experiments.h"

#include <iostream>

#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/thread.hpp>
#include <boost/algorithm/hex.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/asio/spawn.hpp>

#include <boost/array.hpp>
#include <memory>
#include <source_location>

namespace
{
    namespace asio = boost::asio;
    namespace ip = asio::ip;
    using tcp = boost::asio::ip::tcp;
    namespace system = boost::system;


    [[maybe_unused]]
    constexpr size_t serverPort = 52525;

    [[maybe_unused]]
    constexpr std::string_view serverHost { "0.0.0.0" };
}


namespace Experiments
{
    std::string getCurrentTime() noexcept
    {
        const std::chrono::time_point now { std::chrono::system_clock::now() };
        const time_t in_time_t { std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()) };
        const std::chrono::duration nowMs = std::chrono::duration_cast<std::chrono::microseconds>(
                now.time_since_epoch()) % 1000000;
        std::stringstream ss;
        ss << std::put_time(std::localtime(&in_time_t), "%a %b %d %Y %T")
           << '.' << std::setfill('0') << std::setw(6) << nowMs.count();
        return ss.str();
    }

    std::string make_daytime_string()
    {
        time_t now = time(nullptr);
        return ctime(&now);
    }
}


namespace Experiments
{

    void print(const boost::system::error_code& /*e*/)
    {
        std::cout << getCurrentTime() << ": Hello world!\n";
    }

#if 0
    struct Printer
    {
        explicit Printer(boost::asio::io_context& io):
                timer(io, boost::asio::chrono::seconds(1)), count(0) {
        }

        void print()
        {
            if (count < 5)
            {
                std::cout << getCurrentTime() << ": " << count << std::endl;
                ++count;

                timer.expires_at(timer.expiry() + boost::asio::chrono::seconds(1));

                /** CPU 100+ when uncommented **/
                //timer.async_wait(boost::bind(&Printer::print, this));
            }
        }

        ~Printer()
        {
            std::cout << getCurrentTime() << ": Final count is " << count << std::endl;
        }

    private:
        boost::asio::steady_timer timer;
        int count;
    };
#endif

    void connectToServer()
    {
        asio::io_context ioContext;
        tcp::socket socket(ioContext);

        const tcp::endpoint server { tcp::v4(), 52525 };

        boost::system::error_code ec;
        socket.connect(server, ec);
    }

    void Timer()
    {
        std::cout << getCurrentTime() << ": Starting...." << std::endl;

        asio::io_context ioContext;
        asio::steady_timer timer(ioContext, boost::asio::chrono::seconds(3));
        timer.wait();

        std::cout << getCurrentTime() << ": Hello, world!" << std::endl;
    }

    void Timer_Async()
    {
        std::cout << getCurrentTime() << ": Starting...." << std::endl;

        asio::io_context ioContext;
        asio::steady_timer timer(ioContext, boost::asio::chrono::seconds(3));
        timer.async_wait(&print);
        timer.wait();

        ioContext.run();
    }

    void Timer_Async_With_MemberFunction()
    {
        // asio::io_context ioContext;
        // printer p(ioContext);
        // ioContext.run();
    }
}


namespace Experiments::TcpDaytimeServer
{
    constexpr size_t port = 13;
    constexpr std::string_view host { "0.0.0.0" };

    void runServer()
    {
        try {
            asio::io_context ioContext;
            tcp::acceptor acceptor(ioContext, tcp::endpoint(tcp::v4(), port));

            system::error_code error;
            while (true)
            {
                tcp::socket clientSocket(ioContext);
                acceptor.accept(clientSocket);

                std::cout << "Connection from " << clientSocket.remote_endpoint()
                          << " tot " << clientSocket.local_endpoint() << " established." << std::endl;

                const std::string message = make_daytime_string();
                asio::write(clientSocket, boost::asio::buffer(message), error);
            }
        }
        catch (std::exception& e)
        {
            std::cerr << e.what() << std::endl;
        }
    }

    void runClient()
    {
        try
        {
            asio::io_context ioContext;
            tcp::resolver resolver(ioContext);

            tcp::resolver::results_type endpoints = resolver.resolve(host, "daytime");

            tcp::socket socket(ioContext);
            boost::asio::connect(socket, endpoints);

            boost::array<char, 128> buf {};
            system::error_code error;
            while (true)
            {
                const size_t bytesRead = socket.read_some(boost::asio::buffer(buf), error);
                if (asio::error::eof == error)
                    break; // Connection closed cleanly by peer.
                else if (error)
                    throw boost::system::system_error(error); // Some other error.

                std::cout.write(buf.data(), static_cast<ptrdiff_t>(bytesRead));
            }
        }
        catch (std::exception& e)
        {
            std::cerr << e.what() << std::endl;
        }
    }
}


namespace Coroutines
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

namespace Coroutines2
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

void Experiments::TestAll()
{
    // connectToServer();

    // Timer();
    // Timer_Async();
    // Timer_Async_With_MemberFunction();


    // TcpDaytimeServer::runServer();
    // TcpDaytimeServer::runClient();

    // Coroutines::Test();

    Coroutines2::runServer();
}