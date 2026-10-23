/**============================================================================
Name        : Timer.h
Created on  : 03.09.2022
Author      : Tokmakov Andrey
Version     : 1.0
Copyright   : Your copyright notice
Description : Timer
============================================================================**/

#include "Timer.h"

#include <iostream>
#include <string_view>
#include <syncstream>
#include <thread>
#include <chrono>
#include <functional>
#include <thread>
#include <chrono>


#include <boost/version.hpp>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/thread.hpp>
#include <boost/timer/timer.hpp>
#include <boost/version.hpp>
#include <boost/uuid/detail/md5.hpp>
#include <boost/algorithm/hex.hpp>
#include <boost/asio/ssl.hpp>

using namespace boost::timer;

namespace
{
    using namespace std::chrono;

    constexpr std::string_view formatMSeconds { "%d-%02d-%02d %02d:%02d:%02d.%06ld" };

    [[maybe_unused]]
    [[nodiscard]]
    std::string getCurrentTime(const time_point<system_clock>& timestamp = system_clock::now())
    {
        const time_t time { system_clock::to_time_t(timestamp) };
        std::tm tm {};
        ::localtime_r(&time, &tm);

        std::string buffer(64, '\0');
        const int32_t size = std::sprintf(buffer.data(), formatMSeconds.data(),
                                          tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec,
                                          duration_cast<microseconds>(timestamp - time_point_cast<seconds>(timestamp)).count()
        );
        buffer.resize(size);
        buffer.shrink_to_fit();
        return buffer;
    }

    #define LOG std::osyncstream { std::cout } << getCurrentTime() << ' '
    #define ERR std::osyncstream { std::cerr } << getCurrentTime() << ' '
}

namespace Timer
{
    void start_Resume()
    {
        cpu_timer timer;

        std::cout << timer.format() << '\n';
        std::this_thread::sleep_for(std::chrono::seconds(2));
        timer.stop();

        std::this_thread::sleep_for(std::chrono::seconds(2));
        std::cout << timer.format() << '\n';

        timer.resume();

        std::this_thread::sleep_for(std::chrono::seconds(2));
        std::cout << timer.format() << '\n';

        std::this_thread::sleep_for(std::chrono::seconds(100));
    }
};

namespace Timer::Callback
{
    class handler
    {
    public:
        explicit handler(boost::asio::io_service& io)
                : m_timer(io, boost::posix_time::seconds(1)), m_count(0)
        {
            m_timer.async_wait(boost::bind(&handler::message, this));
        }

        ~handler() {
            std::cout << "The last count : " << m_count << "\n";
        }

        void message()
        {
            if (m_count < 5)
            {
                std::cout << m_count << "\n";
                ++m_count;

                m_timer.expires_at(m_timer.expires_at() + boost::posix_time::seconds(1));
                m_timer.async_wait(boost::bind(&handler::message, this));
            }
        }

    private:
        boost::asio::deadline_timer m_timer;
        int m_count;
    };

    void testCallback()
    {
        boost::asio::io_service io;
        handler h(io);
        io.run();
    }
}

namespace Timer::one_shot_timer
{
    void run()
    {
        boost::asio::io_context ioCtx;
        boost::asio::steady_timer timer { ioCtx, std::chrono::seconds(2) };
        timer.async_wait( [] (const boost::system::error_code& ec) {
            if (ec == boost::asio::error::operation_aborted) {
                LOG << "Timer canceled\n";
                return;
            }
            if (ec) {
                ERR << "Error: " << ec.message() << '\n';
                return;
            }
            LOG << "The asynchronous timer triggered.\n";
        });

        LOG << "Starting ....\n";
        ioCtx.run();
    }
}

namespace Timer::one_shot_timer_2
{
    void run()
    {
        boost::asio::io_context io;
        boost::asio::steady_timer timer(io);
        timer.expires_after(std::chrono::seconds(2));

        timer.async_wait([](const boost::system::error_code& ec){
            if (ec)
            {
                ERR << "Timer error: " << ec.message() << '\n';
                return;
            }
            LOG << "One-shot timer expired\n";
        });
        LOG << "Timer started\n";
        io.run();
    }
}

namespace Timer::periodic_timer
{
    class PeriodicTimer
    {
        boost::asio::steady_timer timer;
        std::size_t counter = 0;

    public:
        explicit PeriodicTimer(boost::asio::io_context& io): timer(io) {
        }

        void start(){
            schedule();
        }

    private:
        void schedule()
        {
            timer.expires_after(std::chrono::milliseconds(500));
            timer.async_wait([this](const boost::system::error_code& ec)
            {
                if (ec)
                    return;
                LOG << "Tick: " << ++counter << '\n';
                schedule();
            });
        }
    };

    void run()
    {
        boost::asio::io_context io;
        PeriodicTimer timer(io);
        timer.start();

        io.run();
    }
}

namespace Timer::cancellation
{
    void run()
    {
        boost::asio::io_context io;
        boost::asio::steady_timer timer(io);
        timer.expires_after(std::chrono::seconds(5));
        timer.async_wait([](const boost::system::error_code& ec) {
            if (ec == boost::asio::error::operation_aborted){
                LOG << "Timer cancelled\n";
                return;
            }
            if (ec){
                ERR << "Timer error: " << ec.message() << '\n';
                return;
            }
            LOG << "Timeout expired\n";
        });

        timer.cancel();
        io.run();
    }
}

void Timer::TestAll()
{
    // start_Resume();
    // Callback::testCallback();

    // one_shot_timer::run();
    // one_shot_timer_2::run();

    // periodic_timer::run();

    cancellation::run();
};