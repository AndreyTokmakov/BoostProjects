/**============================================================================
Name        : Threads.cpp
Created on  : 03.09.2022
Author      : Tokmakov Andrey
Version     : 1.0
Copyright   : Your copyright notice
Description : Threads
============================================================================**/

#include "Threads.h"

#define _SILENCE_CXX17_ALLOCATOR_VOID_DEPRECATION_WARNING

#include <algorithm>
#include <cassert>
#include <iostream>
#include <thread>
#include <string>
#include <string_view>
#include <semaphore>

#include <boost/asio/io_service.hpp>
#include <boost/asio/thread_pool.hpp>
#include <boost/asio/post.hpp>

#include <boost/bind.hpp>
#include <boost/thread/thread.hpp>

#include <boost/program_options.hpp>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/uuid/detail/md5.hpp>
#include <boost/algorithm/hex.hpp>


namespace Threads
{
    using IOContext = boost::asio::io_context;
    using IOService = boost::asio::io_service ;

    using ThreadGroup = boost::thread_group;
    using ThreadPool = boost::asio::thread_pool;

    /*
    void SimpleTest() {
        IOService ioContext;
        ThreadGroup pool;

        IOContext::work work(ioContext);

        pool.create_thread(boost::bind(&IOContext::run, &ioContext));
        pool.create_thread(boost::bind(&IOContext::run, &ioContext));


        auto task = [](std::string&& data, unsigned int timeout) {
            auto id = std::this_thread::get_id();
            std::cout << "Task started: Id:" << id  << ". Data: " << data << ". Timeout: " << timeout << "\n";

            std::this_thread::sleep_for(std::chrono::seconds(timeout));
            std::cout << "Task completed: Id:" << id << "\n";
        };

        ioContext.post(boost::bind(task, "Hello World!", 3));
        ioContext.post(boost::bind(task, "./cache",9));
        ioContext.post(boost::bind(task, "twitter,gmail,facebook,tumblr,reddit", 5));


        ioContext.stop();
        // ioContext.join_all();
    }
    */

    void SimpleTest() {
        ThreadPool pool{1};

        auto task = [] {
            auto id = std::this_thread::get_id();
            std::cout << "Task statred: Id:" << id << "\n";
            std::this_thread::sleep_for(std::chrono::seconds(5));
            std::cout << "Task completed: Id:" << id << "\n";
        };

        for (int i = 0; i < 10; ++i) {
            boost::asio::post(pool, task);
            std::cout << "Added" << std::endl;
        }



        pool.join();
    }


    /*
    void SimpleTest2() {
        ThreadPool pool{ 4 };

        auto task = [](std::string&& data, unsigned int timeout) {
            auto id = std::this_thread::get_id();
            std::cout << "Task started: Id:" << id << ". Data: " << data << ". Timeout: " << timeout << "\n";

            std::this_thread::sleep_for(std::chrono::seconds(timeout));
            std::cout << "Task completed: Id:" << id << "\n";
        };

        boost::asio::post(pool, boost::bind(task, "Hello World!", 3));
        pool.join();
    }
    */
}

namespace Threads::Pools {


    class Worker final {
    private:
        // Creating a thread pool with an available capacity equal to the number of logical
        // cores on the CPU (It may be reasonable to reduce the number by half to increase
        // performance - by making capacity equal to the number of real/physical CPU's on the machine)
        boost::asio::thread_pool pool = []() {
            // return boost::asio::thread_pool(std::thread::hardware_concurrency());
            return boost::asio::thread_pool(1);
        }();


        std::atomic<bool> run{ true };

    private:
        template <typename... Args>
        void submit(Args&& ... params) {
            boost::asio::post(pool, std::forward<Args>(params)...);
        }
        void handler() {
            auto id = std::this_thread::get_id();
            std::cout << "Task statred: Id:" << id << "\n";
            std::this_thread::sleep_for(std::chrono::seconds(1));
            std::cout << "Task completed: Id:" << id << "\n";
        }

    public:

        void RunTest() {

            for (int i = 0; i < 10; ++i) {
                submit([this] { handler(); });
                std::cout << "Added" << std::endl;
            }

            pool.join();
        }

        ~Worker() {
            pool.stop();
        }
    };


    void TestPool() {
        Worker w;
        w.RunTest();
    }
};

void Threads::TestAll()
{
    // SimpleTest();

    Pools::TestPool();
};
