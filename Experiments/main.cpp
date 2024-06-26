/**============================================================================
Name        : Experiments.cpp
Created on  : 04.10.2021
Author      : Andrei Tokmakov
Version     : 1.0
Copyright   : Your copyright notice
Description : Boost Experiments modules tests
============================================================================**/

#include <iostream>
#include <vector>

#include <boost/version.hpp>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/thread.hpp>
#include <boost/uuid/detail/md5.hpp>
#include <boost/algorithm/hex.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/tokenizer.hpp>
#include <boost/noncopyable.hpp>


namespace Utilities {

    void PrintBoostVersion()
    {
        std::cout << "Using Boost " << BOOST_VERSION / 100000     << "."  // major version
                  << BOOST_VERSION / 100 % 1000 << "."  // minor version
                  << BOOST_VERSION % 100                // patch level
                  << std::endl;
    }

    /** Just to print version at the startup: **/
    const static int dummy = []() {
        PrintBoostVersion();
        return 0;
    }();
}


namespace Multithreading {

    void SimpleAsynchTest() {
        auto task = []{
            for (int i = 0; i < 10; ++i) {
                std::cout << i << std::endl;
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
        };

        auto future = std::async(task);
        future.wait();
    }
}

namespace MD5 {

    using boost::uuids::detail::md5;

    // TODO: Размер блока (по умолчанию, 1 Мб)

    class HashingAlgorithm {
    public:
        virtual ~HashingAlgorithm() = default;

        [[nodiscard]]
        virtual std::string calculateHash(std::string_view input) const noexcept = 0;
    };

    class BoostHash final : public HashingAlgorithm {
    private:
        using MD5 = boost::uuids::detail::md5;
        using digest_type = boost::uuids::detail::md5::digest_type;

        static inline constexpr size_t digest_size { sizeof(digest_type) };

    public:
        ~BoostHash() override = default;

        [[nodiscard("Make sure to handle return value")]]
        virtual std::string calculateHash(std::string_view input) const noexcept override {
            if (input.empty())
                return {};
            MD5 boost_md5;
            boost_md5.process_bytes(input.data(), input.size());
            digest_type digest;
            boost_md5.get_digest(digest);
            const auto char_digest = reinterpret_cast<const char*>(&digest);

            std::string result {};
            result.reserve(digest_size);
            boost::algorithm::hex(char_digest,char_digest + digest_size,
                                  std::back_inserter(result));
            return result;
        }
    };


    std::string GetMd5(std::string_view input) {
        if (input.empty())
            return {};
        boost::uuids::detail::md5 boost_md5;
        boost_md5.process_bytes(input.data(), input.size());
        boost::uuids::detail::md5::digest_type digest;
        boost_md5.get_digest(digest);
        const auto char_digest = reinterpret_cast<const char*>(&digest);

        std::string str_md5 {};
        boost::algorithm::hex(char_digest,
                              char_digest + sizeof(boost::uuids::detail::md5::digest_type), std::back_inserter(str_md5));
        return str_md5;
    }

    std::string toString(const md5::digest_type &digest)
    {
        const auto charDigest = reinterpret_cast<const char*>(&digest);
        std::string result;
        boost::algorithm::hex(charDigest, charDigest + sizeof(md5::digest_type), std::back_inserter(result));
        return result;
    }

    //--------------------------------------------------------------------

    void Test1()
    {
        const std::string text ("123456789qwerty");
        md5 hash;
        md5::digest_type digest;

        hash.process_bytes(text.data(), text.size());
        hash.get_digest(digest);

        std::cout << "md5(" << text << ") = " << toString(digest) << '\n';
    }

    void Test2() {
        constexpr std::string_view text ("123456789qwerty");
        const std::string hash = GetMd5(text);
        std::cout << "md5(" << text << ") = " << hash << '\n';
    }


    void Test3() {
        std::unique_ptr<HashingAlgorithm> hasher { std::make_unique<BoostHash>()};

        constexpr std::string_view text ("123456789qwerty");
        const std::string hash = hasher->calculateHash(text);

        std::cout << "md5(" << text << ") = " << hash << '\n';
    }

}


namespace  ThreadPool {

    void Test() {
        std::mutex mtx;
        boost::asio::thread_pool pool(std::thread::hardware_concurrency());

        for (int i = 0; i < 100; ++i) {
            boost::asio::post(pool, [i, &mtx](){
                {
                    std::lock_guard<std::mutex> lock{mtx};
                    std::cout << "Task started " << i << "\n";
                }
                std::this_thread::sleep_for(std::chrono::seconds(1));
                {
                    std::lock_guard<std::mutex> lock{mtx};
                    std::cout << "Task completed " << i << "\n";
                }
            });
        }

        pool.join();
        std::cout << "Done\n";
    }



    class Worker final {
    private:
        boost::asio::thread_pool pool = []() {
            return boost::asio::thread_pool(std::thread::hardware_concurrency());
        }();

        mutable std::mutex mtx;
        constexpr static inline size_t DEFAULT_BLOCK_SIZE { 1024 * 1024 };

    private:

        template <typename F>
        void submit(F&& f) {
            boost::asio::post(pool, std::forward<F>(f));
        }

        void handler(int i) {
            {
                std::lock_guard<std::mutex> lock{mtx};
                std::cout << "Task started " << i << "\n";
            }
            std::this_thread::sleep_for(std::chrono::seconds(1));
            {
                std::lock_guard<std::mutex> lock{mtx};
                std::cout << "Task completed " << i << "\n";
            }
        }

    public:
        void Run() {
            for (int i = 0; i < 100; ++i) {
                submit(boost::bind(&Worker::handler, this, i));
            }
            pool.join();
        }

        ~Worker() {
            pool.stop();
        }
    };

    void Test2() {
        Worker w;
        w.Run();
    }
}



namespace Callback
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



int main([[maybe_unused]] int argc,
         [[maybe_unused]] char** argv)
{
    const std::vector<std::string_view> args(argv + 1, argv + argc);
    Utilities::PrintBoostVersion();

    // Multithreading::SimpleAsynchTest()
    // Callback::testCallback();

    // MD5::Test1();
    // MD5::Test2();
    // MD5::Test3();

    // ThreadPool::Test();
    // ThreadPool::Test2();

    return EXIT_SUCCESS;
}
