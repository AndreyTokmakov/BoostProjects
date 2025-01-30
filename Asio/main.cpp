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
#include "Post_Dispatch_Tasks/PostDispatchTasks.h"

int main([[maybe_unused]] int argc,
         [[maybe_unused]] char** argv)
{
    const std::vector<std::string_view> args(argv + 1, argv + argc);

    // BackgroundTasks::TestAll();
    // Timer::TestAll();
    PostDispatchTasks::TestAll();
    // Experiments::TestAll();
    // CoroutineApps::TestAll();
    // Timeouts::TestAll();

    return EXIT_SUCCESS;
}
