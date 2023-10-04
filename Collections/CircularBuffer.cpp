/**============================================================================
Name        : CircularBuffer.cpp
Created on  : 26.08.2023
Author      : Andrei Tokmakov
Version     : 1.0
Copyright   : Your copyright notice
Description : CircularBuffer.cpp
============================================================================**/

#include "CircularBuffer.h"

#include <iostream>
#include "boost/circular_buffer.hpp"

namespace CircularBuffer
{
    void pushBack_CapacityTest()
    {
        boost::circular_buffer<int> circular_buffer(3);

        circular_buffer.push_back(1);
        circular_buffer.push_back(2);
        circular_buffer.push_back(3);

        std::cout << circular_buffer[0] << ' ' <<  circular_buffer[1] << ' ' << circular_buffer[2]  << '\n';
        circular_buffer.push_back(4);
        std::cout << circular_buffer[0] << ' ' <<  circular_buffer[1] << ' ' << circular_buffer[2]  << '\n';

        // output:
        // 1 2 3
        // 2 3 4
    }
}

void CircularBuffer::TestAll()
{
    pushBack_CapacityTest();
}

