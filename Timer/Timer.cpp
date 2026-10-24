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
    using std::chrono::microseconds;
    using std::chrono::seconds;
    using std::chrono::time_point;
    using std::chrono::system_clock;

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

namespace timer
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

namespace timer::Callback
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

namespace timer::one_shot_timer
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

namespace timer::one_shot_timer_2
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

namespace timer::periodic_timer
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

namespace timer::cancellation
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


namespace timer::heartbeats
{
    class Heartbeat
    {
        boost::asio::steady_timer timer;
        std::size_t counter = 0;

    public:
        explicit Heartbeat(boost::asio::io_context& io): timer(io){
        }

        void start() {
            schedule();
        }

    private:
        void schedule()
        {
            timer.expires_after(std::chrono::seconds(1));
            timer.async_wait([this](const boost::system::error_code& ec)
            {
                if (ec)
                    return;

                sendHeartbeat();
                schedule();
            });
        }

        void sendHeartbeat()
        {
            std::cout << "Heartbeat #" << ++counter << '\n';
        }
    };

    void run()
    {
        boost::asio::io_context io;

        Heartbeat heartbeat(io);
        heartbeat.start();

        io.run();
    }
}


namespace timer::debounce
{
    class Debouncer
    {
        boost::asio::steady_timer timer;

    public:
        explicit Debouncer(boost::asio::io_context& io): timer(io) {
        }

        void notify()
        {
            LOG << "notify called\n";
            timer.expires_after(std::chrono::milliseconds(300));
            timer.async_wait([this](const boost::system::error_code& ec)
            {
                if (ec)
                {
                    if (ec != boost::asio::error::operation_aborted)
                        ERR << "Timer error: " << ec.message() << '\n';
                    return;
                }
                execute();
            });
        }

    private:

        void execute() const
        {
            LOG << "Processing accumulated events\n";
        }
    };

    void run()
    {
        boost::asio::io_context io;

        Debouncer debouncer(io);

        debouncer.notify();
        debouncer.notify();
        debouncer.notify();

        io.run();

        /**
        2026-10-24 07:09:06.669177 notify called
        2026-10-24 07:09:06.669256 notify called
        2026-10-24 07:09:06.669264 notify called
        2026-10-24 07:09:06.969304 Processing accumulated events
        **/
    }
}


namespace timer::operation_timeout
{
    using namespace std::chrono_literals;

    class OperationWithTimeout
    {
        enum class OperationState: uid_t
        {
            Running,
            Completed,
            TimedOut
        };

        boost::asio::io_context& ioContext;
        boost::asio::steady_timer operationTimeoutTimer;
        boost::asio::thread_pool operationPool { 1 };
        boost::asio::executor_work_guard<boost::asio::io_context::executor_type> workGuard;

        std::atomic<OperationState> operationState { OperationState::Running };

        static constexpr std::chrono::seconds MaxOperationDuration { 2 };

    public:
        explicit OperationWithTimeout(boost::asio::io_context& io):
            ioContext { io },
            operationTimeoutTimer { ioContext },
            workGuard { boost::asio::make_work_guard(ioContext) }
        {
        }

        void start(const std::chrono::milliseconds operationTime = 1s)
        {
            LOG << "Start processing ...\n";
            registerOperationTimeout();
            startOperation(operationTime);
        }

    private:
        void registerOperationTimeout()
        {
            operationTimeoutTimer.expires_after(MaxOperationDuration);
            operationTimeoutTimer.async_wait([this](const boost::system::error_code& ec)
            {
                if (ec == boost::asio::error::operation_aborted) {
                    return;
                }
                if (ec) {
                    ERR << "Timeout timer error: " << ec.message() << '\n';
                    return;
                }

                OperationState expected = OperationState::Running;
                if (operationState.compare_exchange_strong(expected, OperationState::TimedOut)) {
                    ERR << "Operation timed out\n";
                    cancelOperation();
                }
            });
        }

        void startOperation(const std::chrono::milliseconds operationTime)
        {
            LOG << "Operation started\n";
            boost::asio::post(operationPool, [this, operationTime]
            {
                // Simulate a blocking operation running on a worker thread.
                std::this_thread::sleep_for(operationTime);
                boost::asio::post(ioContext, [this] {
                    completeOperation();
                });
            });
        }

