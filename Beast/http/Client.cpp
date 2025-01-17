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

#include "server_certificate.hpp"
#include "root_certificates.hpp"


namespace
{
    namespace beast = boost::beast;
    namespace http = beast::http;
    namespace asio = boost::asio;
    namespace ssl = boost::asio::ssl;
    namespace json = boost::json;
    namespace ip = boost::asio::ip;
    using tcp = asio::ip::tcp;

    void fail(const beast::error_code& errorCode,
              char const* what)
    {
        std::cerr << what << ": " << errorCode.message() << "\n";
    }
}

namespace Simple_Synch
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
        try
        {
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


namespace HTTP_Client_Async
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

namespace SSL_Clients
{
    void SSL_Request_Test()
    {
        std::string host = "api64.ipify.org";

        asio::io_context ioc;
        ssl::context ctx(ssl::context::tlsv13_client);
        ctx.set_default_verify_paths();

        // Set up an SSL context
        beast::ssl_stream<beast::tcp_stream> stream(ioc, ctx);
        stream.set_verify_mode(ssl::verify_none);
        stream.set_verify_callback([]([[maybe_unused]] bool preverified, [[maybe_unused]] ssl::verify_context& ctx) {
            return true; // Accept any certificate
        });
        // Enable SNI
        if (!SSL_set_tlsext_host_name(stream.native_handle(), host.c_str())){
            beast::error_code ec{static_cast<int>(::ERR_get_error()), asio::error::get_ssl_category()};
            throw beast::system_error{ec};
        }
        // Connect to the HTTPS server
        ip::tcp::resolver resolver(ioc);
        get_lowest_layer(stream).connect(resolver.resolve({host, "https"}));
        get_lowest_layer(stream).expires_after(std::chrono::seconds(30u));

        // Construct request
        http::request<http::empty_body> req{http::verb::get, "/?format=json" , 11};
        req.set(http::field::host, host);
        req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);

        // Send the request
        stream.handshake(ssl::stream_base::client);
        http::write(stream, req);

        // Receive the response
        beast::flat_buffer buffer;
        http::response<http::dynamic_body> res;
        http::read(stream, buffer, res);

        // Parse the JSON response
        boost::system::error_code err;
        json::value j = json::parse(buffers_to_string(res.body().data()), err);

        std::cout << "IP address: " << j.at("ip").as_string() << std::endl;

        if (err) {
            std::cerr << "Error parsing JSON: " << err.message() << std::endl;
        }

        beast::error_code errorCode;
        const boost_swap_impl::error_code result = stream.shutdown(errorCode);
        if (result) {
            std::cerr << "stream.shutdown() failed" << std::endl;
        }

        if (errorCode == asio::error::eof) {
            errorCode = {};
        }
        if (errorCode) {
            throw beast::system_error{errorCode};
        }
    }
}

namespace HTTPS_Client_Async
{
    // Performs an HTTP GET and prints the response
    class Session : public std::enable_shared_from_this<Session>
    {
        tcp::resolver resolver_;
        ssl::stream<beast::tcp_stream> tcpStream;
        beast::flat_buffer buffer;
        http::request<http::empty_body> request;
        http::response<http::string_body> response;

    public:
        explicit Session(asio::any_io_executor ex,
                         ssl::context& ctx): resolver_(ex), tcpStream { ex, ctx } {
        }

        // Start the asynchronous operation
        void run(std::string_view host,
                 int32_t port,
                 std::string_view target,
                 int version)
        {
            // Set SNI Hostname (many hosts need this to handshake successfully)
            if (! SSL_set_tlsext_host_name(tcpStream.native_handle(), host.data()))
            {
                beast::error_code ec{static_cast<int>(::ERR_get_error()), asio::error::get_ssl_category()};
                std::cerr << ec.message() << "\n";
                return;
            }

            // Set up an HTTP GET request message
            request.version(version);
            request.method(http::verb::get);
            request.target(target);
            request.set(http::field::host, host);
            request.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);

