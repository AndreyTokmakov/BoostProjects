/**============================================================================
Name        : HTTPS_Server.cpp
Created on  : 17.01.2025
Author      : Andrei Tokmakov
Version     : 1.0
Copyright   : Your copyright notice
Description : HTTPS_Server.cpp
============================================================================**/

#include "HTTPS_Server.h"

#include <iostream>
#include <string_view>
#include <array>
#include <source_location>
#include <thread>

#include <boost/config.hpp>
#include <boost/asio.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/asio/ssl/stream.hpp>

#include <boost/beast.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>

#include "server_certificate.hpp"
#include "root_certificates.hpp"


namespace HTTPS_Server
{
    using namespace std::string_view_literals;

    namespace asio  = boost::asio;
    namespace beast = boost::beast;
    namespace http  = beast::http;
    namespace ssl   = asio::ssl;
    namespace ip   = asio::ip;

    using tcp = ip::tcp;
    using udp = ip::udp;
}

namespace HTTPS_Server
{
    constexpr std::string_view host { "0.0.0.0" };
    constexpr int32_t port { 8443 };


    // TODO: Refactor using std::string_view && replace 'iequals'
    beast::string_view mime_type(beast::string_view path)
    {
        using beast::iequals;
        auto const ext = [&path]
        {
            auto const pos = path.rfind(".");
            if(pos == beast::string_view::npos)
                return beast::string_view{};
            return path.substr(pos);
        }();
        if(iequals(ext, ".htm"))  return "text/html";
        if(iequals(ext, ".html")) return "text/html";
        if(iequals(ext, ".php"))  return "text/html";
        if(iequals(ext, ".css"))  return "text/css";
        if(iequals(ext, ".txt"))  return "text/plain";
        if(iequals(ext, ".js"))   return "application/javascript";
        if(iequals(ext, ".json")) return "application/json";
        if(iequals(ext, ".xml"))  return "application/xml";
        if(iequals(ext, ".swf"))  return "application/x-shockwave-flash";
        if(iequals(ext, ".flv"))  return "video/x-flv";
        if(iequals(ext, ".png"))  return "image/png";
        if(iequals(ext, ".jpe"))  return "image/jpeg";
        if(iequals(ext, ".jpeg")) return "image/jpeg";
        if(iequals(ext, ".jpg"))  return "image/jpeg";
        if(iequals(ext, ".gif"))  return "image/gif";
        if(iequals(ext, ".bmp"))  return "image/bmp";
        if(iequals(ext, ".ico"))  return "image/vnd.microsoft.icon";
        if(iequals(ext, ".tiff")) return "image/tiff";
        if(iequals(ext, ".tif"))  return "image/tiff";
        if(iequals(ext, ".svg"))  return "image/svg+xml";
        if(iequals(ext, ".svgz")) return "image/svg+xml";
        return "application/text";
    }

    void fail(const beast::error_code& errorCode,
              char const* what)
    {
        std::cerr << what << ": " << errorCode.message() << "\n";
    }

    // Append an HTTP rel-path to a local filesystem path -> returned path is normalized for the platform.
    std::string path_cat(std::string_view base,
                         std::string_view path)
    {
        if (base.empty())
            return std::string(path);
        std::string result(base);

        constexpr char path_separator = '/';
        if (result.back() == path_separator)
            result.resize(result.size() - 1);
        result.append(path.data(), path.size());
        return result;
    }


