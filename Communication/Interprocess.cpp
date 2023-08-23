/**============================================================================
Name        : Interprocess.cpp
Created on  : 23.08.2023
Author      : Andrei Tokmakov
Version     : 1.0
Copyright   : Your copyright notice
Description : Interprocess
============================================================================**/

#include "Interprocess.h"

#include <boost/interprocess/shared_memory_object.hpp>
#include <boost/interprocess/mapped_region.hpp>
#include <iostream>
#include <string_view>

namespace Interprocess
{
    using namespace boost::interprocess;

    constexpr std::string_view memBlockName { "Boost" };

    void create_shared_memory()
    {
        shared_memory_object sharedMemory { open_or_create, memBlockName.data(), read_write };
        sharedMemory.truncate(1024);

        std::cout << sharedMemory.get_name() << '\n';

        offset_t size;
        if (sharedMemory.get_size(size))
            std::cout << size << '\n';

        const bool removed = shared_memory_object::remove(memBlockName.data());
        std::cout << "Deleted: " << std::boolalpha << removed << '\n';
    }

    void read_write_shared_memory()
    {
        shared_memory_object sharedMemory { open_or_create, memBlockName.data(), read_write };
        sharedMemory.truncate(1024);

        mapped_region region {sharedMemory, read_write};

        int *i1 = static_cast<int*>(region.get_address());

        *i1 = 99;

        mapped_region region2 {sharedMemory, read_only};

        int *i2 = static_cast<int*>(region2.get_address());
        std::cout << *i2 << '\n';

        const bool removed = shared_memory_object::remove(memBlockName.data());
        std::cout << "Deleted: " << std::boolalpha << removed << '\n';
    }

    void delete_shared_memory()
    {
        const bool removed = shared_memory_object::remove(memBlockName.data());
        std::cout << "Deleted: " << std::boolalpha << removed << '\n';
    }
}


void Interprocess::TestAll([[maybe_unused]] int argc,
                           [[maybe_unused]] char** argv)
{
    // Interprocess::create_shared_memory();
    // Interprocess::read_write_shared_memory();
    Interprocess::delete_shared_memory();
};