        void completeOperation()
        {
            OperationState stateExpected = OperationState::Running;
            if (!operationState.compare_exchange_strong(stateExpected, OperationState::Completed)) {
                LOG << "Operation result ignored: operation already timed out\n";
                workGuard.reset();
                return;
            }

            LOG << "Operation completed\n";

            boost::system::error_code cancelError;
            operationTimeoutTimer.cancel(cancelError);

            if (cancelError)
                ERR << "Failed to cancel timeout: " << cancelError.message() << '\n';

            workGuard.reset();
        }

        void cancelOperation()
        {
            // Request cancellation of the actual operation here.
            LOG << "Operation cancellation requested\n";
        }
    };

    void run()
    {
        boost::asio::io_context io;

        OperationWithTimeout operation(io);
        operation.start(5s);

        io.run();
    }
}

namespace timer::operation_timeout_with_cancellation
{
    using namespace std::chrono_literals;

    class OperationWithTimeout
    {
        enum class OperationState: uint8_t
        {
            Running,
            Completed,
            TimedOut
        };

        boost::asio::io_context& ioContext;
        boost::asio::steady_timer operationTimeoutTimer;
        boost::asio::executor_work_guard<boost::asio::io_context::executor_type> workGuard;

        std::atomic<OperationState> operationState { OperationState::Running };

        std::mutex operationMutex;
        std::condition_variable operationCondition;
        bool cancellationRequested = false;

        static constexpr std::chrono::seconds MaxOperationDuration { 2 };

        // Declared last so the pool joins its worker before other members are destroyed.
        boost::asio::thread_pool operationPool { 1 };

    public:
        explicit OperationWithTimeout(boost::asio::io_context& io):
            ioContext { io },
            operationTimeoutTimer { ioContext },
            workGuard { boost::asio::make_work_guard(ioContext) }
        {
        }

        void start(const std::chrono::milliseconds operationTime = 1s)
        {
            LOG << "Start processing ...\n";
            registerOperationTimeout();
            startOperation(operationTime);
        }

    private:
        void registerOperationTimeout()
        {
            operationTimeoutTimer.expires_after(MaxOperationDuration);
            operationTimeoutTimer.async_wait([this](const boost::system::error_code& ec)
            {
                if (ec == boost::asio::error::operation_aborted) {
                    return;
                }
                if (ec) {
                    ERR << "Timeout timer error: " << ec.message() << '\n';
                    cancelOperation();
                    workGuard.reset();
                    return;
                }

                OperationState expected = OperationState::Running;
                if (operationState.compare_exchange_strong(expected, OperationState::TimedOut))
                {
                    ERR << "Operation timed out\n";
                    cancelOperation();
                    workGuard.reset();
                }
            });
        }

        void startOperation(const std::chrono::milliseconds operationTime)
        {
            LOG << "Operation started\n";
            boost::asio::post(operationPool, [this, operationTime]
            {
                std::unique_lock lock { operationMutex };
                const bool cancelled = operationCondition.wait_for(lock, operationTime, [this] {
                    return cancellationRequested;
                });

                lock.unlock();
                if (cancelled || operationState.load() != OperationState::Running){
                    LOG << "Operation stopped\n";
                    return;
                }

                boost::asio::post(ioContext, [this]{
                    completeOperation();
                });
            });
        }

        void completeOperation()
        {
            OperationState expected = OperationState::Running;
            if (!operationState.compare_exchange_strong(expected, OperationState::Completed)) {
                return;
            }

            LOG << "Operation completed\n";

            boost::system::error_code cancelError;
            operationTimeoutTimer.cancel(cancelError);

            if (cancelError) {
                ERR << "Failed to cancel timeout: " << cancelError.message() << '\n';
            }
            workGuard.reset();
        }

        void cancelOperation()
        {
            {
                std::lock_guard lock { operationMutex };
                cancellationRequested = true;
            }
            operationCondition.notify_all();
        }
    };

    void run()
    {
        boost::asio::io_context io;

        OperationWithTimeout operation(io);
        operation.start(5s);

        io.run();

        // 2026-10-24 08:36:33.969759 Start processing ...
        // 2026-10-24 08:36:33.969842 Operation started
        // 2026-10-24 08:36:35.969918 Operation stopped
        // 2026-10-24 08:36:35.969879 Operation timed out
    }
}

void timer::TestAll()
{
    // start_Resume();
    // Callback::testCallback();

    // one_shot_timer::run();
    // one_shot_timer_2::run();

    // periodic_timer::run();
    // cancellation::run();
    // heartbeats::run();

    // operation_timeout::run();
    operation_timeout_with_cancellation::run();

    // debounce::run();
};