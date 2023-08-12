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

#include <thread>
#include <chrono>

#include <functional>

#include <boost/timer/timer.hpp>

#include <boost/version.hpp>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/thread.hpp>
#include <boost/uuid/detail/md5.hpp>
#include <boost/algorithm/hex.hpp>

#include <boost/asio/ssl.hpp>

using namespace boost::timer;

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

void Timer::TestAll()
{
    // start_Resume();

    Callback::testCallback();
};