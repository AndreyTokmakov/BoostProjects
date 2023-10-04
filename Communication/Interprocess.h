/**============================================================================
Name        : Interprocess.h
Created on  : 23.08.2023
Author      : Andrei Tokmakov
Version     : 1.0
Copyright   : Your copyright notice
Description : Interprocess
============================================================================**/

#ifndef BOOSTPROJECTS_INTERPROCESS_H
#define BOOSTPROJECTS_INTERPROCESS_H

#include <vector>
#include <string_view>

namespace Interprocess
{
    void TestAll(const std::vector<std::string_view>& params);
};

#endif //BOOSTPROJECTS_INTERPROCESS_H
