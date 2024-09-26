cmake_minimum_required(VERSION 3.16 FATAL_ERROR)

project(Math)

SET(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -pthread")
SET(PROJECT_VERSION 1.0.0.0)
SET(CMAKE_CXX_STANDARD 20)
project(${PROJECT_NAME} VERSION ${PROJECT_VERSION} LANGUAGES CXX)

add_compile_options(-c -Wall -Werror -Wextra -O3 -std=c++23)

# include all components
add_executable(${PROJECT_NAME}
        main.cpp
        BoostMathTests.h BoostMathTests.cpp
)

TARGET_LINK_LIBRARIES(${PROJECT_NAME}
        Boost::math
)