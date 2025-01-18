/**============================================================================
Name        : WebSocketClients.cpp
Created on  : 16.01.2025
Author      : Andrei Tokmakov
Version     : 1.0
Copyright   : Your copyright notice
Description : WebSocketClients.cpp
============================================================================**/

#include "WebSocketClients.h"

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
    namespace ip = asio::ip;
    using tcp = ip::tcp;

    [[maybe_unused]]
    void fail(const beast::error_code& errorCode,
              std::string_view message)
    {
        std::cerr << message << ": " << errorCode.message() << "\n";
    }

}

namespace SimpleClient
{
    constexpr std::string host { "0.0.0.0" };
    constexpr uint16_t port { 6789 };

    void Client()
    {
        asio::io_context ioCtx;

        tcp::resolver resolver { ioCtx };
        websocket::stream<tcp::socket> wsStream { ioCtx };

        auto const results = resolver.resolve(host, std::to_string(port));
        const ip::basic_endpoint<tcp> endpoint = asio::connect(wsStream.next_layer(), results);
        std::cout << "endpoint: " << endpoint.address() << std::endl;

        // Set a decorator to change the User-Agent of the handshake
        wsStream.set_option(websocket::stream_base::decorator([](websocket::request_type& req){
            req.set(http::field::user_agent,std::string(BOOST_BEAST_VERSION_STRING) +" websocket-client-coro");
        }));

        wsStream.handshake(host + ':' + std::to_string(port), "/chargeStationState");

        constexpr std::string_view message { "CLIENT HELLO" };
        const size_t bytesSend = wsStream.write(asio::buffer(message));
        std::cout << "Bytes send: " << bytesSend << std::endl;

        beast::flat_buffer buffer;
        const size_t bytesRead = wsStream.read(buffer);
        std::cout << "Bytes read: " << bytesRead << std::endl;

        wsStream.close(websocket::close_code::normal);
        std::cout << beast::make_printable(buffer.data()) << std::endl;
    }
}


namespace SslClient
{
    constexpr std::string host { "0.0.0.0" };
    constexpr uint16_t port { 6789 };


    void Client()
    {
        asio::io_context ioCtx;

        // The SSL context is required, and holds certificates
        ssl::context sslCtx { ssl::context::tlsv13_client };

        // This holds the root certificate used for verification
        // load_root_certificates(sslCtx);

        tcp::resolver resolver { ioCtx };
        websocket::stream<ssl::stream<tcp::socket>> wsStream { ioCtx, sslCtx };

        auto const results = resolver.resolve(host, std::to_string(port));
        const asio::ip::basic_endpoint<tcp> endpoint = asio::connect(beast::get_lowest_layer(wsStream), results);
        std::cout << "endpoint: " << endpoint.address() << std::endl;

        // Set SNI Hostname (many hosts need this to handshake successfully)
        if(! SSL_set_tlsext_host_name(wsStream.next_layer().native_handle(), host.c_str()))
            throw beast::system_error(beast::error_code(static_cast<int>(::ERR_get_error()),
                asio::error::get_ssl_category()), "Failed to set SNI Hostname");

        // Perform the SSL handshake
        wsStream.next_layer().handshake(ssl::stream_base::client);

        // Set a decorator to change the User-Agent of the handshake
        wsStream.set_option(websocket::stream_base::decorator([](websocket::request_type& req){
            req.set(http::field::user_agent,std::string(BOOST_BEAST_VERSION_STRING) +" websocket-client-coro");
        }));

        wsStream.handshake(host + ':' + std::to_string(port), "/chargeStatissonState");

        constexpr std::string_view message { "CLIENT HELLO" };
        const size_t bytesSend = wsStream.write(asio::buffer(message));
        std::cout << "Bytes send: " << bytesSend << std::endl;

        beast::flat_buffer buffer;
        const size_t bytesRead = wsStream.read(buffer);
        std::cout << "Bytes read: " << bytesRead << std::endl;

        wsStream.close(websocket::close_code::normal);
        std::cout << beast::make_printable(buffer.data()) << std::endl;
    }
}

namespace AsyncSslClient
{
    class Session : public std::enable_shared_from_this<Session>
    {
        tcp::resolver resolver;
        websocket::stream<ssl::stream<beast::tcp_stream>> wsStream;
        beast::flat_buffer buffer;
        std::string host;
        std::string message;

    public:

        explicit Session(asio::io_context& ioc, ssl::context& ctx)
            : resolver { asio::make_strand(ioc) }, wsStream { asio::make_strand(ioc), ctx } {
        }

        // Start the asynchronous operation
        void run(const std::string_view _host,
                 const int port,
                 const std::string_view text)
        {
            std::cout << __PRETTY_FUNCTION__ << std::endl;
            // Save these for later
            this->host.assign(_host);
            this->message.assign(text);

            // Look up the domain name
            resolver.async_resolve(host, std::to_string(port),
                beast::bind_front_handler(&Session::on_resolve, shared_from_this()));
        }

