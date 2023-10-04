/**============================================================================
Name        : SmartPointers.cpp
Created on  : 04.10.2023
Author      : Andrei Tokmakov
Version     : 1.0
Copyright   : Your copyright notice
Description : SmartPointers.cpp
============================================================================**/

#include "SmartPointers.h"

#include <iostream>
#include <string_view>
#include <list>

#include <boost/intrusive_ptr.hpp>
#include <boost/smart_ptr/intrusive_ref_counter.hpp>


namespace SmartPointers::Intrusive_Ref_Counter
{
    struct Item : boost::intrusive_ref_counter<Item>
    {
        std::string data;
        explicit Item(std::string data) : data { std::move(data) } {}

        bool operator<=>(Item const& rhs) const {
            return data < rhs.data;
        }
    };

    /*
    template <> struct fmt::formatter<Item> : fmt::formatter<std::string> {
    template <typename Ctx>
    auto format(Item const&val, Ctx &ctx) {
        return fmt::format_to(ctx.out(), "'{}'", val.data); }
    };
    */

};

void SmartPointers::TestAll()
{

};