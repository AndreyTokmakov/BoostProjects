/**============================================================================
Name        : Networking.cpp
Created on  : 14.05.2023
Author      : Andrei Tokmakov
Version     : 1.0
Copyright   : Your copyright notice
Description : Networking
============================================================================**/

#include "Networking.h"

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



namespace Networking::SSL1
{

    using ssl_socket = asio::ssl::stream<asio::ip::tcp::socket>;

    class session
    {
    public:
        session(asio::io_service& io_service, asio::ssl::context& context) :
                socket_(io_service, context) {
        }

        ssl_socket::lowest_layer_type& socket() {
            return socket_.lowest_layer();
        }

        void start()
        {
            socket_.async_handshake(
                    asio::ssl::stream_base::server,boost::bind(
                            &session::handle_handshake,this,asio::placeholders::error));
        }

        void handle_handshake(const boost::system::error_code& error)
        {
            if (!error) {
                socket_.async_read_some(
                        asio::buffer(data_, max_length),
                        boost::bind(&session::handle_read, this,
                                    asio::placeholders::error,asio::placeholders::bytes_transferred));
            } else {
                delete this;
            }
        }

        void handle_read(const boost::system::error_code& error,
                         size_t bytes_transferred)
        {
            if (!error) {
                std::cout <<"read: " << std::string(data_, bytes_transferred) << std::endl;
                asio::async_write(socket_,
                                  asio::buffer(data_, bytes_transferred),
                                         boost::bind(&session::handle_write, this,
                                                     asio::placeholders::error));
            } else {
                delete this;
            }
        }

        void handle_write(const boost::system::error_code& error)
        {
            if (!error)
            {
                socket_.async_read_some(asio::buffer(data_, max_length),
                                        boost::bind(&session::handle_read, this,
                                                    asio::placeholders::error,
                                                    asio::placeholders::bytes_transferred));
            }
            else
            {
                delete this;
            }
        }

    private:
        ssl_socket socket_;
        enum { max_length = 1024 };
        char data_[max_length];
    };

    class server
    {
    public:
        server(asio::io_service& io_service, unsigned short port)
                : io_service_(io_service),
                  acceptor_(io_service,
                            asio::ip::tcp::endpoint(asio::ip::tcp::v4(), port)),
                  context_(asio::ssl::context::sslv23)
        {
            context_.set_options(asio::ssl::context::default_workarounds |
                                         asio::ssl::context::no_sslv2 |
                                         asio::ssl::context::single_dh_use);
            context_.set_password_callback(boost::bind(&server::get_password, this));
            context_.use_certificate_chain_file("server.pem");
            context_.use_private_key_file("server.pem", asio::ssl::context::pem);
            context_.use_tmp_dh_file("dh2048.pem");

            start_accept();
        }

        [[nodiscard]]
        std::string get_password() const {
            return std::string { "test" };
        }

        void start_accept()
        {
            session* new_session = new session(io_service_, context_);
            acceptor_.async_accept(new_session->socket(),
                                   boost::bind(&server::handle_accept, this, new_session,
                                               asio::placeholders::error));
        }

        void handle_accept(session* new_session,
                           const boost::system::error_code& error)
        {
            if (!error) {
                new_session->start();
            } else {
                delete new_session;
            }
            start_accept();
        }

    private:
        asio::io_service& io_service_;
        asio::ip::tcp::acceptor acceptor_;
        asio::ssl::context context_;
    };

    void runServer()
    {
        try
        {
            asio::io_service io_service;
            server s(io_service, 52525);
            io_service.run();
        }
        catch (std::exception& e)
        {
            std::cerr << "Exception: " << e.what() << "\n";
        }
    }
}

namespace Networking::SSL2
{
    struct Service
    {
        void handle_client(asio::ssl::stream<asio::ip::tcp::socket>& ssl_stream)
        {
            try {
                // Blocks until the handshake completes.
                ssl_stream.handshake(asio::ssl::stream_base::server);
                asio::streambuf request;
                asio::read_until(ssl_stream, request, '\n');

                // Emulate request processing.
                std::this_thread::sleep_for(std::chrono::milliseconds(500));

                // Sending response.
                std::string response = "Response\n";
                asio::write(ssl_stream, asio::buffer(response));
            }
            catch (boost::system::system_error &e) {
                std::cerr << "Error occured! Error code = " << e.code() << ". Message: " << e.what();
            }
        }
    };

