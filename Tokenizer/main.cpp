/**============================================================================
Name        : Tokenizer.cpp
Created on  : 04.10.2021
Author      : Andrei Tokmakov
Version     : 1.0
Copyright   : Your copyright notice
Description : Boost Tokenizer modules tests
============================================================================**/

#include <iostream>
#include <vector>

#include "Tokenizer.h"

int main([[maybe_unused]] int argc,
         [[maybe_unused]] char** argv)
{
    const std::vector<std::string_view> args(argv + 1, argv + argc);

    Tokenizer::TestAll(argc, argv);

    return EXIT_SUCCESS;
}
