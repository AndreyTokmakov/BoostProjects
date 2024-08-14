/**============================================================================
Name        : Tests.cpp
Created on  : 15.09.2021
Author      : Tokmakov Andrey
Version     : 1.0
Copyright   : Your copyright notice
Description : Tests C++ project
============================================================================**/

#include <iostream>
#include <vector>
#include <string_view>

#include "CircularBuffer.h"
#include "FlatMap.h"

int main([[maybe_unused]] int argc,
         [[maybe_unused]] char** argv)
{
    const std::vector<std::string_view> args(argv + 1, argv + argc);

    // CircularBuffer::TestAll();
    FlatMap::TestAll();

    return EXIT_SUCCESS;
}
