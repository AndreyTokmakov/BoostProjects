/**============================================================================
Name        : Buffers.cpp
Created on  : 18.08.2023
Author      : Andrei Tokmakov
Version     : 1.0
Copyright   : Your copyright notice
Description : Buffers
============================================================================**/

#include "Buffers.h"

#include <boost/asio.hpp>
#include <iostream>
using namespace boost;

namespace Buffers
{
    void readInputToBuffer_Simulation()
    {
        asio::streambuf buf;
        std::ostream output(&buf);
        output << "Message1\nMessage2";

        // Now we want to read all data from a streambuf
        // Instantiate an input stream which uses our stream buffer.
        std::istream input(&buf);

        // We'll read data into this string.
        std::string readData;

        while (std::getline(input, readData))
        {
            std::cout << readData << std::endl;
        }
    }
};

void Buffers::TestAll()
{
    readInputToBuffer_Simulation();
};

