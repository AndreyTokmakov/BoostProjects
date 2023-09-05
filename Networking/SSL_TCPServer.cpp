/**============================================================================
Name        : SSL_TCPServer.cpp
Created on  : 27.08.2023
Author      : Andrei Tokmakov
Version     : 1.0
Copyright   : Your copyright notice
Description : SSL_TCPServer
============================================================================**/

#include "SSL_TCPServer.h"

#include <iostream>
#include <string_view>

#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/thread.hpp>
#include <boost/algorithm/hex.hpp>
#include <boost/asio/ssl.hpp>

namespace
{
    namespace asio = boost::asio;
    namespace ip = asio::ip;
    namespace ssl = asio::ssl;

    using tcp = ip::tcp;
    using namespace std::string_view_literals;


    constexpr std::string_view host { "0.0.0.0." };
    constexpr uint16_t port { 8443 };
}


namespace SSL_TCPServer::SSL1
{

    using ssl_socket = ssl::stream<tcp::socket>;

    class session
    {
    public:
        session(asio::io_service& io_service, ssl::context& context) :
                socket(io_service, context) {
        }

        ssl_socket::lowest_layer_type& getSocket() {
            return socket.lowest_layer();
        }

        void start()
        {
            socket.async_handshake(ssl::stream_base::server,
                                    boost::bind(&session::handle_handshake,this,asio::placeholders::error));
        }

        void handle_handshake(const boost::system::error_code& error)
        {
            if (!error) {
                socket.async_read_some(
                        asio::buffer(data, max_length),
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
                std::cout <<"read: " << std::string(data, bytes_transferred) << std::endl;
                asio::async_write(socket,
                                  asio::buffer(data, bytes_transferred),
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
                socket.async_read_some(asio::buffer(data, max_length),
                                        boost::bind(&session::handle_read, this,
                                                    asio::placeholders::error,
                                                    asio::placeholders::bytes_transferred));
            }
            else
            {
                delete this;
            }
        }

        ~session()
        {
            socket.shutdown();
        }

    private:
        ssl_socket socket;
        static inline constexpr size_t max_length { 1024 };
        char data[max_length] {};
    };

    class server
    {
    public:
        server(asio::io_service& io_service, unsigned short port):
                ioService { io_service },
                acceptor {io_service,tcp::endpoint(tcp::v4(), port)},
                context {ssl::context::sslv23 }
        {
            context.set_options(ssl::context::default_workarounds |
                                 ssl::context::no_sslv2 |
                                 ssl::context::single_dh_use);
            context.set_password_callback(boost::bind(&server::get_password, this));
            context.use_certificate_chain_file("../data/server.pem");
            context.use_private_key_file("../data/key.pem", ssl::context::pem);
            // context_.use_tmp_dh_file("dh2048.pem");

            start_accept();
        }

        [[nodiscard]]
        std::string get_password() const {
            return std::string { "test" };
        }

        void start_accept()
        {
            session* new_session = new session(ioService, context);
            acceptor.async_accept(new_session->getSocket(),
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
        asio::io_service& ioService;
        tcp::acceptor acceptor;
        ssl::context context;
    };

    // How to create a self-signed PEM file:
    // openssl req -newkey rsa:2048 -new -nodes -x509 -days 3650 -keyout key.pem -out cert.pem
    //

    void runServer()
    {
        try
        {
            asio::io_service ioService;
            server s(ioService, port);
            ioService.run();
        }
        catch (std::exception& e)
        {
            std::cerr << "Exception: " << e.what() << "\n";
        }
    }
}

namespace SSL_TCPServer::SSL2
{
    struct Service
    {
        void handle_client(ssl::stream<tcp::socket>& ssl_stream)
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
                ioService { ios },
                acceptor {ioService,tcp::endpoint(ip::address_v4::any(),port_num) },
                context {ssl::context::sslv23_server }
        {
            // Setting up the context.
            context.set_options(ssl::context::default_workarounds |
                                   ssl::context::no_sslv2 |
                                   ssl::context::single_dh_use);
            context.set_password_callback([this](std::size_t max_length,
                                                    ssl::context::password_purpose purpose)-> std::string {
                return getPassword(max_length, purpose).data();
            });

            context.use_certificate_chain_file("../data/server.pem");
            context.use_private_key_file("../data/key.pem", ssl::context::pem);
            // sslContext.use_tmp_dh_file("dh2048.pem");

            // Start listening for incoming connection requests.
            acceptor.listen();
        }

        void accept()
        {
            ssl::stream<tcp::socket>ssl_stream(ioService, context);
            acceptor.accept(ssl_stream.lowest_layer());
            Service svc;
            svc.handle_client(ssl_stream);
        }

    private:

        [[nodiscard]]
        std::string_view getPassword([[maybe_unused]] std::size_t max_length,
                                     [[maybe_unused]] ssl::context::password_purpose purpose) const
        {
            return password;
        }

    private:
        inline static constexpr std::string_view password { "pass" };

        asio::io_service& ioService;
        tcp::acceptor acceptor;
        ssl::context context;
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
        try {
            Server srv;
            srv.start(port);
            std::this_thread::sleep_for(std::chrono::seconds(60));
            srv.stop();
        }
        catch (boost::system::system_error &e) {
            std::cout << "Error occured! Error code = " << e.code() << ". Message: " << e.what();
        }
    }
}

void SSL_TCPServer:: TestAll()
{
    // SSL1::runServer();
    SSL2::runServer();
};