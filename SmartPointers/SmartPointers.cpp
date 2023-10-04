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
#include <memory>

#include <boost/intrusive_ptr.hpp>
#include <boost/smart_ptr/intrusive_ref_counter.hpp>


namespace SmartPointers::Intrusive_Ref_Counter
{
    struct String : boost::intrusive_ref_counter<String>
    {
        std::string data;
        explicit String(std::string data) : data { std::move(data) } {}

        bool operator<=>(String const& rhs) const {
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


    void Test()
    {
        using Item = boost::intrusive_ptr<String>;
        using List = std::list<Item>;

        List listOrig;
        for (auto name : { "one", "two", "three" })
            listOrig.emplace_back(new String(name));


        List list2 (listOrig.begin(), listOrig.end()),
             list3 (listOrig.begin(), listOrig.end());

        for (const Item& entry: listOrig){
            std::cout << entry->data << " (" << entry.get() << ")"
                      << " | use_count = " << entry->use_count()
                      << std::endl;
        }

        std::cout << std::endl;

        for (const Item& entry: list2) {
            std::cout << entry->data << " (" << entry.get() << ")"
                      << " | use_count = " << entry->use_count()
                      << std::endl;
        }
    }

    void Test_SharedPtr()
    {
        using Item = std::shared_ptr<String>;
        using List = std::list<Item>;

        List listOrig;
        for (auto name : { "one", "two", "three" })
            listOrig.emplace_back(new String(name));


        List list2 (listOrig.begin(), listOrig.end()),
                list3 (listOrig.begin(), listOrig.end());

        for (const Item& entry: listOrig){
            std::cout << entry->data << " (" << entry.get() << ")"
                      << " | use_count = " << entry->use_count()
                      << std::endl;
        }

        std::cout << std::endl;

        for (const Item& entry: list2) {
            std::cout << entry->data << " (" << entry.get() << ")"
                      << " | use_count = " << entry->use_count()
                      << std::endl;
        }
    }
};

void SmartPointers::TestAll()
{
    Intrusive_Ref_Counter::Test();
    Intrusive_Ref_Counter::Test_SharedPtr();

};