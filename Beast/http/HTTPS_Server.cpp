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


namespace
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


namespace
{
    constexpr std::string_view host{"0.0.0.0"};
    constexpr int32_t port{8443};

    [[maybe_unused]]
    beast::string_view mime_type_old(beast::string_view path) {
        using beast::iequals;
        auto const ext = [&path] -> std::string_view {
            auto const pos = path.rfind(".");
            if (pos == beast::string_view::npos)
                return beast::string_view{};
            return path.substr(pos);
        }();

        if (iequals(ext, ".htm")) return "text/html";
        if (iequals(ext, ".html")) return "text/html";
        if (iequals(ext, ".php")) return "text/html";
        if (iequals(ext, ".css")) return "text/css";
        if (iequals(ext, ".txt")) return "text/plain";
        if (iequals(ext, ".js")) return "application/javascript";
        if (iequals(ext, ".json")) return "application/json";
        if (iequals(ext, ".xml")) return "application/xml";
        if (iequals(ext, ".swf")) return "application/x-shockwave-flash";
        if (iequals(ext, ".flv")) return "video/x-flv";
        if (iequals(ext, ".png")) return "image/png";
        if (iequals(ext, ".jpe")) return "image/jpeg";
        if (iequals(ext, ".jpeg")) return "image/jpeg";
        if (iequals(ext, ".jpg")) return "image/jpeg";
        if (iequals(ext, ".gif")) return "image/gif";
        if (iequals(ext, ".bmp")) return "image/bmp";
        if (iequals(ext, ".ico")) return "image/vnd.microsoft.icon";
        if (iequals(ext, ".tiff")) return "image/tiff";
        if (iequals(ext, ".tif")) return "image/tiff";
        if (iequals(ext, ".svg")) return "image/svg+xml";
        if (iequals(ext, ".svgz")) return "image/svg+xml";
        return "application/text";
    }

    constexpr std::string_view mimeType(beast::string_view path) {
        if (path.ends_with(".htm"sv))
            return "text/html";
        if (path.ends_with(".html"sv))
            return "text/html";
        if (path.ends_with(".php"sv))
            return "text/html";
        if (path.ends_with(".css"sv))
            return "text/css";
        if (path.ends_with(".txt"sv))
            return "text/plain";
        if (path.ends_with(".js"sv))
            return "application/javascript";
        if (path.ends_with(".json"sv))
            return "application/json";
        if (path.ends_with(".xml"sv))
            return "application/xml";
        if (path.ends_with(".swf"sv))
            return "application/x-shockwave-flash";
        if (path.ends_with(".flv"sv))
            return "video/x-flv";
        if (path.ends_with(".png"sv))
            return "image/png";
        if (path.ends_with(".jpe"sv))
            return "image/jpeg";
        if (path.ends_with(".jpeg"sv))
            return "image/jpeg";
        if (path.ends_with(".jpg"sv))
            return "image/jpeg";
        if (path.ends_with(".gif"sv))
            return "image/gif";
        if (path.ends_with(".bmp"sv))
            return "image/bmp";
        if (path.ends_with(".ico"sv))
            return "image/vnd.microsoft.icon";
        if (path.ends_with(".tiff"sv))
            return "image/tiff";
        if (path.ends_with(".tif"sv))
            return "image/tiff";
        if (path.ends_with(".svg"sv))
            return "image/svg+xml";
        if (path.ends_with(".svgz"sv))
            return "image/svg+xml";
        return "application/text";
    }

    void fail(const beast::error_code &errorCode,
              char const *what) {
        if (errorCode == asio::ssl::error::stream_truncated)
            return;
        std::cerr << what << ": " << errorCode.message() << "\n";
    }

    // Append an HTTP rel-path to a local filesystem path -> returned path is normalized for the platform.
    std::string path_cat(std::string_view base,
                         std::string_view path) {
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
            res.set(http::field::content_type, mimeType(path));
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
        response.set(http::field::content_type, mimeType(path));
        response.content_length(size);
        response.keep_alive(request.keep_alive());
        return response;
    }
}


