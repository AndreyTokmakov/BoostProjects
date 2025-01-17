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

    void fail(const beast::error_code& errorCode,
              const std::string_view message)
    {
        std::cerr << message << ": " << errorCode.message() << "\n";
    }
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

namespace SSL_Asynch_Server
{
    class Session : public std::enable_shared_from_this<Session>
    {
        websocket::stream<ssl::stream<beast::tcp_stream>> wsStream;
        beast::flat_buffer buffer;

    public:
        Session(tcp::socket&& socket, ssl::context& ctx)
            : wsStream(std::move(socket), ctx) {
        }

        void run()
        {   // We need to be executing within a strand to perform async operations on the I/O objects in this session.
            // Although not strictly necessary for single-threaded contexts, this example code is written to be
            // thread-safe by default.
            asio::dispatch(wsStream.get_executor(),
                beast::bind_front_handler(&Session::on_run, shared_from_this()));
        }

        // Start the asynchronous operation
        void on_run()
        {
            // Set the timeout.
            beast::get_lowest_layer(wsStream).expires_after(std::chrono::seconds(30));

             // Perform the SSL handshake
            wsStream.next_layer().async_handshake(ssl::stream_base::server,
                beast::bind_front_handler(&Session::on_handshake, shared_from_this()));
        }

        void on_handshake(const beast::error_code& errorCode)
        {
            if (errorCode)
                return fail(errorCode, "handshake");

            // Turn off the timeout on the tcp_stream, because
            // the websocket stream has its own timeout system.
            beast::get_lowest_layer(wsStream).expires_never();

            // Set suggested timeout settings for the websocket
            wsStream.set_option(websocket::stream_base::timeout::suggested(beast::role_type::server));

            // Set a decorator to change the Server of the handshake
            wsStream.set_option(websocket::stream_base::decorator([](websocket::response_type& res){
                res.set(http::field::server, std::string(BOOST_BEAST_VERSION_STRING) + " websocket-server-async-ssl");
            }));

            // Accept the websocket handshake
            wsStream.async_accept(beast::bind_front_handler(&Session::on_accept,shared_from_this()));
        }

        void on_accept(const beast::error_code& errorCode)
        {
            if (errorCode)
                return fail(errorCode, "accept");
            // Read a message
            do_read();
        }

        void do_read()
        {   // Read a message into our buffer
            wsStream.async_read(buffer, beast::bind_front_handler(&Session::on_read, shared_from_this()));
        }

        void on_read(const beast::error_code& errorCode,
                     std::size_t bytes_transferred)
        {
            boost::ignore_unused(bytes_transferred);
            // This indicates that the session was closed
            if (errorCode == websocket::error::closed)
                return;

            if (errorCode)
                fail(errorCode, "read");

            // Echo the message
            wsStream.text(wsStream.got_text());
            wsStream.async_write(buffer.data(), beast::bind_front_handler(&Session::on_write, shared_from_this()));
        }

        void on_write(const beast::error_code& errorCode,
                      std::size_t bytes_transferred)
        {
            boost::ignore_unused(bytes_transferred);
            if (errorCode)
                return fail(errorCode, "write");
            // Clear the buffer
            buffer.consume(buffer.size());
            // Do another read
            do_read();
        }
    };

    class Listener : public std::enable_shared_from_this<Listener>
    {
        asio::io_context& ioContext;
        ssl::context& context;
        tcp::acceptor acceptor_;

    public:
        Listener(asio::io_context& ioc,
                 ssl::context& ctx,
                 const tcp::endpoint& endpoint)
            : ioContext(ioc), context(ctx), acceptor_(asio::make_strand(ioc))
        {
            beast::error_code errorCode;

            // Open the acceptor
            acceptor_.open(endpoint.protocol(), errorCode);
            if (errorCode) {
                fail(errorCode, "open");
                return;
            }

            // Allow address reuse
            acceptor_.set_option(asio::socket_base::reuse_address(true), errorCode);
            if (errorCode) {
                fail(errorCode, "set_option");
                return;
            }

            // Bind to the server address
            acceptor_.bind(endpoint, errorCode);
            if (errorCode) {
                fail(errorCode, "bind");
                return;
            }

            // Start listening for connections
            acceptor_.listen(asio::socket_base::max_listen_connections, errorCode);
            if (errorCode) {
                fail(errorCode, "listen");
                return;
            }
        }

        // Start accepting incoming connections
        void run()
        {
            do_accept();
        }

    private:

        void do_accept()
        {
            // The new connection gets its own strand
            acceptor_.async_accept(asio::make_strand(ioContext),
                beast::bind_front_handler(&Listener::on_accept,shared_from_this()));
        }

        void on_accept(const beast::error_code& errorCode,
                       tcp::socket socket)
        {
            if (errorCode) {
                fail(errorCode, "accept");
            } else {
                // Create the session and run it
                std::make_shared<Session>(std::move(socket), context)->run();
            }

            // Accept another connection
            do_accept();
        }
    };

    void runServer()
    {
        constexpr std::string_view host { "0.0.0.0" };
        constexpr uint16_t port { 6789 };
        constexpr uint16_t threads { 1 };

        try
        {
            asio::io_context ioCtx { threads };
            ssl::context ctx { ssl::context::tlsv13 };

            load_server_certificate(ctx);

            const asio::ip::address address = asio::ip::make_address(host);
            std::make_shared<Listener>(ioCtx, ctx, tcp::endpoint { address, port })->run();

            std::vector<std::thread> workers;
            workers.reserve(threads - 1);
            for (auto i = threads - 1; i > 0; --i)
                workers.emplace_back([&ioCtx]{
                    ioCtx.run();
            });
            ioCtx.run();
        }
        catch (const std::exception& e)
        {
            std::cerr << "Error: " << e.what() << std::endl;
        }
    }
}


void WebSocketServers::TestAll()
{
    // SimpleServer::runServer();
    // SSLServer::runServer();
    SSL_Asynch_Server::runServer();
}