        void on_resolve(const beast::error_code& errorCode,
                        const tcp::resolver::results_type& results)
        {
            std::cout << __PRETTY_FUNCTION__ << std::endl;
            if (errorCode)
                return fail(errorCode, "resolve");

            // Set a timeout on the operation
            beast::get_lowest_layer(wsStream).expires_after(std::chrono::seconds(30u));

            // Make the connection on the IP address we get from a lookup
            beast::get_lowest_layer(wsStream).async_connect(results,
                beast::bind_front_handler(&Session::on_connect, shared_from_this()));
        }

        void on_connect(const beast::error_code& errorCode,
                        const tcp::resolver::results_type::endpoint_type& ep)
        {
            std::cout << __PRETTY_FUNCTION__ << std::endl;
            if (errorCode)
                return fail(errorCode, "connect");

            // Set a timeout on the operation
            beast::get_lowest_layer(wsStream).expires_after(std::chrono::seconds(30u));

            // Set SNI Hostname (many hosts need this to handshake successfully)
            if (! SSL_set_tlsext_host_name(wsStream.next_layer().native_handle(), host.c_str()))
            {
                const beast::error_code err = beast::error_code(static_cast<int>(::ERR_get_error()),
                    asio::error::get_ssl_category());
                return fail(err, "connect");
            }

            // Update the host_ string. This will provide the value of the
            // Host HTTP header during the WebSocket handshake.
            // See https://tools.ietf.org/html/rfc7230#section-5.4
            host += ':' + std::to_string(ep.port());

            // Perform the SSL handshake
            wsStream.next_layer().async_handshake(ssl::stream_base::client,
                beast::bind_front_handler(&Session::on_ssl_handshake, shared_from_this()));
        }

        void on_ssl_handshake(const beast::error_code& errorCode)
        {
            std::cout << __PRETTY_FUNCTION__ << std::endl;
            if (errorCode)
                return fail(errorCode, "ssl_handshake");

            // Turn off the timeout on the tcp_stream, because the websocket stream has its own timeout system.
            beast::get_lowest_layer(wsStream).expires_never();

            // Set suggested timeout settings for the websocket
            wsStream.set_option(websocket::stream_base::timeout::suggested(beast::role_type::client));

            // Set a decorator to change the User-Agent of the handshake
            wsStream.set_option(websocket::stream_base::decorator([](websocket::request_type& req){
                req.set(http::field::user_agent, std::string(BOOST_BEAST_VERSION_STRING) + " websocket-client-async-ssl");
            }));

            // Perform the websocket handshake
            wsStream.async_handshake(host, "/",
                    beast::bind_front_handler(&Session::on_handshake, shared_from_this()));
        }

        void on_handshake(const beast::error_code& errorCode)
        {
            std::cout << __PRETTY_FUNCTION__ << std::endl;
            if (errorCode)
                return fail(errorCode, "handshake");

            // Send the message
            wsStream.async_write(asio::buffer(message),
                beast::bind_front_handler(&Session::on_write, shared_from_this()));
        }

        void on_write(const beast::error_code& errorCode,
                      std::size_t bytes_transferred)
        {
            std::cout << __PRETTY_FUNCTION__ << std::endl;
            boost::ignore_unused(bytes_transferred);
            if (errorCode)
                return fail(errorCode, "write");

            // Read a message into our buffer
            wsStream.async_read(buffer, beast::bind_front_handler(&Session::on_read, shared_from_this()));
        }

        void on_read(const beast::error_code& errorCode,
                     std::size_t bytes_transferred)
        {
            std::cout << __PRETTY_FUNCTION__ << std::endl;
            boost::ignore_unused(bytes_transferred);
            if (errorCode)
                return fail(errorCode, "read");

            // Close the WebSocket connection
            wsStream.async_close(websocket::close_code::normal,
                beast::bind_front_handler(&Session::on_close, shared_from_this()));
        }

        void on_close(const beast::error_code& errorCode)
        {
            std::cout << __PRETTY_FUNCTION__ << std::endl;
            if (errorCode)
                return fail(errorCode, "close");

            // If we get here then the connection is closed gracefully

            // The make_printable() function helps print a ConstBufferSequence
            std::cout << beast::make_printable(buffer.data()) << std::endl;
        }
    };

    void Client()
    {
        constexpr std::string_view host { "0.0.0.0" };
        constexpr uint16_t port { 6789 };

        const std::string text { "Some_Message" };

        // The io_context is required for all I/O
        asio::io_context ioCtx;

        // The SSL context is required, and holds certificates
        ssl::context ctx{ssl::context::tlsv13_client};

        // This holds the root certificate used for verification
        // load_root_certificates(ctx);

        // Launch the asynchronous operation
        std::make_shared<Session>(ioCtx, ctx)->run(host, port, text);

        // Run the I/O service. The call will return when the socket is closed.
        ioCtx.run();
    }
}


void WebSocketClients::TestAll()
{
    // SimpleClient::Client();
    // SslClient::Client();
    AsyncSslClient::Client();
}