/**============================================================================
Name        : HTTPServer.cpp
Created on  : 19.08.2023
Author      : Andrei Tokmakov
Version     : 1.0
Copyright   : Your copyright notice
Description : HTTPServer
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

namespace HTTPServer::Beast
{
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
        boost::asio::io_context io_context;
        tcp::acceptor acceptor(io_context, {tcp::v4(), 8080});

        while (true) {
            tcp::socket socket(io_context);
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

    // http://0.0.0.0:8080/
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
    Beast::start();
};
