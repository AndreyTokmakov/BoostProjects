/**============================================================================
Name        : Serial.h
Created on  : 11.03.2023.
Author      : Andrei Tokmakov
Version     : 1.0
Copyright   : Your copyright notice
Description : C++ serial
============================================================================**/

#include <boost/asio.hpp>

#include <algorithm>
#include <cctype>
#include <chrono>
#include <iostream>
#include <stdexcept>
#include <string>
#include <thread>

#include "Serial.h"

namespace Serial
{
    namespace asio = boost::asio;

    const char *const clearDisplay {"\x1b[J"};
    const char *const clearEol {"\x1b[K"};
    const char *const reverseVideo {"\x1b[7m"};
    const char *const normalVideo {"\x1b[0m"};
    const char *const graphicCharSet {"\x1b(0"};
    const char *const normalCharSet {"\x1b(B"};
    constexpr char graphicDiamond {'\x60'};

    std::string rowCol(int row, int col)
    {
        return "\x1b[" + std::to_string(row) + ';' + std::to_string(col) + 'H';
    }

    class Service
    {
    public:
        explicit Service(asio::io_context &ctx): m_ctx(ctx),
                    m_timer(ctx, asio::chrono::seconds(1)),
                    m_port(ctx)
        {
            post(m_ctx,
                 []
                 {
                     std::cout << (clearDisplay +
                                    rowCol(10, 3) + "Input:" +
                                    rowCol(12, 3) + "Value:" +
                                    rowCol(14, 30) + "Type Ctrl+C to exit.");
                 });
            m_timer.async_wait([this](const boost::system::error_code &ec)
                               { timerExpired(ec); });
        }


        void stop()
        {
            m_timer.cancel();
            m_port.cancel();
            m_stop = true;
        }

        void input(char c)
        {
            post(m_ctx,
                 [c, this]
                 {
                     if (m_charCount > 60)
                     {
                         m_charCount = 0;
                     }
                     std::string output = rowCol(10, 10 + m_charCount++) + clearEol;
                     if (std::isprint(c))
                     {
                         output += c;
                     }
                     else if (std::iscntrl(c))
                     {
                         output += reverseVideo;
                         output += static_cast<char>(c | 0x40);
                         output += normalVideo;
                     }
                     else
                     {
                         output += graphicCharSet;
                         output += graphicDiamond;
                         output += normalCharSet;
                     }
                     std::cout << output;
                 });
        }

        void openSerialPort(const std::string &device)
        {
            m_port.open(device);
            m_port.set_option(asio::serial_port::baud_rate(115200));
            readLine();
        }

    private:

        void timerExpired(const boost::system::error_code &ec)
        {
            if (ec || m_stop) {
                return;
            }

            std::time_t now = std::time(nullptr);
            std::cout << rowCol(1, 40) + clearEol + std::ctime(&now);
            m_timer.expires_after(asio::chrono::seconds(1));
            m_timer.async_wait([this](const boost::system::error_code &ec){
                timerExpired(ec);
            });
        }

        void readLine()
        {
            async_read_until(m_port, m_serialData, "\n",
                             [this](const boost::system::error_code &ec,std::size_t size) {
                lineReceived(ec, size);
            });
        }


        void lineReceived(const boost::system::error_code &ec,
                          [[maybe_unused]] std::size_t size)
        {
            if (ec)
                return;

            std::istream str(&m_serialData);
            unsigned int value;
            str >> value;
            if (str.good())
            {
                value = std::min(64U, (value >> 4) + 1); // map 0-1023 to 1-64
                std::string boxes;
                boxes.assign(value, graphicDiamond);

                // FIXME: Performance
                std::cout << rowCol(12, 10) + graphicCharSet + boxes + normalCharSet + clearEol;
            }

            readLine();
        }

        asio::io_context &m_ctx;
        asio::steady_timer m_timer;
        asio::serial_port m_port;
        asio::streambuf m_serialData;
        bool m_stop{};
        int m_charCount{};
    };

    void getConsoleInput(Service &svc)
    {
        static const int CTRL_C = 3;

        while (true)
        {
            int c = getchar();
            if (c == CTRL_C)
            {
                break;
            }

            svc.input(static_cast<char>(c));
        }
    }

}

void Serial::TestAll()
{
    try
    {
        asio::io_context ctx;
        Service svc(ctx);
        svc.openSerialPort("/dev/ttyUSB0");

        std::thread thread(
                [&svc]
                {
                    getConsoleInput(svc);
                    svc.stop();
                });

        ctx.run();
        thread.join();
    }
    catch (const std::exception &bang)
    {
        std::cout << rowCol(24, 1);
        std::cerr << bang.what() << '\n';
    }
    catch (...)
    {
        std::cout << rowCol(24, 1);
        std::cerr << "Unknown error\n";
    }

    std::cout << rowCol(24, 1);
}