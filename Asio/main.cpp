/**============================================================================
Name        : Asio.cpp
Created on  : 04.10.2021
Author      : Andrei Tokmakov
Version     : 1.0
Copyright   : Your copyright notice
Description : Boost Asio modules tests
============================================================================**/

#include <iostream>
#include <vector>

#include "BackgroundTasks/BackgroundTasks.h"
#include "Experiments/Experiments.h"
#include "Coroutines/CoroutineApps.h"
#include "Timeouts/Timeouts.h"
#include "Timer/Timer.h"
#include "Threading/Threading.h"
#include "IO_Context/IoContext.h"
#include "ExceptionHandling/ExceptionHandling.h"

int main([[maybe_unused]] int argc,
         [[maybe_unused]] char** argv)
{
    const std::vector<std::string_view> args(argv + 1, argv + argc);

    BackgroundTasks::TestAll();
    // Timer::TestAll();
    // Threading::TestAll();
    // Experiments::TestAll();
    // CoroutineApps::TestAll();
    // Timeouts::TestAll();
    // IoContext::TestAll();
    // ExceptionHandling::TestAll();

    return EXIT_SUCCESS;
}