            // Look up the domain name
            resolver_.async_resolve(host, std::to_string(port),
                                    beast::bind_front_handler(&Session::on_resolve, shared_from_this()));
        }

        void on_resolve(const beast::error_code& errorCode,
                        const tcp::resolver::results_type& results)
        {
            if (errorCode) {
                return fail(errorCode, "resolve");
            }
            // Set a timeout on the operation
            beast::get_lowest_layer(tcpStream).expires_after(std::chrono::seconds(30U));

            // Make the connection on the IP address we get from a lookup
            beast::get_lowest_layer(tcpStream)
                    .async_connect(results,
                                   beast::bind_front_handler(&Session::on_connect,shared_from_this()));
        }

        void on_connect(const beast::error_code& errorCode,
                        const tcp::resolver::results_type::endpoint_type&)
        {
            if (errorCode) {
                return fail(errorCode, "connect");
            }
            // Perform the SSL handshake
            tcpStream.async_handshake(ssl::stream_base::client,
                                      beast::bind_front_handler(&Session::on_handshake, shared_from_this()));
        }

        void on_handshake(const beast::error_code& errorCode)
        {
            if (errorCode) {
                return fail(errorCode, "handshake");
            }
            // Set a timeout on the operation
            beast::get_lowest_layer(tcpStream).expires_after(std::chrono::seconds(30U));

            // Send the HTTP request to the remote host
            http::async_write(tcpStream, request,
                              beast::bind_front_handler(&Session::on_write, shared_from_this()));
        }

        void on_write(const beast::error_code& errorCode,
                      std::size_t bytes_transferred)
        {
            boost::ignore_unused(bytes_transferred);
            if (errorCode) {
                return fail(errorCode, "write");
            }

            buffer.clear();
            // Receive the HTTP response
            http::async_read(tcpStream, buffer, response,
                             beast::bind_front_handler( &Session::on_read, shared_from_this()));
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

            // Set a timeout on the operation
            beast::get_lowest_layer(tcpStream).expires_after(std::chrono::seconds(30U));

            // Gracefully close the stream
            tcpStream.async_shutdown(beast::bind_front_handler(
                    &Session::on_shutdown, shared_from_this()));
        }

        void on_shutdown(const beast::error_code& errorCode)
        {
            // ssl::error::stream_truncated, also known as an SSL "short read",
            // indicates the peer closed the connection without performing the
            // required closing handshake (for example, Google does this to
            // improve performance). Generally this can be a security issue,
            // but if your communication protocol is self-terminated (as
            // it is with both HTTP and WebSocket) then you may simply
            // ignore the lack of close_notify.
            //
            // https://github.com/boostorg/beast/issues/38
            //
            // https://security.stackexchange.com/questions/91435/how-to-handle-a-malicious-ssl-tls-shutdown
            //
            // When a short read would cut off the end of an HTTP message,
            // Beast returns the error beast::http::error::partial_message.
            // Therefore, if we see a short read here, it has occurred
            // after the message has been completed, so it is safe to ignore it.

            if (asio::ssl::error::stream_truncated != errorCode)
                return fail(errorCode, "shutdown");
        }
    };

    void Send_Request()
    {
        constexpr std::string_view host { "0.0.0.0"};
        constexpr int32_t port { 8443 };

        constexpr std::string_view path { "/" };
        // constexpr std::string_view path { "/index" };
        constexpr int32_t version { 11 };

        // The io_context is required for all I/O
        asio::io_context ioc;

        // The SSL context is required, and holds certificates
        ssl::context ctx{ssl::context::tlsv13_client};

        // This holds the root certificate used for verification
        load_root_certificates(ctx);

        // Verify the remote server's certificate
        // ctx.set_verify_mode(ssl::verify_peer);

        // Launch the asynchronous operation
        // The session is constructed with a strand to
        // ensure that handlers do not execute concurrently.
        std::make_shared<Session>(asio::make_strand(ioc), ctx)->run(host, port, path, version);

        // Run the I/O service. The call will return when
        // the get operation is complete.
        ioc.run();
    }
}



