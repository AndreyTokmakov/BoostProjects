cmake_minimum_required(VERSION 3.16 FATAL_ERROR)

project(Beast)

SET(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -pthread")
SET(PROJECT_VERSION 1.0.0.0)
SET(CMAKE_CXX_STANDARD  23)
project(${PROJECT_NAME} VERSION ${PROJECT_VERSION} LANGUAGES CXX)

add_compile_options(-c -Wall -Werror -Wextra -O3 -std=c++26)


link_directories(${THIRD_PARTY_DIR}/openssl)
include_directories(${THIRD_PARTY_DIR}/openssl/include)

##[[
find_package(OpenSSL REQUIRED)
message (STATUS "OPENSSL_FOUND: ${OPENSSL_FOUND}")
message (STATUS "   OPENSSL_INCLUDE_DIR: ${OPENSSL_INCLUDE_DIR}")
message (STATUS "   OPENSSL_CRYPTO_LIBRARIES: ${OPENSSL_CRYPTO_LIBRARIES}")
message (STATUS "   OPENSSL_SSL_LIBRARY: ${OPENSSL_SSL_LIBRARY}")
message (STATUS "   OPENSSL_SSL_LIBRARIES: ${OPENSSL_SSL_LIBRARIES}")
message (STATUS "   OPENSSL_LIBRARIES: ${OPENSSL_LIBRARIES}")
message (STATUS "   OPENSSL_VERSION: ${OPENSSL_VERSION}")
message (STATUS "   OPENSSL_APPLINK_SOURCE: ${OPENSSL_APPLINK_SOURCE}")
##]]

# include all components
add_executable(${PROJECT_NAME}
        main.cpp
        common/root_certificates.hpp
        common/server_certificate.hpp
        http/Client.cpp
        http/HTTPServer.cpp
        http/HTTPS_Server.cpp
        web_sockets/WebSocketServers.cpp
        web_sockets/WebSocketClients.cpp
)

include_directories("common")
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