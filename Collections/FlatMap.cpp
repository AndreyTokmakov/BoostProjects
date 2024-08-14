/**============================================================================
Name        : FlatMap.cpp
Created on  : 12.08.2024
Author      : Andrei Tokmakov
Version     : 1.0
Copyright   : Your copyright notice
Description : FlatMap.cpp
============================================================================**/

#include "FlatMap.h"
#include <boost/container/flat_map.hpp>

#include <iostream>


namespace FlatMap
{
    void Sorted_Less()
    {
        boost::container::flat_map<int, int, std::less<>> my_map;

        my_map[1] = 1;
        my_map[3] = 3;
        my_map[2] = 2;

        for (const auto& [k, v]: my_map)
            std::cout <<  k << " = " << v << std::endl;
    }

    void Sorted_Greater()
    {
        boost::container::flat_map<int, int, std::greater<>> my_map;

        my_map[1] = 1;
        my_map[3] = 3;
        my_map[2] = 2;

        for (const auto& [k, v]: my_map)
            std::cout <<  k << " = " << v << std::endl;
    }
}

void FlatMap::TestAll()
{
    // FlatMap::Sorted_Less();
    FlatMap::Sorted_Greater();


};