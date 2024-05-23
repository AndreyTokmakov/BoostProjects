cmake_minimum_required(VERSION 3.21)
project(BoostProjects)

set (CMAKE_CXX_STANDARD 23)

set(BOOST_ROOT         home/andtokm/DiskS/ProjectsUbuntu/third_party/boost)

set(BOOST_INCLUDEDIR   /home/andtokm/DiskS/ProjectsUbuntu/third_party/boost)
set(Boost_INCLUDE_DIR  /home/andtokm/DiskS/ProjectsUbuntu/third_party/boost)
set(Boost_INCLUDE_DIRS /home/andtokm/DiskS/ProjectsUbuntu/third_party/boost)

set(BOOST_LIBRARYDIR   /home/andtokm/DiskS/ProjectsUbuntu/third_party/boost/stage/lib)
set(Boost_LIBRARY_DIR  /home/andtokm/DiskS/ProjectsUbuntu/third_party/boost/stage/lib)
set(Boost_LIBRARY_DIRS /home/andtokm/DiskS/ProjectsUbuntu/third_party/boost/stage/lib)

# set(Boost_DEBUG ON)
# set(Boost_VERBOSE ON)

message (STATUS "\t\t Boost_INCLUDE_DIRS: ${Boost_INCLUDE_DIRS}")
message (STATUS "\t\t Boost_LIBRARY_DIR:  ${Boost_LIBRARY_DIR}")
message (STATUS "\t\t Boost_LIBRARY_DIRS: ${Boost_LIBRARY_DIRS}")

include_directories(/home/andtokm/DiskS/ProjectsUbuntu/third_party/boost)
include_directories(${Boost_INCLUDE_DIRS})
link_directories(${Boost_LIBRARY_DIR})



# https://cmake.org/cmake/help/v3.10/module/FindBoost.html

# Set to ON to disable searching in locations not specified by these hint variables. Default is OFF.
set (Boost_NO_SYSTEM_PATHS ON)

# Set to OFF to use the non-multithreaded libraries ('mt' tag).  Default is ON.
# set (Boost_USE_MULTITHREADED ON)

# Set to ON to force the use of the static libraries.  Default is OFF.
set (Boost_USE_STATIC_LIBS ON)

# Set to ON or OFF to specify whether to use libraries linked statically to the C++ runtime ('s' tag).
# Default is platform dependent.
set (Boost_USE_STATIC_RUNTIME OFF)

# set (BOOST_ALL_DYN_LINK OFF)

# find_package(Boost MODULE)
find_package(Boost 1.86.0 COMPONENTS ALL)
# find_package(Boost 1.86.0 MODULE)
# find_package (Boost 1.71.0 MODULE)

# set(Boost_USE_STATIC_LIBS ON)
# set(Boost_NO_SYSTEM_PATHS TRUE)


message (STATUS "\t\t Boost_VERSION_STRING: ${Boost_VERSION_STRING}")
message (STATUS "\t\t Boost_VERSION: ${Boost_VERSION}")
message (STATUS "\t\t Boost_LIBRARIES: ${Boost_LIBRARIES}")
message (STATUS "\t\t Boost_INCLUDE_DIR: ${Boost_INCLUDE_DIR}")
message (STATUS "\t\t Boost_INCLUDE_DIRS: ${Boost_INCLUDE_DIRS}")
message (STATUS "\t\t Boost_LIBRARY_DIR: ${Boost_LIBRARY_DIR}")

find_package(Boost
        REQUIRED
        COMPONENTS
        regex
        date_time
        system
        filesystem
        thread
        graph
        program_options
)

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