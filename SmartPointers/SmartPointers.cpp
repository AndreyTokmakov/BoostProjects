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
#include <utility>

#include <boost/intrusive_ptr.hpp>
#include <boost/smart_ptr/intrusive_ref_counter.hpp>
#include <boost/smart_ptr/intrusive_ptr.hpp>


namespace
{
    struct Long
    {
        long value {0};

        explicit Long(long val = 0) : value {val} {
            std::cout << "Long(" << value << ")\n";
        }

        Long(const Long &obj) {
            this->value = obj.value;
            std::cout << "Long(" << value << ") [Copy constructor]\n";
        }

        Long(Long &&obj) noexcept: value {std::exchange(obj.value, 0)} {
            std::cout << "Long(" << value << ") [Move constructor]\n";
        }

        inline void setValue(long v) noexcept {
            value = v;
        }

        [[nodiscard]]
        inline long getValue() const noexcept {
            return value;
        }

        ~Long() {
            std::cout << "~Long(" << value << ")\n";
        }

        Long &operator=(const Long &right) {
            std::cout << "Right: Long(" << right.value << ")\n";
            std::cout << "[Copy assignment] (" << value << " -> " << right.value << ")" << std::endl;
            if (&right != this)
                value = right.value;
            return *this;
        }

        Long &operator=(long val) {
            std::cout << "[Copy assignment (from long)]" << std::endl;
            this->value = val;
            return *this;
        }

        Long &operator=(Long &&right) noexcept {
            std::cout << "[Move assignment operator]" << std::endl;
            if (this != &right) {
                this->value = std::exchange(right.value, 0);
            }
            return *this;
        }

        Long &operator*(const Long &right) noexcept {
            this->value *= right.value;
            return *this;
        }

        /** Postfix increment: **/
        Long operator++(int) {
            auto prev = *this;
            ++value;
            return prev;
        }

        /** Prefix increment: **/
        Long operator++() {
            ++value;
            return *this;
        }

        friend Long operator*(const Long &left, long v) noexcept {
            return Long(left.value * v);
        }

        friend std::ostream &operator<<(std::ostream &stream, const Long &l) {
            stream << l.value;
            return stream;
        }
    };

}

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


namespace SmartPointers::Intrusive_Ptr
{
    struct LongWithCounter: Long
    {
        explicit LongWithCounter(long v): Long(v) {
        }
    };

    void intrusive_ptr_add_ref([[maybe_unused]] LongWithCounter* ptr)
    {
        std::cout << "Called on creation. (" << ptr->getValue() << ")\n";
        // p->AddRef();
    }
    void intrusive_ptr_release([[maybe_unused]] LongWithCounter* ptr)
    {
        std::cout << "Called on deletion. (" << ptr->getValue() << ")\n";
        // p->Release();
    }


    void Test()
    {
        auto ptr = boost::intrusive_ptr<LongWithCounter>(new LongWithCounter(1));
    }
}


void SmartPointers::TestAll()
{
    // Intrusive_Ref_Counter::Test();
    // Intrusive_Ref_Counter::Test_SharedPtr();

    Intrusive_Ptr::Test();

};