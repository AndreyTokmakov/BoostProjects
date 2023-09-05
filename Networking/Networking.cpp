/**============================================================================
Name        : Networking.cpp
Created on  : 14.05.2023
Author      : Andrei Tokmakov
Version     : 1.0
Copyright   : Your copyright notice
Description : Networking
============================================================================**/

#include "Networking.h"
#include "UDPEchoServerClient.h"
#include "TCPEchoServerClient.h"
#include "HTTPServer.h"
#include "SSL_TCPServer.h"

#include <iostream>

#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/thread.hpp>
#include <boost/algorithm/hex.hpp>
#include <boost/asio/ssl.hpp>

#include <boost/array.hpp>
#include <memory>
#include <source_location>

namespace asio = boost::asio;
using namespace std::string_view_literals;

namespace Networking::Utilities
{

    void log(const std::string_view message = ""sv,
             const std::source_location location = std::source_location::current())
    {
        std::cout << location.function_name() << ": " << location.line();
        if (!message.empty())
            std::cout << " | " << message;
        std::cout << std::endl;
    }
}


namespace Networking
{
    using asio::ip::tcp;

    std::string make_daytime_string()
    {
        std::time_t now = std::time(0);
        return std::ctime(&now);
    }

    void server()
    {
        try
        {
            asio::io_service io_service;
            tcp::acceptor acceptor(io_service, tcp::endpoint(tcp::v4(), 52525));
            for (;;)
            {
                tcp::socket socket(io_service);
                acceptor.accept(socket);
                std::string message = make_daytime_string();
                boost::system::error_code ignored_error;
                asio::write(socket, asio::buffer(message), ignored_error);
            }
        }
        catch (std::exception& e)
        {
            std::cerr << e.what() << std::endl;
        }
    }

    void client()
    {
        try
        {
            asio::io_service io_service;
            tcp::resolver resolver(io_service);

            tcp::resolver::query query("0.0.0.0", "daytime");
            tcp::resolver::iterator endpoint_iterator = resolver.resolve(query);

            tcp::socket socket(io_service);
            asio::connect(socket, endpoint_iterator);

            while (true) {
                boost::array<char, 128> buf {};
                boost::system::error_code error;

                const size_t len = socket.read_some(asio::buffer(buf), error);
                if (error == asio::error::eof)
                    break; // Connection closed cleanly by peer.
                else if (error)
                    throw boost::system::system_error(error); // Some other error.

                std::cout.write(buf.data(), len);
            }
        }
        catch (std::exception& e)
        {
            std::cerr << e.what() << std::endl;
        }
    }
}

namespace Networking::HTTP
{
    constexpr std::string_view host { "0.0.0.0." };
    constexpr std::string_view path { "/debug" };

