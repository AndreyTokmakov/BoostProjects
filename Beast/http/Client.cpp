/**============================================================================
Name        : Client.cpp
Created on  : 16.01.2025
Author      : Andrei Tokmakov
Version     : 1.0
Copyright   : Your copyright notice
Description : Client.cpp
============================================================================**/

#include "Client.h"


#include <boost/asio.hpp>
#include <boost/thread.hpp>
#include <boost/algorithm/hex.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/spawn.hpp>
#include <boost/json.hpp>

#include <iostream>
#include <string>
#include <memory>
#include <source_location>


namespace
{
    namespace beast = boost::beast;
    namespace http = beast::http;
    namespace asio = boost::asio;
    namespace ssl = boost::asio::ssl;
    namespace json = boost::json;
    namespace ip = boost::asio::ip;
    using tcp = asio::ip::tcp;
}

namespace Client::Simple_Synch
{
    constexpr std::string_view host { "0.0.0.0"};
    constexpr int32_t port { 52525 };
    constexpr std::string_view path { "/index" };
    constexpr int32_t version { 11 };

    void httpGetRequest()
    {
        asio::io_context ioCtx;
        beast::tcp_stream stream(ioCtx);

        tcp::resolver resolver(ioCtx);
        tcp::resolver::results_type serverEndpoint = resolver.resolve(host, std::to_string(port));

        stream.connect(serverEndpoint);

        // Set up an HTTP GET request message
        http::request<http::string_body> request { http::verb::get, path.data(), version };
        request.set(http::field::host, host);
        request.set(http::field::content_type, "application/json");
        request.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);
        request.set(http::field::connection, "close");

        // Send the HTTP request to the remote host
        http::write(stream, request);

        beast::flat_buffer buffer;
        http::response<http::dynamic_body> response;

        try
        {
            const size_t bytesRead = http::read(stream, buffer, response);
            std::cout << "[" << response << "]\n";
            std::cout << "Bytes received: " << bytesRead << std::endl;
            // std::cout << beast::buffers_to_string(buffer.data()) << std::endl;
        }
        catch (const std::exception& exc) {
            std::cerr << exc.what() << std::endl;
        }

        beast::error_code ec;
        stream.socket().shutdown(tcp::socket::shutdown_both, ec);
    }

    void httpGetRequest_TcpSocket()
    {
        try {
            asio::io_context ioContext;

            tcp::resolver resolver(ioContext);
            tcp::resolver::results_type serverEndpoint = resolver.resolve(host, std::to_string(port));

            tcp::socket socket(ioContext);
            asio::connect(socket, serverEndpoint);

            asio::streambuf request;
            std::ostream request_stream(&request);
            request_stream << "GET " << path << " HTTP/1.0\r\n"
                           << "Host: " << host << "\r\n"
                           << "Accept: */*\r\n"
                           << "Connection: close\r\n\r\n";

            // Send the request.
            asio::write(socket, request);

            asio::streambuf response;
            asio::read_until(socket, response, "\r\n");

            std::istream response_stream(&response);

            std::string http_version;
            response_stream >> http_version;

            unsigned int status_code;
            response_stream >> status_code;

            std::string status_message;
            std::getline(response_stream, status_message);
            if (!response_stream || http_version.substr(0, 5) != "HTTP/"){
                std::cout << "Invalid response\n";
                return;
            }
            else if (200 != status_code) {
                std::cout << "Response returned with status code " << status_code << "\n";
                return;
            }

            // Read the response headers, which are terminated by a blank line.
            asio::read_until(socket, response, "\r\n\r\n");

            // Process the response headers.
            std::string header;
            while (std::getline(response_stream, header) && header != "\r")
                std::cout << header << "\n";
            std::cout << "\n";

            // Write whatever content we already have to output.
            if (response.size() > 0)
                std::cout << &response;

            // Read until EOF, writing data to output as we go.
            boost::system::error_code error;
            while (asio::read(socket, response,asio::transfer_at_least(1), error))
                std::cout << &response;
            if (error != asio::error::eof)
                throw boost::system::system_error(error);
        }
        catch (std::exception& e)
        {
            std::cout << "Exception: " << e.what() << "\n";
        }
    }
}


