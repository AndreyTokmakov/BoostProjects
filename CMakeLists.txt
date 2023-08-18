cmake_minimum_required(VERSION 3.25)

project(BoostProjects)
add_compile_options(-c -Wall -Werror -Wextra -O3 -std=c++2a)

SET(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -pthread")
SET(PROJECT_VERSION 1.0.0.0)
SET(CMAKE_CXX_STANDARD 20)
project(${PROJECT_NAME} VERSION ${PROJECT_VERSION} LANGUAGES CXX)

# include_directories(/home/andtokm/DiskS/ProjectsUbuntu/third_party/Eigen/Eigen)

if(Boost_VERSION LESS 106500)
    message(STATUS "Boost version: Boost_VERSION=${Boost_VERSION}")
    message(FATAL "Required Boost version >= 1.65.0")
endif()
message (STATUS "BOOST VERSION: ${Boost_VERSION}")

find_package(OpenSSL REQUIRED)

find_package(Boost REQUIRED COMPONENTS
        log_setup
        log
        filesystem
        program_options
        )


# include all components
add_executable(BoostProjects
        main.cpp
        Json/Json.cpp
        # Utilities.cpp
        # Features.cpp
        Math/BoostMathTests.cpp Math/BoostMathTests.h
        Spirit/Spirit.cpp Spirit/Spirit.h
        ProgramOptions/ProgramOptions.cpp ProgramOptions/ProgramOptions.h
        Logging/Logging.cpp Logging/Logging.h Uuid/Uuid.cpp Uuid/Uuid.h
        Timer/Timer.cpp Timer/Timer.h Threads/Threads.cpp Threads/Threads.h DateTime/DateTime.cpp DateTime/DateTime.h Serial/Serial.cpp Serial/Serial.h Networking/Networking.cpp Networking/Networking.h Buffers/Buffers.cpp Buffers/Buffers.h)


# Link phread library
TARGET_LINK_LIBRARIES(BoostProjects pthread
        ${Boost_LIBRARIES}
        ${OPENSSL_LIBRARIES}
        boost_timer
)
