cmake_minimum_required(VERSION 3.21)
project(BoostProjects)

include(FetchContent)
set(CMAKE_CXX_STANDARD 23)

message(STATUS "[dependency] fetching boost")
set(BOOST_ENABLE_PYTHON OFF)
set(BOOST_IOSTREAMS_ENABLE_ZLIB ON)
FetchContent_Declare(boost
        GIT_REPOSITORY https://github.com/boostorg/boost.git
        GIT_TAG boost-1.86.0
        GIT_SHALLOW ON
        GIT_PROGRESS ON
        CMAKE_ARGS  -DBOOST_IOSTREAMS_ENABLE_ZLIB=ON
)
FetchContent_MakeAvailable(boost)

message(STATUS "[dependency] fetching OpenSSL")
FetchContent_Declare(OpenSSL
        GIT_REPOSITORY https://github.com/openssl/openssl.git
        GIT_TAG        openssl-3.3.2
        GIT_SHALLOW ON
        GIT_PROGRESS ON
)
FetchContent_MakeAvailable(OpenSSL)


#[=[
set(THIRD_PARTY_LIBS_DIR /home/andtokm/Projects/third_party)

set(BOOST_DIR ${THIRD_PARTY_LIBS_DIR}/boost)
set(BOOST_DIR_1_85 ${THIRD_PARTY_LIBS_DIR}/boost_1_85)
set(BOOST_DIR_1_85_CLANG ${THIRD_PARTY_LIBS_DIR}/boost_1_85_clang)

set(BOOST_ROOT_DIR ${BOOST_DIR})
# set(BOOST_ROOT_DIR ${BOOST_DIR_1_85})
set(BOOST_LIBS_DIR ${BOOST_ROOT_DIR}/stage/lib)

set(BOOST_ROOT         ${BOOST_ROOT_DIR})
set(BOOST_INCLUDEDIR   ${BOOST_ROOT_DIR})
set(BOOST_INCLUDE_DIR  ${BOOST_ROOT_DIR})
set(BOOST_INCLUDE_DIRS ${BOOST_ROOT_DIR})

set(BOOST_LIBRARYDIR   ${BOOST_LIBS_DIR})
set(BOOST_LIBRARY_DIR  ${BOOST_LIBS_DIR})
set(BOOST_LIBRARY_DIRS ${BOOST_LIBS_DIR})

include_directories(${BOOST_ROOT_DIR})
include_directories(${BOOST_INCLUDE_DIRS})
link_directories(${BOOST_LIBRARY_DIR})

# set(Bost_DEBUG ON)
# set(Boost_VERBOSE ON)

# https://cmake.org/cmake/help/v3.10/module/FindBoost.html

# Set to ON to disable searching in locations not specified by these hint variables. Default is OFF.
set (Boost_NO_SYSTEM_PATHS ON)

# Set to OFF to use the non-multithreaded libraries ('mt' tag).  Default is ON.
set (Boost_USE_MULTITHREADED ON)

# Set to ON to force the use of the static libraries.  Default is OFF.
set (Boost_USE_STATIC_LIBS ON)

# Set to ON or OFF to specify whether to use libraries linked statically to the C++ runtime ('s' tag).
# Default is platform dependent.
set (Boost_USE_STATIC_RUNTIME OFF)

# set (BOOST_ALL_DYN_LINK OFF)

# find_package(Boost MODULE)
# find_package(Boost 1.85.0 COMPONENTS ALL)
# find_package(Boost 1.86.0 COMPONENTS ALL)
# find_package(Boost 1.86.0 MODULE)
# find_package(Boost 1.71.0 MODULE)
#]=]

set (Boost_NO_SYSTEM_PATHS ON)

# find_package(Boost 1.86.0 COMPONENTS ALL)


message (STATUS "\t\t Boost_VERSION_STRING: ${Boost_VERSION_STRING}")
message (STATUS "\t\t Boost_VERSION: ${Boost_VERSION}")
message (STATUS "\t\t Boost_LIBRARIES: ${Boost_LIBRARIES}")
message (STATUS "\t\t Boost_INCLUDE_DIR: ${Boost_INCLUDE_DIR}")
message (STATUS "\t\t Boost_INCLUDE_DIRS: ${Boost_INCLUDE_DIRS}")
message (STATUS "\t\t Boost_LIBRARY_DIR: ${Boost_LIBRARY_DIR}")


add_subdirectory(Asio)
add_subdirectory(Collections)
add_subdirectory(Communication)
add_subdirectory(DateTime)
add_subdirectory(Describe)
add_subdirectory(Json)
add_subdirectory(Logging)
add_subdirectory(Math)
add_subdirectory(Networking)
add_subdirectory(Experiments)
add_subdirectory(ProgramOptions)
add_subdirectory(Serial)
add_subdirectory(SmartPointers)
add_subdirectory(Spirit)
add_subdirectory(Threads)
add_subdirectory(Timer)
add_subdirectory(Tokenizer)
add_subdirectory(Uuid)