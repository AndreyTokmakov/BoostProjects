/**============================================================================
Name        : DateTime.h
Created on  : 03.09.2022
Author      : Tokmakov Andrey
Version     : 1.0
Copyright   : Your copyright notice
Description : DateTime
============================================================================**/

#define _SILENCE_CXX17_ALLOCATOR_VOID_DEPRECATION_WARNING

#include "DateTime.h"

#include <boost/date_time/local_time/local_time.hpp>

#include <algorithm>
#include <cassert>

#include <iostream>
#include <thread>
#include <string>
#include <string_view>
#include <sstream>

namespace DateTime
{
    void Not_a_DateTime() {
        /******  ******/
        boost::gregorian::date d(2004, boost::gregorian::Feb, 29);
        boost::posix_time::time_duration td(12, 34, 56, 789);
        std::stringstream ss;

        ss << d << ' ' << td;
        boost::posix_time::ptime pt(boost::posix_time::not_a_date_time);
        std::cout << pt << std::endl; // "not-a-date-time"
    }


    void Test2() {
        boost::gregorian::date date{ 2014, 1, 31 };
        std::cout << date.year() << '\n';
        std::cout << date.month() << '\n';
        std::cout << date.day() << '\n';
        std::cout << date.day_of_week() << '\n';
        std::cout << date.end_of_month() << '\n';
    }

    void DateFrom_Clock()
    {
        boost::gregorian::date d = boost::gregorian::day_clock::universal_day();
        std::cout << d.year() << '\n';
        std::cout << d.month() << '\n';
        std::cout << d.day() << '\n';
    }

    void DateFrom_String()
    {
        boost::gregorian::date d = boost::gregorian::date_from_iso_string("20190131");
        std::cout << d.year() << '\n';
        std::cout << d.month() << '\n';
        std::cout << d.day() << '\n';
    }

    void Date_Duration()
    {
        boost::gregorian::date d1{ 2014, 1, 31 };
        boost::gregorian::date d2{ 2014, 2, 28 };
        boost::gregorian::date_duration dd = d2 - d1;
        std::cout << dd.days() << '\n';
    }

    void Specialized_Duration()
    {
        boost::gregorian::date_duration dd{ 4 };
        std::cout << dd.days() << '\n';

        boost::gregorian::weeks ws{ 4 };
        std::cout << ws.days() << '\n';

        boost::gregorian::months ms{ 4 };
        std::cout << ms.number_of_months() << '\n';

        boost::gregorian::years ys{ 4 };
        std::cout << ys.number_of_years() << '\n';
    }

    void Period()
    {
        boost::gregorian::date d1{ 2014, 1, 1 };
        boost::gregorian::date d2{ 2014, 2, 28 };
        boost::gregorian::date_period dp{ d1, d2 };
        boost::gregorian::date_duration dd = dp.length();

        std::cout << dd.days() << '\n';
    }
};

void DateTime::TestAll()
{
    // Not_a_DateTime();
    Test2();

    // DateFrom_Clock();
    // DateFrom_String();
    // Date_Duration();
    // Specialized_Duration();
    // Period();

};
