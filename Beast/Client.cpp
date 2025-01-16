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
    namespace net = boost::asio;
    namespace ssl = boost::asio::ssl;
    namespace json = boost::json;
    namespace ip = boost::asio::ip;
    using tcp = net::ip::tcp;
}

namespace Client::Simple_Synch
{
    void httpGetRequest()
    {
        constexpr std::string_view host { "0.0.0.0"}, port { "52525"};
        constexpr std::string_view path { "/index" };
        constexpr int version { 11 };

        net::io_context ioCtx;
        beast::tcp_stream stream(ioCtx);

        tcp::resolver resolver(ioCtx);
        tcp::resolver::results_type serverEndpoint = resolver.resolve(host, port);

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
        http::response<http::dynamic_body> response;

        try {
            http::read(stream, buffer, response);
            std::cout << response << std::endl;
        }
        catch (const std::exception& exc) {
            std::cerr << exc.what() << std::endl;
        }

        beast::error_code ec;
        stream.socket().shutdown(tcp::socket::shutdown_both, ec);
    }

}

void Client::TestAll()
{
    Simple_Synch::httpGetRequest();
}