    class Acceptor
    {
    public:
        Acceptor(asio::io_service& ios, unsigned short port_num) :
                m_ios(ios),
                acceptor(m_ios,asio::ip::tcp::endpoint(asio::ip::address_v4::any(),port_num)),
                sslContext(asio::ssl::context::sslv23_server)
        {
            // Setting up the context.
            sslContext.set_options(asio::ssl::context::default_workarounds |
                        asio::ssl::context::no_sslv2 | asio::ssl::context::single_dh_use);
            sslContext.set_password_callback([this](std::size_t max_length,
                    asio::ssl::context::password_purpose purpose)-> std::string {
                return get_password(max_length, purpose);
            });

            sslContext.use_certificate_chain_file("server.crt");
            sslContext.use_private_key_file("server.key",asio::ssl::context::pem);
            sslContext.use_tmp_dh_file("dhparams.pem");

            // Start listening for incoming connection requests.
            acceptor.listen();
        }

        void accept()
        {
            asio::ssl::stream<asio::ip::tcp::socket>ssl_stream(m_ios, sslContext);
            acceptor.accept(ssl_stream.lowest_layer());
            Service svc;
            svc.handle_client(ssl_stream);
        }

    private:

        [[nodiscard]]
        std::string get_password([[maybe_unused]] std::size_t max_length,
                                 [[maybe_unused]] asio::ssl::context::password_purpose purpose) const
        {
            return password;
        }

    private:
        static inline const std::string password { "pass" };

        asio::io_service& m_ios;
        asio::ip::tcp::acceptor acceptor;
        asio::ssl::context sslContext;
    };


    class Server {
    public:
        Server() : m_stop(false) {
        }

        void start(unsigned short port_num) {
            m_thread = std::make_unique<std::thread>([this, port_num]() {
                run(port_num);
            });
        }
        void stop() {
            m_stop.store(true);
            m_thread->join();
        }
    private:
        void run(unsigned short port_num)
        {
            Acceptor acc(m_ios, port_num);
            while (!m_stop.load()) {
                acc.accept();
            }
        }

        std::unique_ptr<std::thread> m_thread;
        std::atomic<bool> m_stop;
        asio::io_service m_ios;
    };

    void runServer()
    {
        constexpr uint16_t portNum { 3333 };
        try {
            Server srv;
            srv.start(portNum);
            std::this_thread::sleep_for(std::chrono::seconds(60));
            srv.stop();
        }
        catch (boost::system::system_error &e) {
            std::cout << "Error occured! Error code = " << e.code() << ". Message: " << e.what();
        }
    }
}


namespace Networking::UDP::Echo
{
    using asio::ip::udp;
    using namespace Utilities;

    class serverAsync
    {
    private:
        static constexpr inline size_t maxLength = 1024;

        udp::socket socket;
        udp::endpoint senderEndpoint;
        char data [maxLength];

    public:
        serverAsync(asio::io_context& io_context, short port):
                socket(io_context, udp::endpoint(udp::v4(), port))
        {
            log();
            do_receive();
        }

        void do_receive()
        {
            log();
            socket.async_receive_from(asio::buffer(data, maxLength), senderEndpoint,
                                      [this](std::error_code ec, std::size_t bytesReceived){
                log(std::to_string(bytesReceived).append(" bytes received"));
                if (!ec && bytesReceived > 0) {
                    do_send(bytesReceived);
                } else {
                    log();
                    do_receive();
                }
            });
        }

        void do_send(std::size_t length)
        {
            log();
            socket.async_send_to(asio::buffer(data, length), senderEndpoint,
                                 [this](std::error_code /*ec*/, std::size_t /*bytes_sent*/){
                do_receive();
            });
        }
    };


    void startServer()
    {
        try {
            asio::io_context io_context;
            serverAsync s(io_context, 10525);
            io_context.run();
        } catch (std::exception& e) {
            std::cerr << "Exception: " << e.what() << "\n";
        }
    }


    void startClient()
    {
        constexpr size_t maxLength = 1024;

        try
        {
            asio::io_context io_context;
            udp::socket s(io_context, udp::endpoint(udp::v4(), 0));

            udp::resolver resolver(io_context);
            udp::endpoint endpoint = *resolver.resolve(udp::v4(), "0.0.0.0", "10525").begin();

            constexpr std::string_view request { "TEST_DATA" };
            s.send_to(asio::buffer(request, request.size()), endpoint);

            char reply[maxLength];
            udp::endpoint sender_endpoint;
            size_t reply_length = s.receive_from(
                    asio::buffer(reply, maxLength), sender_endpoint);
            std::cout << "Reply is: ";
            std::cout.write(reply, reply_length);
            std::cout << "\n";
        }
        catch (std::exception& e)
        {
            std::cerr << "Exception: " << e.what() << "\n";
        }
    }
}

namespace Networking::TCP::Echo
{
    using asio::ip::tcp;
    using namespace Utilities;

