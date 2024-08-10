cmake_minimum_required(VERSION 3.16 FATAL_ERROR)

project(Json)

SET(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -pthread")
SET(PROJECT_VERSION 1.0.0.0)
SET(CMAKE_CXX_STANDARD 20)
project(${PROJECT_NAME} VERSION ${PROJECT_VERSION} LANGUAGES CXX)

add_compile_options(-c -Wall -Werror -Wextra -O3 -std=c++23)

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
        Json.h Json.cpp
)

TARGET_LINK_LIBRARIES(${PROJECT_NAME}
        pthread
        ${Boost_LIBRARIES}
        ${OPENSSL_LIBRARIES}
        boost_timer  -lrt
)