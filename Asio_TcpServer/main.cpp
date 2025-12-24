/**============================================================================
Name        : main.cpp
Created on  : 24.12.2025
Author      : Andrei Tokmakov
Version     : 1.0
Copyright   : Your copyright notice
Description : Asio_TcpServer
============================================================================**/

#include <iostream>
#include <vector>

#include <boost/asio.hpp>
#include <boost/system/error_code.hpp>

#include <array>
#include <optional>
#include <cstddef>
#include <cstring>

namespace
{
    constexpr std::size_t RX_SIZE = 4096;
    constexpr std::size_t TX_SIZE = 4096;
    constexpr std::size_t MAX_SESSIONS = 128;


    struct Session
    {
        boost::asio::ip::tcp::socket socket;

        alignas(64) std::array<char, RX_SIZE> rx;
        alignas(64) std::array<char, TX_SIZE> tx;

        std::size_t rx_used = 0;
        std::size_t tx_used = 0;

        explicit Session(boost::asio::io_context& io)
            : socket(io)
        {
        }

        void start() {
            do_read();
        }

        void do_read()
        {
            socket.async_read_some(boost::asio::buffer(rx.data() + rx_used, rx.size() - rx_used),
                [this](const boost::system::error_code& ec, std::size_t n) {
                    if (ec) {
                        close();
                        return;
                    }

                    rx_used += n;
                    on_data();
                    do_read();
                }
            );
        }

        void on_data()
        {
            std::memcpy(tx.data(), rx.data(), rx_used);
            tx_used = rx_used;
            rx_used = 0;

            do_write();
        }

        void do_write()
        {
            boost::asio::async_write(socket,boost::asio::buffer(tx.data(), tx_used),
                [this](const boost::system::error_code& ec, std::size_t) {
                    if (ec)
                        close();
                }
            );
        }

        void close()
        {
            boost::system::error_code ec;

            socket.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
            socket.close(ec);
        }
    };

    class SessionPool
    {
        std::array<std::optional<Session>, MAX_SESSIONS> sessions;
        boost::asio::io_context& io;

    public:

        explicit SessionPool(boost::asio::io_context& io): io(io) {
        }

        Session* acquire()
        {
            for (auto& s : sessions)
            {
                if (!s.has_value())
                {
                    s.emplace(io);
                    return &*s;
                }
            }

            return nullptr;
        }

        void release(const Session* session)
        {
            for (auto& s : sessions)
            {
                if (s.has_value() && &*s == session)
                {
                    s.reset();
                    return;
                }
            }
        }
    };

    class Server
    {
        boost::asio::io_context& io;
        boost::asio::ip::tcp::acceptor acceptor;
        SessionPool pool;

    public:
        Server(boost::asio::io_context& io, std::uint16_t port)
            : io(io), acceptor(io, { boost::asio::ip::tcp::v4(), port }) , pool(io) {
        }

        void start() {
            do_accept();
        }

    private:

        void do_accept()
        {
            Session* session = pool.acquire();
            if (!session)
                return;

            acceptor.async_accept(session->socket, [this, session](const boost::system::error_code& ec){
                if (!ec)
                    session->start();
                else
                    pool.release(session);
                do_accept();
            });
        }
    };

    void run()
    {
        boost::asio::io_context io;

        Server server(io, 9000);
        server.start();

        io.run();

    }
}

int main([[maybe_unused]] int argc,
         [[maybe_unused]] char** argv)
{
    const std::vector<std::string_view> args(argv + 1, argv + argc);

    run();

    return EXIT_SUCCESS;
}
