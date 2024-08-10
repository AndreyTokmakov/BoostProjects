cmake_minimum_required(VERSION 3.16 FATAL_ERROR)

project(Logging)

SET(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -pthread")
SET(PROJECT_VERSION 1.0.0.0)
SET(CMAKE_CXX_STANDARD 20)
project(${PROJECT_NAME} VERSION ${PROJECT_VERSION} LANGUAGES CXX)

add_compile_options(-c -Wall -Werror -Wextra -O3 -BOOST_LOG_DYN_LINK -std=c++23)


#[=[
cmake_policy(SET CMP0144 OLD)
find_package(Boost REQUIRED COMPONENTS
        log_setup
        log
        filesystem
        program_options
)
#]=]

# include all components
add_executable(${PROJECT_NAME}
        main.cpp
        Logging.cpp Logging.h
)

add_definitions(-DBOOST_LOG_DYN_LINK)

TARGET_LINK_LIBRARIES(${PROJECT_NAME}
        pthread
        boost_log
        boost_log_setup
        boost_system
        # boost_log_with_support
)