namespace HTTPS_Awaitable
{
    // Performs an HTTP GET and prints the response
    asio::awaitable<void> do_session(std::string_view host,
                                     int32_t port,
                                     std::string_view target,
                                     int32_t version,
                                     ssl::context& ctx)
    {
        asio::any_io_executor executor = co_await asio::this_coro::executor;
        tcp::resolver resolver = asio::ip::tcp::resolver{ executor };
        ssl::stream<beast::tcp_stream> tcpStream = ssl::stream<beast::tcp_stream>{ executor, ctx };

        // Set SNI Hostname (many hosts need this to handshake successfully)
        if (!SSL_set_tlsext_host_name(tcpStream.native_handle(), host.data()))
        {
            throw boost::system::system_error(
                    static_cast<int>(::ERR_get_error()),
                    asio::error::get_ssl_category());
        }

        // Look up the domain name
        auto const results = co_await resolver.async_resolve(host, std::to_string(port));

        // Set the timeout.
        beast::get_lowest_layer(tcpStream).expires_after(std::chrono::seconds(30u));

        // Make the connection on the IP address we get from a lookup
        co_await beast::get_lowest_layer(tcpStream).async_connect(results);

        // Set the timeout.
        beast::get_lowest_layer(tcpStream).expires_after(std::chrono::seconds(30u));

        // Perform the SSL handshake
        co_await tcpStream.async_handshake(ssl::stream_base::client);

        // Set up an HTTP GET request message
        http::request<http::string_body> req{ http::verb::get, target, version };
        req.set(http::field::host, host);
        req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);

        // Set the timeout.
        beast::get_lowest_layer(tcpStream).expires_after(std::chrono::seconds(30u));

        // Send the HTTP request to the remote host
        co_await http::async_write(tcpStream, req);

        // This buffer is used for reading and must be persisted
        beast::flat_buffer buffer;

        // Declare a container to hold the response
        http::response<http::dynamic_body> res;

        // Receive the HTTP response
        co_await http::async_read(tcpStream, buffer, res);

        // Write the message to standard out
        std::cout << res << std::endl;

        // Set the timeout.
        beast::get_lowest_layer(tcpStream).expires_after(std::chrono::seconds(30u));

        // Gracefully close the stream - do not threat every error as an exception!
        const auto [ec] = co_await tcpStream.async_shutdown(asio::as_tuple);
        if (ec && ec != asio::ssl::error::stream_truncated)
            throw boost::system::system_error(ec, "shutdown");
    }


    void Send_Request()
    {
        constexpr std::string_view host { "0.0.0.0"};
        constexpr int32_t port { 8443 };

        constexpr std::string_view path { "/" };
        // constexpr std::string_view path { "/index" };
        constexpr int32_t version { 11 };

        // The io_context is required for all I/O
        asio::io_context ioCtx;

        // The SSL context is required, and holds certificates
        ssl::context ctx { ssl::context::tlsv13_client };

        // This holds the root certificate used for verification
        load_root_certificates(ctx);

        // Launch the asynchronous operation
        asio::co_spawn(ioCtx, do_session(host, port, path, version, ctx),[](std::exception_ptr e){
            if (e) {
                std::rethrow_exception(e);
            }
        });

        // Run the I/O service. The call will return when
        // the get operation is complete.
        ioCtx.run();
    }
}


void Client::TestAll()
{
    // Simple_Synch::httpGetRequest();
    // Simple_Synch::httpGetRequest_TcpSocket();

    // HTTP_Client_Async::Send_Request();
    // HTTPS_Client_Async::Send_Request();
    HTTPS_Awaitable::Send_Request();

    // SSL_Clients::SSL_Request_Test();

}