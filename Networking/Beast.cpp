/**============================================================================
Name        : Beast.cpp
Created on  : 27.09.2024
Author      : Andrei Tokmakov
Version     : 1.0
Copyright   : Your copyright notice
Description : Beast.cpp
============================================================================**/

#include "Beast.h"

#include <iostream>

#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/thread.hpp>
#include <boost/algorithm/hex.hpp>
#include <boost/asio/ssl.hpp>

#include <boost/beast/core.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>

#include <boost/json.hpp>

#include <cstdlib>
#include <string>

#include <boost/array.hpp>
#include <memory>
#include <source_location>

namespace asio = boost::asio;
using namespace std::string_view_literals;


namespace Beast
{
    namespace beast = boost::beast;
    namespace http = beast::http;
    namespace net = boost::asio;
    namespace ssl = boost::asio::ssl;
    namespace json = boost::json;
    namespace ip = boost::asio::ip;
    using tcp = net::ip::tcp;
}


namespace Beast::Client
{
    void SimpleHttpGetRequest()
    {
        constexpr std::string_view host { "0.0.0.0"}, port { "52525"};
        constexpr std::string_view path { "/index" };
        constexpr int version { 11 };

        net::io_context ioCtx;
        tcp::resolver resolver(ioCtx);
        beast::tcp_stream stream(ioCtx);

        tcp::resolver::results_type serverEndpoint = resolver.resolve(host, port);

        // Make the connection on the IP address we get from a lookup
        stream.connect(serverEndpoint);

        // Set up an HTTP GET request message
        http::request<http::string_body> request { http::verb::get, path.data(), version };
        request.set(http::field::host, host);
        request.set(http::field::connection, "close");
        request.set(http::field::content_type, "application/json");
        request.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);

        // Send the HTTP request to the remote host
        http::write(stream, request);
        beast::flat_buffer buffer;

        http::response<http::dynamic_body> response;    // Declare a container to hold the response
        http::read(stream, buffer, response);  // Receive the HTTP response

        std::cout << response << std::endl;

        beast::error_code ec;
        stream.socket().shutdown(tcp::socket::shutdown_both, ec);
    }

    void SSL_Request_Test()
    {
        std::string host = "api64.ipify.org";

        // Initialize IO context
        net::io_context ioc;
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
            beast::error_code ec{static_cast<int>(::ERR_get_error()), net::error::get_ssl_category()};
            throw beast::system_error{ec};
        }
        // Connect to the HTTPS server
        ip::tcp::resolver resolver(ioc);
        get_lowest_layer(stream).connect(resolver.resolve({host, "https"}));
        get_lowest_layer(stream).expires_after(std::chrono::seconds(30));

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

        // Cleanup
        beast::error_code ec;
        stream.shutdown(ec);

        if (ec == net::error::eof) {
            ec = {};
        }
        if (ec) {
            throw beast::system_error{ec};
        }
    }
}

namespace Beast::HTTP_Methods
{
    void do_get(beast::tcp_stream & stream,
                http::request<http::string_body> & request,
                beast::flat_buffer buffer,
                http::response<http::dynamic_body> & res)
    {
        request.target("/get");
        request.method(beast::http::verb::get);
        http::write(stream, request);
        http::read(stream, buffer, res);
    }

    void do_head(beast::tcp_stream & stream,
                http::request<http::string_body> & request,
                beast::flat_buffer buffer,
                http::response<http::dynamic_body> & res)
    {
        // we reuse the get endpoint
        request.target("/get");
        request.method(beast::http::verb::head);
        http::write(stream, request);
        http::response_parser<http::dynamic_body> p;
        http::read_header(stream, buffer, p);
        res = p.release();
    }

    void do_patch(beast::tcp_stream & stream,
                  http::request<http::string_body> & request,
                  beast::flat_buffer buffer,
                  http::response<http::dynamic_body> & res)
    {
        request.target("/patch");
        request.method(beast::http::verb::patch);
        request.body() = "Some random patch data";
        request.prepare_payload(); // set content-length based on the body
        http::write(stream, request);
        http::read(stream, buffer, res);
    }

    void do_put(beast::tcp_stream & stream,
                http::request<http::string_body> & request,
                beast::flat_buffer buffer,
                http::response<http::dynamic_body> & res)
    {
        request.target("/put");
        request.method(beast::http::verb::put);
        request.body() = "Some random put data";
        request.prepare_payload(); // set content-length based on the body
        http::write(stream, request);
        http::read(stream, buffer, res);
    }

