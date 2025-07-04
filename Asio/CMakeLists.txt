cmake_minimum_required(VERSION 3.16 FATAL_ERROR)

project(Asio)

SET(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -pthread")
SET(PROJECT_VERSION 1.0.0.0)
SET(CMAKE_CXX_STANDARD 26)
project(${PROJECT_NAME} VERSION ${PROJECT_VERSION} LANGUAGES CXX)

# add_compile_options(-c -Wall -Werror -Wextra -O3 -std=c++26)
add_compile_options(-c -O3 -std=c++26)

# find_package(OpenSSL REQUIRED)

# include all components
add_executable(${PROJECT_NAME}
        main.cpp
        BackgroundTasks/BackgroundTasks.cpp
        Experiments/Experiments.cpp
        Threading/Threading.cpp
        Coroutines/CoroutineApps.cpp
        Timer/Timer.cpp
        Timeouts/Timeouts.cpp
        Utils/Utilities.cpp
        IO_Context/IoContext.cpp
        ExceptionHandling/ExceptionHandling.cpp
)

TARGET_LINK_LIBRARIES(${PROJECT_NAME}
        pthread
        Boost::asio
        #OpenSSL::ct
        crypto
)