cmake_minimum_required(VERSION 3.16 FATAL_ERROR)

project(Serial)

SET(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -pthread")
SET(PROJECT_VERSION 1.0.0.0)
SET(CMAKE_CXX_STANDARD 20)
project(${PROJECT_NAME} VERSION ${PROJECT_VERSION} LANGUAGES CXX)

add_compile_options(-c -Wall -Werror -Wextra -O3 -std=c++23)

find_package(Boost REQUIRED COMPONENTS
        log_setup
        log
        filesystem
        program_options
)

# include all components
add_executable(Serial
        main.cpp
        Serial.cpp
        Serial.h
)

TARGET_LINK_LIBRARIES(Serial
        pthread
        ${Boost_LIBRARIES}
        ${OPENSSL_LIBRARIES}
        boost_timer  -lrt
)