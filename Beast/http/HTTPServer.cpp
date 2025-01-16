/**============================================================================
Name        : HTTPServer.cpp
Created on  : 16.01.2025
Author      : Andrei Tokmakov
Version     : 1.0
Copyright   : Your copyright notice
Description : HTTPServer.cpp
============================================================================**/

#include "HTTPServer.h"


#include <iostream>
#include <string_view>
#include <array>

#include <boost/asio.hpp>
#include <source_location>
#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <iostream>

namespace HTTPServer
{
    namespace asio = boost::asio;
    using namespace std::string_view_literals;

    using asio::ip::tcp;
    using asio::ip::udp;

    using tcp = boost::asio::ip::tcp;
    namespace http = boost::beast::http;
}

namespace HTTPServer
{
    constexpr std::string_view host { "0.0.0.0"};
    constexpr int32_t port { 52525 };

    void handleRequest(http::request<http::string_body>& request,
                       tcp::socket& socket)
    {
        // Prepare the response message
        http::response<http::string_body> response;
        response.version(request.version());
        response.result(http::status::ok);
        response.set(http::field::server, "My HTTP Server");
        response.set(http::field::content_type, "text/plain");
        response.body() = "Hello, World!";
        response.prepare_payload();

        // Send the response to the client
        boost::beast::http::write(socket, response);
    }

    [[noreturn]]
    void runServer()
    {
        boost::asio::io_context ioContext;
        asio::ip::tcp::endpoint serverAddress(asio::ip::address::from_string(host.data()), port);
        tcp::acceptor acceptor(ioContext, serverAddress);

        while (true) {
            tcp::socket socket(ioContext);
            acceptor.accept(socket);

            // Read the HTTP request
            boost::beast::flat_buffer buffer;
            http::request<http::string_body> request;
            boost::beast::http::read(socket, buffer, request);

            // Handle the request
            handleRequest(request, socket);

            // Close the socket
            socket.shutdown(tcp::socket::shutdown_send);
        }
    }

    // http://0.0.0.0:52525/
    void start()
    {
        try {
            runServer();
        } catch (const std::exception& e) {
            std::cerr << "Exception: " << e.what() << std::endl;
        }
    }
};

void HTTPServer::TestAll()
{
    start();
}