namespace Client::HTTP_Client_Async
{
    class session : public std::enable_shared_from_this<session>
    {
        tcp::resolver resolver;
        beast::tcp_stream stream;
        beast::flat_buffer buffer;
        http::request<http::empty_body> request;
        http::response<http::string_body> response;

    public:
        // Objects are constructed with a strand to ensure that handlers do not execute concurrently.
        explicit session(asio::io_context& ioc):
            resolver(asio::make_strand(ioc)), stream(asio::make_strand(ioc)) {
        }

        // Start the asynchronous operation
        void run(const std::string_view host,
                 const std::string_view port,
                 const std::string_view target,
                 const int version)
        {
            // Set up an HTTP GET request message
            request.version(version);
            request.method(http::verb::get);
            request.target(target);
            request.set(http::field::host, host);
            request.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);

            // Look up the domain name
            resolver.async_resolve(host,port,
                                   beast::bind_front_handler(&session::on_resolve,shared_from_this()));
        }

        void on_resolve(const beast::error_code& errorCode,
                        const tcp::resolver::results_type& results)
        {
            if (errorCode) {
                return fail(errorCode, "resolve");
            }
            // Set a timeout on the operation
            stream.expires_after(std::chrono::seconds(30u));

            // Make the connection on the IP address we get from a lookup
            stream.async_connect(results,
                                 beast::bind_front_handler(&session::on_connect, shared_from_this()));
        }

        void on_connect(const beast::error_code& errorCode,
                        const tcp::resolver::results_type::endpoint_type&)
        {
            if (errorCode) {
                return fail(errorCode, "connect");
            }
            // Set a timeout on the operation
            stream.expires_after(std::chrono::seconds(30u));

            // Send the HTTP request to the remote host
            http::async_write(stream, request,
                              beast::bind_front_handler(&session::on_write, shared_from_this()));
        }

        void on_write(const beast::error_code& errorCode,
                      std::size_t bytes_transferred)
        {
            boost::ignore_unused(bytes_transferred);
            if (errorCode) {
                return fail(errorCode, "write");
            }
            // Receive the HTTP response
            http::async_read(stream, buffer, response,
                             beast::bind_front_handler(&session::on_read, shared_from_this()));
        }

        void on_read(const beast::error_code& errorCode,
                     std::size_t bytes_transferred)
        {
            boost::ignore_unused(bytes_transferred);
            if (errorCode) {
                return fail(errorCode, "read");
            }

            // Write the message to standard out
            std::cout << response << std::endl;

            // Gracefully close the socket
            boost::system::error_code err;
            [[maybe_unused]] auto result = stream.socket().shutdown(tcp::socket::shutdown_both, err);
            if (err) {
                std::cerr << "shutdown failed: " << err.message() << "\n";
            }

            // not_connected happens sometimes so don't bother reporting it.
            if (errorCode && errorCode != beast::errc::not_connected)
                return fail(errorCode, "shutdown");
        }

        static void fail(const beast::error_code& ec,
                         const std::string_view what)
        {
            std::cerr << what << ": " << ec.message() << "\n";
        }
    };

    void Send_Request()
    {
        constexpr std::string_view host { "0.0.0.0"}, port { "52525"};
        constexpr std::string_view path { "/index" };
        constexpr int version { 11 };

        // The io_context is required for all I/O
        asio::io_context ioc;

        // Launch the asynchronous operation
        std::make_shared<session>(ioc)->run(host, port, path, version);

        // Run the I/O service. The call will return when the get operation is complete.
        ioc.run();
    }
}


void Client::TestAll()
{
    // Simple_Synch::httpGetRequest();
    Simple_Synch::httpGetRequest_TcpSocket();

    // HTTP_Client_Async::Send_Request();
}