    static constexpr inline size_t maxLength = 1024;
    static constexpr inline uint16_t serverPort = 10525;

    class session: public std::enable_shared_from_this<session>
    {
    public:
        explicit session(tcp::socket socket) : socket(std::move(socket)){
        }

        void start() {
            do_read();
        }

    private:
        void do_read()
        {
            log();
            auto self(shared_from_this());
            socket.async_read_some(asio::buffer(data, maxLength),
                                    [this, self](std::error_code ec, std::size_t length){
                if (!ec) {
                    do_write(length);
                }
            });
        }

        void do_write(std::size_t length)
        {
            log();
            auto self(shared_from_this());
            asio::async_write(socket, asio::buffer(data, length),
                              [this, self](std::error_code ec, std::size_t /*length*/){
                if (!ec){
                    do_read();
                }
            });
        }

        tcp::socket socket;
        char data[maxLength]{};
    };

    class server
    {
    public:
        server(asio::io_context& io_context, short port):
            acceptor(io_context, tcp::endpoint(tcp::v4(), port)),
            socket(io_context)
        {
            log();
            do_accept();
        }

    private:
        void do_accept()
        {
            log();
            acceptor.async_accept(socket,[this](std::error_code ec){
                if (!ec) {
                    std::make_shared<session>(std::move(socket))->start();
                }
                do_accept();
            });
        }

        tcp::acceptor acceptor;
        tcp::socket socket;
    };

    void startServer()
    {
        try
        {
            asio::io_context io_context;
            server s(io_context, serverPort);
            io_context.run();
        } catch (std::exception& e) {
            std::cerr << "Exception: " << e.what() << "\n";
        }
    }

    void client()
    {
        try {
            asio::io_context io_context;

            tcp::socket s(io_context);
            tcp::resolver resolver(io_context);
            asio::connect(s, resolver.resolve( "0.0.0.0", std::to_string(serverPort)));


            constexpr std::string_view request { "TEST_DATA" };
            asio::write(s, asio::buffer(request, request.size()));

            char reply[maxLength];
            size_t reply_length = asio::read(s, asio::buffer(reply, request.size()));
            std::cout << "Reply is: ";
            std::cout.write(reply, reply_length);
            std::cout << "\n";
        }
        catch (std::exception& e)
        {
            std::cerr << "Exception: " << e.what() << "\n";
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

    std::string read_from_socket(asio::ip::tcp::socket& sock)
    {
        std::cout << "read_from_socket" << std::endl;
        // std::array<char, 16> buffer {};

        char data[512];
        [[maybe_unused]]
        size_t len = sock.read_some(asio::buffer(data));

        return {};
    }

    std::string read_from_socket2(asio::ip::tcp::socket& sock)
    {
        std::cout << "read_from_socket2" << std::endl;

        std::array<char, 16> buffer {};
        std::size_t bytesRead { 0 };
        asio::read(sock, asio::buffer(buffer.data(), buffer.size()));
        return std::string { buffer.data(), bytesRead };
    }

    void writeAndReadFromSocket()
    {
        constexpr std::string_view host { "0.0.0.0"sv };
        constexpr uint16_t portNum { 52525 };

        try {
            asio::ip::tcp::endpoint server(asio::ip::address::from_string(host.data()),portNum);
            asio::io_service ios;
            asio::ip::tcp::socket sock(ios, server.protocol());
            sock.connect(server);

            write_to_socket(sock);
            read_from_socket(sock);
            // read_from_socket2(sock);
        }
        catch (boost::system::system_error &e) {
            std::cout << "Error occurred! Error code = " << e.code() << ". Message: " << e.what();
            //return e.code().value();
        }
    }
}


// TODO: https://drive.google.com/file/d/0B1lMPPfEr07IRmFIVGhUOFVTekE/preview?resourcekey=0-gAdn0cm3jM05vZmkIxOqCg
void Networking::TestAll()
{

    // Basics::createAcceptorSocket();
    // Basics::acceptConnections();
    // Basics::resolveDNS();
    // Basics::connectToSocket();
    // Basics::writeToSocket();
    Basics::writeAndReadFromSocket();

    // Networking::server();
    // Networking::client();
    // Networking::SSL::runServer();

    // Networking::HTTP::sendRequestGET();


    // UDP::Echo::startServer();
    // UDP::Echo::startClient();

    // TCP::Echo::startServer();
    // TCP::Echo::client();


    // SSL2::runServer();


    /*
    constexpr std::string_view raw_ip_address { "0.0.0.0" };
    constexpr uint16_t port_num = 10525;

    asio::ip::tcp::endpoint ep(asio::ip::address::from_string(raw_ip_address.data()), port_num);
    */
};
