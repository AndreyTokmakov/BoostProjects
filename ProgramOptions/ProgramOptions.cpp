//============================================================================
// Name        : ProgramOptions.h
// Created on  : 03.07.2022
// Author      : Tokmakov Andrei
// Version     : 1.0
// Copyright   : Your copyright notice
// Description : C++ Boost ProgramOptions
//============================================================================

#include "ProgramOptions.h"

#include <iostream>
#include <boost/program_options.hpp>
#include <exception>

namespace po = boost::program_options;

namespace ProgramOptions
{
    void Test(int argc, char** argv) {
        try {
            po::options_description desc("Allowed options");
            desc.add_options()
                    ("help", "produce help message")
                    ("compression", po::value<double>(), "set compression level");
            po::variables_map paramsMap {};
            po::store(po::parse_command_line(argc, argv, desc), paramsMap);
            po::notify(paramsMap);

            if (paramsMap.count("help")) {
                std::cout << desc << "\n";
                return;
            }

            if (paramsMap.count("compression")) {
                std::cout << "Compression level was set to " << paramsMap["compression"].as<double>() << ".\n";
            } else {
                std::cout << "Compression level was not set.\n";
            }
        }
        catch(const std::exception& exc) {
            std::cout << "error: " << exc.what() << "\n";
            return;
        }
        catch(...) {
            std::cout << "Exception of unknown type!\n";
            return;
        }
    }
}

void ProgramOptions::TestAll([[maybe_unused]] int argc,
                             [[maybe_unused]] char** argv) {
    Test(argc, argv);
}
