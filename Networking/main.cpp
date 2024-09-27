/**============================================================================
Name        : Communication.cpp
Created on  : 04.10.2021
Author      : Andrei Tokmakov
Version     : 1.0
Copyright   : Your copyright notice
Description : Boost Communication modules tests
============================================================================**/

#include <iostream>
#include <vector>

#include "Networking.h"
#include "Beast.h"

int main([[maybe_unused]] int argc,
         [[maybe_unused]] char** argv)
{
    const std::vector<std::string_view> params(argv + 1, argv + argc);

    Networking::TestAll();
    Beast::TestAll(params);

    return EXIT_SUCCESS;
}
