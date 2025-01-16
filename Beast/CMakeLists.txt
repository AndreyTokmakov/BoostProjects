cmake_minimum_required(VERSION 3.16 FATAL_ERROR)

project(Beast)

SET(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -pthread")
SET(PROJECT_VERSION 1.0.0.0)
SET(CMAKE_CXX_STANDARD  23)
project(${PROJECT_NAME} VERSION ${PROJECT_VERSION} LANGUAGES CXX)

add_compile_options(-c -Wall -Werror -Wextra -O3 -std=c++26)

# include all components
add_executable(${PROJECT_NAME}
        main.cpp
        http/Client.cpp
        http/HTTPServer.cpp
        web_sockets/WebSocketServers.cpp
        web_sockets/WebSocketClients.cpp
)

link_directories(${THIRD_PARTY_DIR}/openssl)
include_directories(${THIRD_PARTY_DIR}/openssl/include)


include_directories("http")
include_directories("web_sockets")

TARGET_LINK_LIBRARIES(${PROJECT_NAME}
        pthread
        Boost::asio
        Boost::beast
        Boost::json
        crypto
        ssl
)