namespace HTTPS_Server_Sync
{
    void do_session(tcp::socket& socket,
                    ssl::context& ctx,
                    std::string_view docRoot)
    {
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

namespace HTTPS_Server_ASync
{
    class session : public std::enable_shared_from_this<session>
    {
        ssl::stream<beast::tcp_stream> tcpStream;
        beast::flat_buffer buffer;
        std::string_view docRoot;
        http::request<http::string_body> request {};

    public:

        // Take ownership of the socket
        explicit session(tcp::socket&& socket,
                         ssl::context& ctx,
                         std::string_view doc_root) : tcpStream { std::move(socket), ctx } , docRoot { doc_root }{
        }

        // Start the asynchronous operation
        void run()
        {
            // We need to be executing within a strand to perform async operations on the I/O objects in this session.
            // not strictly necessary for single-threaded contexts, this example code is written to be thread-safe by default.
            asio::dispatch(tcpStream.get_executor(),
                           beast::bind_front_handler(&session::on_run,shared_from_this()));
        }

        void on_run()
        {
            // Set the timeout.
            beast::get_lowest_layer(tcpStream).expires_after(std::chrono::seconds(30U));
            // Perform the SSL handshake
            tcpStream.async_handshake(ssl::stream_base::server,
                                    beast::bind_front_handler(&session::on_handshake, shared_from_this()));
        }

        void on_handshake(beast::error_code ec)
        {
            if (ec) {
                return fail(ec, "handshake");
            }
            do_read();
        }

        void do_read()
        {
            request.clear();
            // Set the timeout.
            beast::get_lowest_layer(tcpStream).expires_after(std::chrono::seconds(30u));

            // Read a request
            http::async_read(tcpStream, buffer, request,
                             beast::bind_front_handler(&session::on_read,shared_from_this()));
        }

        void on_read(const beast::error_code& errorCode,
                     std::size_t bytes_transferred)
        {
            boost::ignore_unused(bytes_transferred);

            // This means they closed the connection
            if (http::error::end_of_stream == errorCode) {
                return do_close();
            }
            if (errorCode) {
                return fail(errorCode, "read");
            }
            // Send the response
            send_response(handle_request(docRoot, std::move(request)));
        }

        void send_response(http::message_generator&& msg)
        {
            const bool keep_alive = msg.keep_alive();

            // Write the response
            beast::async_write(tcpStream,std::move(msg),
                               beast::bind_front_handler(&session::on_write, this->shared_from_this(), keep_alive));
        }

        void on_write(bool keep_alive,
                      const beast::error_code& errorCode,
                      std::size_t bytes_transferred)
        {
            boost::ignore_unused(bytes_transferred);
            if (errorCode) {
                return fail(errorCode, "write");
            }
            if(! keep_alive)
            {   // This means we should close the connection, usually because
                // the response indicated the "Connection: close" semantic.
                return do_close();
            }

            // Read another request
            do_read();
        }

        void do_close()
        {
            // Set the timeout.
            beast::get_lowest_layer(tcpStream).expires_after(std::chrono::seconds(30U));
            // Perform the SSL shutdown
            tcpStream.async_shutdown(beast::bind_front_handler(&session::on_shutdown, shared_from_this()));
        }

        void on_shutdown(const beast::error_code& errorCode)
        {
            if (errorCode) {
                return fail(errorCode, "shutdown");
            }
            // At this point the connection is closed gracefully
        }
    };

    class Listener : public std::enable_shared_from_this<Listener>
    {
        asio::io_context& ioContext;
        ssl::context& context;
        tcp::acceptor acceptor;
        std::string_view docRoot;

    public:

        Listener(asio::io_context& ioc,
                 ssl::context& ctx,
                 const tcp::endpoint& endpoint,
                 std::string_view doc_root) :
                 ioContext { ioc }, context { ctx }, acceptor { ioc }, docRoot { doc_root }
        {
            beast::error_code errorCode;

            // Open the acceptor
            acceptor.open(endpoint.protocol(), errorCode);
            if (errorCode) {
                fail(errorCode, "open");
                return;
            }

            // Allow address reuse
            acceptor.set_option(asio::socket_base::reuse_address(true), errorCode);
            if (errorCode) {
                fail(errorCode, "set_option");
                return;
            }

            // Bind to the server address
            acceptor.bind(endpoint, errorCode);
            if (errorCode) {
                fail(errorCode, "bind");
                return;
            }

            // Start listening for connections
            acceptor.listen(asio::socket_base::max_listen_connections, errorCode);
            if (errorCode) {
                fail(errorCode, "listen");
                return;
            }
        }

        // Start accepting incoming connections
        void
        run()
        {
            do_accept();
        }

    private:
        void do_accept()
        {
            // The new connection gets its own strand
            acceptor.async_accept(asio::make_strand(ioContext),
                                   beast::bind_front_handler(&Listener::on_accept,shared_from_this()));
        }

        void on_accept(const beast::error_code& errorCode,
                       tcp::socket socket)
        {
            if (errorCode)
            {
                fail(errorCode, "accept");
                return; // To avoid infinite loop
            }
            else
            { // Create the session and run it
                std::make_shared<session>(std::move(socket), context, docRoot)->run();
            }

            // Accept another connection
            do_accept();
        }
    };


    int runServer()
    {
        constexpr std::string_view docRoot { "/"sv };
        constexpr uint32_t threads { 4 };

        try
        {
            asio::io_context ioContext { threads };
            ssl::context ctx {ssl::context::tlsv13 };
            load_server_certificate(ctx);

            const tcp::endpoint serverAddress = tcp::endpoint { ip::make_address(host), port };
            std::make_shared<Listener>(ioContext,ctx, serverAddress, docRoot)->run();

            // Run the I/O service on the requested number of threads
            std::vector<std::thread> workers;
            workers.reserve(threads - 1);
            for (uint32_t i = 0; i < threads - 1; ++i) {
                workers.emplace_back([&ioContext] {
                    ioContext.run();
                });
            }
            ioContext.run();

            return EXIT_SUCCESS;
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
    // HTTPS_Server_Sync::runServer();
    HTTPS_Server_ASync::runServer();
}