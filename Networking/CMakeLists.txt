cmake_minimum_required(VERSION 3.16 FATAL_ERROR)

project(Networking)

SET(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -pthread")
SET(PROJECT_VERSION 1.0.0.0)
SET(CMAKE_CXX_STANDARD 20)
project(${PROJECT_NAME} VERSION ${PROJECT_VERSION} LANGUAGES CXX)

add_compile_options(-c -Wall -Wextra -O3 -std=c++23 -Wno-array-bound)

find_package(OpenSSL REQUIRED)

# include all components
add_executable(${PROJECT_NAME}
        main.cpp
        HTTPServer.cpp
        Networking.cpp
        Beast.cpp
        SSL_TCPServer.cpp
        TCPEchoServerClient.cpp
        UDPEchoServerClient.cpp
)

TARGET_LINK_LIBRARIES(${PROJECT_NAME}
        pthread
        Boost::asio
        Boost::beast
        Boost::json
        ${OPENSSL_LIBRARIES}
)