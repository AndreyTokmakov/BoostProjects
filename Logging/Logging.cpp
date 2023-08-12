/**============================================================================
Name        : Logging.h
Created on  : 03.09.2022
Author      : Tokmakov Andrey
Version     : 1.0
Copyright   : Your copyright notice
Description : Logging
============================================================================**/

#define BOOST_LOG_DYN_LINK 1

#include <iostream>
#include <fstream>
#include <string_view>

#include <stdexcept>
#include <exception>

#include <thread>

#include "Logging.h"

#include <boost/log/trivial.hpp>



namespace logging = boost::log;

namespace BasicExamples
{
    void SimpleTests() {
        // Trivial logging: all log records are written into a file
        BOOST_LOG_TRIVIAL(trace) << "A trace severity message";
        BOOST_LOG_TRIVIAL(debug) << "A debug severity message";
        BOOST_LOG_TRIVIAL(info) << "An informational severity message";
        BOOST_LOG_TRIVIAL(warning) << "A warning severity message";
        BOOST_LOG_TRIVIAL(error) << "An error severity message";
        BOOST_LOG_TRIVIAL(fatal) << "A fatal severity message";
    }



    void init_logging()
    {
        /*
        logging::core::get()->set_filter     (
                logging::trivial::severity >= logging::trivial::info
        );
        */
    }


    void Filering() {
        using namespace boost::log;

        // Trivial logging: all log records are written into a file
        init_logging();

        // Now the first two lines will not pass the filter
        BOOST_LOG_TRIVIAL(trace) << "A trace severity message";
        BOOST_LOG_TRIVIAL(debug) << "A debug severity message";
        BOOST_LOG_TRIVIAL(info) << "An informational severity message";
        BOOST_LOG_TRIVIAL(warning) << "A warning severity message";
        BOOST_LOG_TRIVIAL(error) << "An error severity message";
        BOOST_LOG_TRIVIAL(fatal) << "A fatal severity message";
    }


}


void Logging::TestAll()
{
    BasicExamples::SimpleTests();
};

