/**============================================================================
Name        : UDPEchoServerClient.cpp
Created on  : 19.08.2023
Author      : Andrei Tokmakov
Version     : 1.0
Copyright   : Your copyright notice
Description : UDPEchoServerClient
============================================================================**/

#include "UDPEchoServerClient.h"

#include <iostream>
#include <string_view>
#include <array>

#include <boost/asio.hpp>
#include <source_location>


namespace
{
    namespace asio = boost::asio;
    using namespace std::string_view_literals;

    using asio::ip::tcp;
    using asio::ip::udp;

    void log(const std::string_view message = ""sv,
             const std::source_location location = std::source_location::current())
    {
        std::cout << location.function_name() << ": " << location.line();
        if (!message.empty())
            std::cout << " | " << message;
        std::cout << std::endl;
    }
}

namespace UDPEchoServerClient
{
    static constexpr inline uint16_t serverPort = 52525;
    static constexpr inline std::string_view serverHost = "0.0.0.0"sv;
    static constexpr inline size_t maxLength = 1024;

    class ServerAsync
    {
        udp::socket socket;
        udp::endpoint senderEndpoint;
        std::array<char, maxLength> data {};

    public:
        ServerAsync(asio::io_context& io_context, uint16_t port):
                socket(io_context, udp::endpoint(udp::v4(), port))
        {
            log();
            do_receive();
        }

        void do_receive()
        {
            log();
            socket.async_receive_from(asio::buffer(data, maxLength), senderEndpoint,
                                      [this](std::error_code ec, std::size_t bytesReceived){
                                          log(std::to_string(bytesReceived).append(" bytes received"));
                                          if (!ec && bytesReceived > 0) {
                                              do_send(bytesReceived);
                                          } else {
                                              log();
                                              do_receive();
                                          }
                                      });
        }

        void do_send(std::size_t length)
        {
            log();
            socket.async_send_to(asio::buffer(data, length), senderEndpoint,
                                 [this](std::error_code /*ec*/, std::size_t /*bytes_sent*/){
                                     do_receive();
                                 });
        }
    };


    void startServer()
    {
        try {
            asio::io_context io_context;
            ServerAsync server(io_context, serverPort);
            io_context.run();
        } catch (std::exception& e) {
            std::cerr << "Exception: " << e.what() << "\n";
        }
    }


    void client()
    {
        try
        {
            asio::io_context io_context;
            udp::socket s(io_context, udp::endpoint(udp::v4(), 0));

            udp::resolver resolver(io_context);
            udp::endpoint endpoint =
                    *resolver.resolve(udp::v4(), serverHost, std::to_string(serverPort)).begin();

            constexpr std::string_view request { "TEST_DATA" };
            s.send_to(asio::buffer(request, request.size()), endpoint);

            std::array<char, maxLength> reply {};
            udp::endpoint sender_endpoint;
            size_t reply_length = s.receive_from(
                    asio::buffer(reply, maxLength), sender_endpoint);

            std::cout << "Reply is: " << std::string_view {reply.data(), reply_length} << std::endl;
        }
        catch (std::exception& e)
        {
            std::cerr << "Exception: " << e.what() << "\n";
        }
    }
}

void UDPEchoServerClient::TestAll()
{
    // startServer();
    client();
};
