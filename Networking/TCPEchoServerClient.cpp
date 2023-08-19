/**============================================================================
Name        : TCPEchoServerClient.cpp
Created on  : 19.08.2023
Author      : Andrei Tokmakov
Version     : 1.0
Copyright   : Your copyright notice
Description : TCPEchoServerClient
============================================================================**/

#include "TCPEchoServerClient.h"

#include <iostream>
#include <string_view>

#include <boost/asio.hpp>

#include <memory>
#include <source_location>

namespace
{
    namespace asio = boost::asio;
    using namespace std::string_view_literals;

    using asio::ip::tcp;

    void log(const std::string_view message = ""sv,
             const std::source_location location = std::source_location::current())
    {
        std::cout << location.function_name() << ": " << location.line();
        if (!message.empty())
            std::cout << " | " << message;
        std::cout << std::endl;
    }
}

namespace TCPEchoServerClient
{
    static constexpr inline size_t maxLength = 1024;
    static constexpr inline uint16_t serverPort = 52525;

    struct Session: public std::enable_shared_from_this<Session>
    {
    public:
        explicit Session(tcp::socket socket) : socket { std::move(socket) }{
        }

        void start() {
            do_read();
        }

    private:
        void do_read()
        {
            log();
            std::shared_ptr<Session> self { shared_from_this() };
            socket.async_read_some(asio::buffer(data, maxLength),
                                   [this, self](std::error_code ec, std::size_t length){
                                       if (!ec) {
                                           do_write(length);
                                       }
                                   });
        }

        void do_write(std::size_t length)
        {
            log();
            std::shared_ptr<Session> self { shared_from_this() };
            asio::async_write(socket, asio::buffer(data, length),
                              [this, self](std::error_code ec, std::size_t /*length*/){
                                  if (!ec){
                                      do_read();
                                  }
                              });
        }

        tcp::socket socket;
        char data[maxLength] {};
    };

    class Server
    {
    public:
        Server(asio::io_context& io_context, short port):
                acceptor(io_context, tcp::endpoint(tcp::v4(), port)),
                socket(io_context)
        {
            log();
            do_accept();
        }

    private:
        void do_accept()
        {
            log();
            acceptor.async_accept(socket,[this](std::error_code ec){
                if (!ec) {
                    std::make_shared<Session>(std::move(socket))->start();
                }
                do_accept();
            });
        }

        tcp::acceptor acceptor;
        tcp::socket socket;
    };

    void startServer()
    {
        try
        {
            asio::io_context io_context;
            Server sever(io_context, serverPort);
            io_context.run();
        } catch (std::exception& e) {
            std::cerr << "Exception: " << e.what() << "\n";
        }
    }

    void client()
    {
        try {
            asio::io_context io_context;

            tcp::socket s(io_context);
            tcp::resolver resolver(io_context);
            asio::connect(s, resolver.resolve( "0.0.0.0", std::to_string(serverPort)));


            constexpr std::string_view request { "TEST_DATA" };
            asio::write(s, asio::buffer(request, request.size()));

            std::array<char, maxLength> reply {};
            size_t reply_length = asio::read(s, asio::buffer(reply, request.size()));
            std::cout << "Reply is: " << std::string_view {reply.data(), reply_length} << std::endl;
        }
        catch (std::exception& e)
        {
            std::cerr << "Exception: " << e.what() << "\n";
        }
    }
}

void TCPEchoServerClient::TestAll()
{
    // startServer();
    client();
};
