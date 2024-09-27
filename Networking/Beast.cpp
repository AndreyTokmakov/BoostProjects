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

namespace Beast::Client
{
    namespace beast = boost::beast;
    namespace http = beast::http;
    namespace net = boost::asio;
    namespace ssl = boost::asio::ssl;
    namespace json = boost::json;
    namespace ip = boost::asio::ip;
    using tcp = net::ip::tcp;


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
        if(!SSL_set_tlsext_host_name(stream.native_handle(), host.c_str())) {
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



void Beast::TestAll([[maybe_unused]] const std::vector<std::string_view>& params)
{
    // Client::SimpleHttpGetRequest();
    Client::SSL_Request_Test();
}