    void do_post(beast::tcp_stream & stream,
                 http::request<http::string_body> & request,
                 beast::flat_buffer buffer,
                 http::response<http::dynamic_body> & res)
    {
        request.target("/post");
        request.method(beast::http::verb::post);
        request.body() = "Some random post data";
        request.prepare_payload(); // set content-length based on the body
        http::write(stream, request);
        http::read(stream, buffer, res);
    }

    void do_delete(beast::tcp_stream & stream,
                   http::request<http::string_body> & request,
                   beast::flat_buffer buffer,
                   http::response<http::dynamic_body> & res)
    {
        request.target("/delete");
        request.method(beast::http::verb::delete_);
        // NOTE: delete doesn't require a body
        request.body() = "Some random delete data";
        request.prepare_payload(); // set content-length based on the body
        http::write(stream, request);
        http::read(stream, buffer, res);
    }

    void Test()
    {
        constexpr std::string_view host { "0.0.0.0"}, port { "52525"};
        // constexpr std::string_view path { "/index" };
        // constexpr int version { 11 };

        net::io_context ioCtx;
        tcp::resolver resolver(ioCtx);
        beast::tcp_stream stream(ioCtx);

        tcp::resolver::results_type serverEndpoint = resolver.resolve(host, port);

        stream.connect(serverEndpoint);

        // Set up an HTTP GET request message
        http::request<http::string_body> request;
        request.set(http::field::host, "httpbin.cpp.al");
        request.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);

        beast::flat_buffer buffer;
        http::response<http::dynamic_body> response;

        // do_get(stream, request, buffer, response);
        // do_head(stream, request, buffer, response);
        // do_patch(stream, request, buffer, response);
        // do_put(stream, request, buffer, response);
        // do_post(stream, request, buffer, response);
        do_delete(stream, request, buffer, response);
    }
}

namespace Beast::HTTP_Client_Async
{
    class session : public std::enable_shared_from_this<session>
    {
        tcp::resolver resolver;
        beast::tcp_stream stream;
        beast::flat_buffer buffer; // (Must persist between reads)
        http::request<http::empty_body> request;
        http::response<http::string_body> response;

    public:
        // Objects are constructed with a strand to ensure that handlers do not execute concurrently.
        explicit session(net::io_context& ioc): resolver(net::make_strand(ioc)),
                                                stream(net::make_strand(ioc)) {
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

        void on_resolve(const beast::error_code& ec,
                        const tcp::resolver::results_type& results)
        {
            if (ec)
                return fail(ec, "resolve");

            // Set a timeout on the operation
            stream.expires_after(std::chrono::seconds(30));

            // Make the connection on the IP address we get from a lookup
            stream.async_connect(results,
                                  beast::bind_front_handler(&session::on_connect, shared_from_this()));
        }

        void on_connect(const beast::error_code& ec,
                        const tcp::resolver::results_type::endpoint_type&)
        {
            if (ec)
                return fail(ec, "connect");

            // Set a timeout on the operation
            stream.expires_after(std::chrono::seconds(30));

            // Send the HTTP request to the remote host
            http::async_write(stream, request,
                               beast::bind_front_handler(&session::on_write,shared_from_this()));
        }

        void on_write(const beast::error_code& ec,
                      std::size_t bytes_transferred)
        {
            boost::ignore_unused(bytes_transferred);
            if (ec)
                return fail(ec, "write");

            // Receive the HTTP response
            http::async_read(stream, buffer, response,
                              beast::bind_front_handler(&session::on_read, shared_from_this()));
        }

        void on_read(beast::error_code ec,
                     std::size_t bytes_transferred)
        {
            boost::ignore_unused(bytes_transferred);
            if (ec)
                return fail(ec, "read");

            // Write the message to standard out
            std::cout << response << std::endl;

            // Gracefully close the socket
            stream.socket().shutdown(tcp::socket::shutdown_both, ec);

            // not_connected happens sometimes so don't bother reporting it.
            if (ec && ec != beast::errc::not_connected)
                return fail(ec, "shutdown");
        }

        void fail(beast::error_code ec, char const* what) {
            std::cerr << what << ": " << ec.message() << "\n";
        }
    };

    void Send_Request()
    {
        constexpr std::string_view host { "0.0.0.0"}, port { "52525"};
        constexpr std::string_view path { "/index" };
        constexpr int version { 11 };

        // The io_context is required for all I/O
        net::io_context ioc;

        // Launch the asynchronous operation
        std::make_shared<session>(ioc)->run(host, port, path, version);

        // Run the I/O service. The call will return when the get operation is complete.
        ioc.run();
    }

}

void Beast::TestAll([[maybe_unused]] const std::vector<std::string_view>& params)
{
    // Client::SimpleHttpGetRequest();
    // Client::SSL_Request_Test();
    // HTTP_Methods::Test();

    HTTP_Client_Async::Send_Request();
}