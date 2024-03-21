/**============================================================================
Name        : FIbers.cpp
Created on  : 21.03.2024
Author      : Andrei Tokmakov
Version     : 1.0
Copyright   : Your copyright notice
Description : FIbers.cpp
============================================================================**/

#include "Fibers.h"

#include <iostream>
#include <string>
#include <format>
#include <syncstream>

#include <vector>
#include <queue>

#include <boost/fiber/all.hpp>

namespace fibers = boost::fibers;
namespace this_fiber = boost::this_fiber;

namespace
{
    std::string time() {
        return std::format("{:%d-%m-%Y %H:%M:%OS} ", std::chrono::system_clock::now());
    }
}

namespace Fibers
{
    void SimpleTest()
    {
        std::vector<int> buffer {};
        std::atomic_bool done {false};

        auto consume  = [&done, &buffer] {
            while (!done.load(std::memory_order_acquire)) {
                int data = buffer.front();
                buffer.pop_back();
                std::osyncstream(std::cout) << time() <<" Consumed: " << data << std::endl;
                this_fiber::yield();
            }
        };

        auto produce  = [&done, &buffer] {
            for (int i = 0; i < 10; ++i)
            {
                buffer.push_back(i);
                std::osyncstream(std::cout) << time() <<" Produced: " << i << std::endl;
                this_fiber::yield();
            }
            done.store(true, std::memory_order_release);
        };

        fibers::fiber producer {produce},
                      consumer {consume};

        consumer.join();
        producer.join();
    }
}

void Fibers::TestAll()
{
    Fibers::SimpleTest();
};