    void sendRequestGET()
    {
        try {
            asio::io_context io_context;

            tcp::resolver resolver(io_context);
            tcp::resolver::results_type endpoints = resolver.resolve(host, "52525");

            tcp::socket socket(io_context);
            asio::connect(socket, endpoints);

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

namespace Networking::Basics
{
    namespace ip = boost::asio::ip;
    using tcp = boost::asio::ip::tcp;

    void createAcceptorSocket()
    {
        //An instance of 'io_service' class is required by socket constructor.
        asio::io_service ios;

        // Creating an object of 'tcp' class representing a TCP protocol with IPv6 as underlying protocol.
        ip::tcp protocol = asio::ip::tcp::v6();

        // Instantiating an acceptor socket object.
        ip::tcp::acceptor acceptor(ios);

        // Used to store information about error that happens while opening the acceptor socket.
        boost::system::error_code ec;

        //  Opening the acceptor socket.
        acceptor.open(protocol, ec);
        if (ec.value() != 0) {
            // Failed to open the socket.
            std::cout << "Failed to open the acceptor socket! Error code = "
                      << ec.value() << ". Message: " << ec.message();
        }
    }

    void acceptConnections()
    {
        constexpr uint16_t port = 52525, backlog = 10;

        asio::io_service ios; // An instance of 'io_service' class is required by socket constructor.
        tcp::endpoint ep(ip::address_v4::any(), port); // Creating a server endpoint.

        try
        {
            tcp::acceptor acceptor(ios, ep.protocol()); // Instantiating an acceptor socket object.
            acceptor.bind(ep);      // Binding the acceptor socket to the server endpoint.
            acceptor.listen(backlog);       // Starting to listen for incoming connection requests.
            tcp::socket clientSock(ios); // Creating an active socket.
            acceptor.accept(clientSock); // Accept the incoming connecting the active socket to the client.

            std::cout << "Connection accepted. Exiting...\n";
        }
        catch (const boost::system::error_code& exc)
        {
            std::cout << "Error occurred! Error code = " << exc.value() << ". Message: " << exc.message();
        }
    }

    void resolveDNS()
    {
        constexpr std::string_view host = "google.com";
        constexpr std::string_view port_num = "443";

        asio::io_service ios;
        asio::ip::tcp::resolver::query resolver_query(host.data(),port_num.data(),
                                                      asio::ip::tcp::resolver::query::numeric_service);
        asio::ip::tcp::resolver resolver(ios);
        boost::system::error_code ec;
        asio::ip::tcp::resolver::iterator it = resolver.resolve(resolver_query, ec);
        if (ec.value() != 0) {
            std::cout << "Failed to resolve a DNS name."
                      << "Error code = " << ec.value()
                      << ". Message = " << ec.message();
            return;
        }

        const asio::ip::tcp::resolver::iterator it_end;
        for (; it != it_end; ++it) {
            asio::ip::tcp::endpoint ep = it->endpoint();
            std::cout << ep << std::endl;
        }
    }

    void connectToSocket()
    {
        constexpr std::string_view host { "0.0.0.0"sv };
        constexpr uint16_t portNum { 52525 };

        try {
            asio::ip::tcp::endpoint server(asio::ip::address::from_string(host.data()),portNum);
            asio::io_service ios;
            asio::ip::tcp::socket sock(ios, server.protocol());
            sock.connect(server);
        }
        catch (boost::system::system_error &e) {
            std::cout << "Error occurred! Error code = " << e.code() << ". Message: " << e.what();
        }
    }

    void write_to_socket(asio::ip::tcp::socket& sock)
    {
        constexpr std::string_view msg { "Hello"sv };
        std::size_t bytesSend = 0;

        // Run the loop until all data is written to the socket.
        while (bytesSend != msg.length()) {
            bytesSend += sock.write_some(
                    asio::buffer(msg.data() + bytesSend, msg.length() - bytesSend)
            );
        }
    }

    void writeToSocket()
    {
        constexpr std::string_view host { "0.0.0.0"sv };
        constexpr uint16_t portNum { 52525 };

        try {
            asio::ip::tcp::endpoint server(asio::ip::address::from_string(host.data()),portNum);
            asio::io_service ios;
            asio::ip::tcp::socket sock(ios, server.protocol());
            sock.connect(server);

            write_to_socket(sock);
        }
        catch (boost::system::system_error &e) {
            std::cout << "Error occurred! Error code = " << e.code() << ". Message: " << e.what();
            //return e.code().value();
        }
    }

    std::string read_from_socket_all(asio::ip::tcp::socket& sock)
    {
        constexpr size_t replyLen = 128;
        std::array<char, replyLen> buffer {};

        size_t bytesRead = sock.read_some(asio::buffer(buffer));
        return std::string { buffer.data(), bytesRead };
    }

    std::string read_from_socket_size(asio::ip::tcp::socket& sock)
    {
        constexpr size_t replyLen = 5;
        std::array<char, replyLen> buffer {};

        size_t bytesRead = asio::read(sock, asio::buffer(buffer, replyLen));
        return std::string { buffer.data(), bytesRead };
    }

    std::string read_from_socket_until_delim(asio::ip::tcp::socket& sock, const char delim = '\n')
    {
        asio::streambuf buf;
        asio::read_until(sock, buf, delim);
        std::string message;
        std::istream input_stream(&buf);
        std::getline(input_stream, message);
        return message;
    }

    void writeToSocketAndRead()
    {
        constexpr std::string_view host { "0.0.0.0"sv };
        constexpr uint16_t portNum { 52525 };

        try {
            asio::ip::tcp::endpoint server(asio::ip::address::from_string(host.data()),portNum);
            asio::io_service ios;
            asio::ip::tcp::socket sock(ios, server.protocol());
            sock.connect(server);

            write_to_socket(sock);

            std::string reply = read_from_socket_all(sock);
            // std::string reply = read_from_socket_size(sock);

            std::cout << reply << std::endl;
        }
        catch (boost::system::system_error &e) {
            std::cout << "Error occurred! Error code = " << e.code() << ". Message: " << e.what();
            //return e.code().value();
        }
    }

    void writeToSocketAndReadUntil()
    {
        constexpr std::string_view host { "0.0.0.0"sv };
        constexpr uint16_t portNum { 52525 };

        try {
            asio::ip::tcp::endpoint server(asio::ip::address::from_string(host.data()),portNum);
            asio::io_service ios;
            asio::ip::tcp::socket sock(ios, server.protocol());
            sock.connect(server);

            write_to_socket(sock);

            std::string reply = read_from_socket_until_delim(sock, '|');
            std::cout << reply << std::endl;
        }
        catch (boost::system::system_error &e) {
            std::cout << "Error occurred! Error code = " << e.code() << ". Message: " << e.what();
            //return e.code().value();
        }
    }
}

/*
namespace SSL3
{

    using asio::ip::tcp;

    void handle_request(asio::ssl::stream<tcp::socket>& socket,
                        asio::streambuf& buffer)
    {
        std::istream request_stream(&buffer);
        std::string request_line;
        std::getline(request_stream, request_line);

        std::cout << "Request: " << request_line << std::endl;

        std::string response = "HTTP/1.1 200 OK\r\nContent-Length: 13\r\n\r\nHello, World!";
        boost::asio::write(socket, boost::asio::buffer(response));
    }

    void start_ssl_server()
    {
        asio::io_service io_service;

        asio::ssl::context context(boost::asio::ssl::context::sslv23);

        context.use_certificate_file("server.crt", asio::ssl::context::pem);
        context.use_private_key_file("server.key", asio::ssl::context::pem);

        asio::ip::tcp::acceptor acceptor(io_service, asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), 4443));

        asio::ip::tcp::socket socket(io_service);
        acceptor.accept(socket);

        asio::ssl::stream<asio::ip::tcp::socket> ssl_socket(io_service, context);

        ssl_socket.handshake(asio::ssl::stream_base::server, error);

        asio::streambuf buffer;
        asio::read_until(ssl_socket, buffer, "\r\n");

        handle_request(ssl_socket, buffer);

        ssl_socket.shutdown();
    }

    int main()
    {
        try
        {
            start_ssl_server();
        }
        catch (std::exception& e)
        {
            std::cerr << "Exception: " << e.what() << std::endl;
        }

        return 0;
    }
}
 */


// TODO: https://drive.google.com/file/d/0B1lMPPfEr07IRmFIVGhUOFVTekE/preview?resourcekey=0-gAdn0cm3jM05vZmkIxOqCg
void Networking::TestAll()
{

    // Basics::createAcceptorSocket();
    // Basics::acceptConnections();
    // Basics::resolveDNS();
    // Basics::connectToSocket();
    // Basics::writeToSocket();
    // Basics::writeToSocketAndRead();
    // Basics::writeToSocketAndReadUntil();

    // Networking::server();
    // Networking::client();
    // Networking::SSL::runServer();

    // Networking::HTTP::sendRequestGET();


    // UDPEchoServerClient::TestAll();
    // TCPEchoServerClient::TestAll();

    // HTTPServer::TestAll();

    SSL_TCPServer::TestAll();

    /*
    constexpr std::string_view raw_ip_address { "0.0.0.0" };
    constexpr uint16_t port_num = 10525;

    asio::ip::tcp::endpoint ep(asio::ip::address::from_string(raw_ip_address.data()), port_num);
    */
};