    // Return a response for the given request.
    // The concrete type of the response message (which depends on the request), is type-erased in message_generator.
    template <class Body, class Allocator>
    http::message_generator handle_request(std::string_view doc_root,
                                           http::request<Body, http::basic_fields<Allocator>>&& request)
    {
        // Returns a bad request response
        const auto bad_request = [&request](beast::string_view why) {
            http::response<http::string_body> response { http::status::bad_request, request.version() };
            response.set(http::field::server, BOOST_BEAST_VERSION_STRING);
            response.set(http::field::content_type, "text/html");
            response.keep_alive(request.keep_alive());
            response.body() = std::string(why);
            response.prepare_payload();
            return response;
        };

        // Returns a not found response
        const auto not_found = [&request](beast::string_view target) {
            http::response<http::string_body> response { http::status::not_found, request.version() };
            response.set(http::field::server, BOOST_BEAST_VERSION_STRING);
            response.set(http::field::content_type, "text/html");
            response.keep_alive(request.keep_alive());
            response.body() = "The resource '" + std::string(target) + "' was not found.";
            response.prepare_payload();
            return response;
        };

        // Returns a server error response
        const auto server_error = [&request](beast::string_view what) {
            http::response<http::string_body> response { http::status::internal_server_error, request.version() };
            response.set(http::field::server, BOOST_BEAST_VERSION_STRING);
            response.set(http::field::content_type, "text/html");
            response.keep_alive(request.keep_alive());
            response.body() = "An error occurred: '" + std::string(what) + "'";
            response.prepare_payload();
            return response;
        };

        // Make sure we can handle the method
        if (http::verb::get != request.method() && http::verb::head != request.method())
            return bad_request("Unknown HTTP-method");

        // Request path must be absolute and not contain "..".
        if (request.target().empty() || request.target()[0] != '/' || request.target().find("..") != beast::string_view::npos)
            return bad_request("Illegal request-target");

        // Build the path to the requested file
        std::string path = path_cat(doc_root, request.target());
        if ('/' == request.target().back())
            path.append("index.html");

        // Attempt to open the file
        beast::error_code errorCode;
        http::file_body::value_type body;
        body.open(path.c_str(), beast::file_mode::scan, errorCode);

        // Handle the case where the file doesn't exist
        if (beast::errc::no_such_file_or_directory == errorCode)
            return not_found(request.target());

        // Handle an unknown error
        if (errorCode) {
            return server_error(errorCode.message());
        }

        // Cache the size since we need it after the move
        const uint16_t size = body.size();

        // Respond to HEAD request
        if (request.method() == http::verb::head)
        {
            http::response<http::empty_body> res{http::status::ok, request.version()};
            res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
            res.set(http::field::content_type, mime_type(path));
            res.content_length(size);
            res.keep_alive(request.keep_alive());
            return res;
        }

        // Respond to GET request
        http::response<http::file_body> response { std::piecewise_construct,
            std::make_tuple(std::move(body)),
            std::make_tuple(http::status::ok, request.version())
        };
        response.set(http::field::server, BOOST_BEAST_VERSION_STRING);
        response.set(http::field::content_type, mime_type(path));
        response.content_length(size);
        response.keep_alive(request.keep_alive());
        return response;
    }


    void do_session(tcp::socket& socket,
                    ssl::context& ctx,
                    std::string_view docRoot) {

        ssl::stream<tcp::socket &> stream{socket, ctx};

        // Perform the SSL handshake
        beast::error_code errorCode;
        stream.handshake(ssl::stream_base::server, errorCode);
        if (errorCode) {
            return fail(errorCode, "handshake");
        }

        // This buffer is required to persist across reads
        beast::flat_buffer buffer;
        while (true)
        {
            // Read a request
            http::request<http::string_body> req;
            http::read(stream, buffer, req, errorCode);
            if (http::error::end_of_stream == errorCode)
                break;
            if (errorCode) {
                return fail(errorCode, "read");
            }

            // Handle request
            http::message_generator message = handle_request(docRoot, std::move(req));
            // Determine if we should close the connection
            bool keep_alive = message.keep_alive();

            // Send the response
            beast::write(stream, std::move(message), errorCode);

            if (errorCode) {
                return fail(errorCode, "write");
            }
            if (!keep_alive) {
                // This means we should close the connection, usually because
                // the response indicated the "Connection: close" semantic.
                break;
            }
        }

        // Perform the SSL shutdown
        stream.shutdown(errorCode);
        if (errorCode) {
            return fail(errorCode, "shutdown");
        }
    }

    int runServer()
    {
        constexpr std::string_view docRoot { "/"sv };
        try
        {
            asio::io_context ioContext { 1 };
            ssl::context ctx { ssl::context::tlsv13 }; // The SSL context is required, and holds certificates
            load_server_certificate(ctx);               // This holds the self-signed certificate used by the server

            // The acceptor receives incoming connections
            tcp::acceptor acceptor { ioContext, { ip::make_address(host), port } };
            while (true)
            {
                // This will receive the new connection
                tcp::socket clientSocket { ioContext };
                acceptor.accept(clientSocket);

                // Launch the session, transferring ownership of the socket
                std::thread {
                    std::bind(&do_session, std::move(clientSocket), std::ref(ctx), docRoot)
                }.detach();

                /*
                std::thread{[&clientSocket, &ctx, docRoot] {
                    auto s = std::move(clientSocket);
                    do_session(std::move(s), ctx, docRoot); }
                }.detach();*/
            }
        }
        catch (const std::exception& e)
        {
            std::cerr << "Error: " << e.what() << std::endl;
            return EXIT_FAILURE;
        }
    }
}

void HTTPS_Server::TestAll()
{
    HTTPS_Server::runServer();
}