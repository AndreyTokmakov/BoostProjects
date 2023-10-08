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

#include <boost/array.hpp>
#include <memory>
#include <source_location>

namespace Experiments
{
    namespace asio = boost::asio;
    namespace ip = asio::ip;
    using tcp = boost::asio::ip::tcp;
    namespace system = boost::system;


    std::string getCurrentTime() noexcept {
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

    struct printer {
        explicit printer(boost::asio::io_context& io):
                timer(io, boost::asio::chrono::seconds(1)), count(0) {
        }

        void print()
        {
            if (count < 5)
            {
                std::cout << getCurrentTime() << ": " << count << std::endl;
                ++count;

                timer.expires_at(timer.expiry() + boost::asio::chrono::seconds(1));
                timer.async_wait(boost::bind(&printer::print, this));
            }
        }

        ~printer()
        {
            std::cout << getCurrentTime() << ": Final count is " << count << std::endl;
        }

    private:
        boost::asio::steady_timer timer;
        int count;
    };



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
        asio::io_context ioContext;
        printer p(ioContext);
        ioContext.run();
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
        try {
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

void Experiments::TestAll()
{
    // connectToServer();

    // Timer();
    // Timer_Async();
    // Timer_Async_With_MemberFunction();


    // TcpDaytimeServer::runServer();
    TcpDaytimeServer::runClient();
}