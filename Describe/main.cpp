/**============================================================================
Name        : Describe.cpp
Created on  : 23.05.2024
Author      : Andrei Tokmakov
Version     : 1.0
Copyright   : Your copyright notice
Description : Boost Experiments modules tests
============================================================================**/

#include <iostream>
#include <vector>
#include <optional>

#include <boost/mp11.hpp>
#include <boost/describe.hpp>
#include <boost/core/demangle.hpp>

using namespace boost::mp11;

template<class T> std::string name()
{
    return boost::core::demangle( typeid(T).name() );
}

template<class T> using promote = typename std::common_type<T, int>::type;

template<class T, class U> using result =
        typename std::common_type<promote<T>, promote<U>>::type;

template<class T1, class T2> void test_result( mp_list<T1, T2> const& )
{
    using T3 = decltype( T1() + T2() );
    using T4 = result<T1, T2>;

    std::cout << ( std::is_same<T3, T4>::value? "[PASS] ": "[FAIL] " )
              << name<T1>() << " + " << name<T2>() << " -> " << name<T3>()
              << ", result: " << name<T4>() << std::endl;
}


namespace DescribeTests::Enums
{
    enum E
    {
        v1 = 11,
        v2,
        v3 = 5
    };

    BOOST_DESCRIBE_ENUM(E, v1, v2, v3);

    void PrintEnum()
    {
        boost::mp11::mp_for_each< boost::describe::describe_enumerators<E> >([](auto D){
            std::printf( "%s: %d\n", D.name, D.value );
        });
    }
}


namespace DescribeTests::Structs_Classes
{
    struct X
    {
        int m1;
        int m2;
    };

BOOST_DESCRIBE_STRUCT(X, (), (m1, m2))
}



int main([[maybe_unused]] int argc,
         [[maybe_unused]] char** argv)
{
    const std::vector<std::string_view> args(argv + 1, argv + argc);

    DescribeTests::Enums::PrintEnum();

    return EXIT_SUCCESS;
}
