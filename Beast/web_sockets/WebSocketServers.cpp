/**============================================================================
Name        : WebSocketServers.cpp
Created on  : 16.01.2025
Author      : Andrei Tokmakov
Version     : 1.0
Copyright   : Your copyright notice
Description : WebSocketServers.cpp
============================================================================**/

#include "WebSocketServers.h"

#include <iostream>
#include <string_view>
#include <utility>
#include <vector>
#include <thread>
#include <fstream>
#include <format>
#include <print>
#include <chrono>
#include <typeindex>

#include <boost/asio.hpp>
#include <boost/asio/experimental/awaitable_operators.hpp>
#include <boost/asio/experimental/basic_channel.hpp>
#include <boost/asio/experimental/basic_concurrent_channel.hpp>
#include <boost/asio/experimental/channel.hpp>
#include <boost/asio/experimental/co_composed.hpp>
#include <boost/asio/experimental/concurrent_channel.hpp>
#include <boost/asio/experimental/coro.hpp>
#include <boost/asio/experimental/use_coro.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/variant2/variant.hpp>
#include <boost/asio/stream_file.hpp>
#include <boost/system/error_code.hpp>
#include <boost/beast/websocket/stream.hpp>
#include <boost/asio/as_tuple.hpp>
#include <boost/asio/awaitable.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/beast/websocket/ssl.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl.hpp>

#include "server_certificate.hpp"
#include "root_certificates.hpp"


namespace
{
    namespace asio = boost::asio;
    namespace beast = boost::beast;
    namespace websocket = beast::websocket;
    namespace http = beast::http;
    namespace ssl = asio::ssl;
    using tcp = asio::ip::tcp;
}

namespace SimpleServer
{
    constexpr std::string_view host { "0.0.0.0" };
    constexpr uint16_t port { 6789 };

    void handleSession(tcp::socket socket)
    {
        try
        {
            // Construct the stream by moving in the socket
            websocket::stream<tcp::socket> wsStream { std::move(socket) };

            // Set a decorator to change the Server of the handshake
            wsStream.set_option(websocket::stream_base::decorator([](websocket::response_type& res){
                res.set(http::field::server, std::string(BOOST_BEAST_VERSION_STRING) + " websocket-server-sync");
            }));

            // Accept the websocket handshake
            wsStream.accept();
            std::cout << "Handling new connection" << std::endl;

            while (true)
            {
                // This buffer will hold the incoming message
                beast::flat_buffer buffer;

                // Read a message
                const size_t bytesRead = wsStream.read(buffer);
                std::cout << "Got " << bytesRead  << " bytes from client" << std::endl;

                // Echo the message back
                wsStream.text(wsStream.got_text());

                const size_t bytesSend = wsStream.write(buffer.data());
                std::cout << bytesSend << " bytes send back"<< std::endl;
            }
        }
        catch (const beast::system_error& exc)
        {
            // This indicates that the session was closed
            if (exc.code() != websocket::error::closed) {
                std::cerr << "Error: " << exc.code().message() << std::endl;
            }
        }
        catch (const std::exception& exc)
        {
            std::cerr << "Error: " << exc.what() << std::endl;
        }
    }

    void runServer()
    {
        try
        {
            const asio::ip::address address = asio::ip::make_address(host);
            asio::io_context ioCtx { 1 };

            // The acceptor receives incoming connections
            tcp::acceptor acceptor { ioCtx, { address, port } };
            while (true)
            {
                tcp::socket socket{ioCtx};
                acceptor.accept(socket);
                std::jthread(&handleSession, std::move(socket)).detach();
            }
        }
        catch (const std::exception& e)
        {
            std::cerr << "Error: " << e.what() << std::endl;
            return;
        }
    }
}


namespace SSLServer
{
    constexpr std::string_view host { "0.0.0.0" };
    constexpr uint16_t port { 6789 };

    void handleSession(tcp::socket&& socket, ssl::context& ctx)
    {
        try
        {
            // Construct the websocket stream around the socket
            websocket::stream<ssl::stream<tcp::socket&>> wsStream{socket, ctx};
            // Perform the SSL handshake
            wsStream.next_layer().handshake(ssl::stream_base::server);

            // Set a decorator to change the Server of the handshake
            wsStream.set_option(websocket::stream_base::decorator([](websocket::response_type& res){
                res.set(http::field::server, std::string(BOOST_BEAST_VERSION_STRING) + " websocket-server-sync-ssl");
            }));

            // Accept the websocket handshake
            wsStream.accept();
            std::cout << "Handling new connection" << std::endl;

            while (true)
            {
                // This buffer will hold the incoming message
                beast::flat_buffer buffer;

                // Read a message
                const size_t bytesRead = wsStream.read(buffer);
                std::cout << "Got " << bytesRead  << " bytes from client" << std::endl;

                // Echo the message back
                wsStream.text(wsStream.got_text());

                const size_t bytesSend = wsStream.write(buffer.data());
                std::cout << bytesSend << " bytes send back"<< std::endl;
            }
        }
        catch (const beast::system_error& exc)
        {   // This indicates that the session was closed
            if (exc.code() != websocket::error::closed)
                std::cerr << "Beast error: " << exc.code().message() << std::endl;
        }
        catch (const std::exception& exc) {
            std::cerr << "Error: " << exc.what() << std::endl;
        }
    }

    void runServer()
    {
        try
        {
            auto const address = asio::ip::make_address(host);
            asio::io_context ioCtx { 1 };

            // The SSL context is required, and holds certificates
            ssl::context ctx { ssl::context::tlsv13 };

            // This holds the self-signed certificate used by the server
            load_server_certificate(ctx);

            tcp::acceptor acceptor { ioCtx, { address, port } };
            while (true)
            {
                tcp::socket socket{ioCtx};
                acceptor.accept(socket);
                std::jthread(&handleSession, std::move(socket), std::ref(ctx)).detach();
            }
        }
        catch (const std::exception& e)
        {
            std::cerr << "Error: " << e.what() << std::endl;
            return;
        }
    }
}


void WebSocketServers::TestAll()
{
    // SimpleServer::runServer();
    SSLServer::runServer();
}