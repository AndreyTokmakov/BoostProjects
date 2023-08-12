//============================================================================
// Name        : Json.cpp
// Created on  : 17.08.2021
// Author      : Tokmakov Andrey
// Version     : 1.0
// Copyright   : Your copyright notice
// Description : C++ JSON Boost src
//============================================================================

#include <iostream>
#include <string>
#include <string_view>

#include "Json.h"

#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/foreach.hpp>

// Eigen includes:
// #include <Core>
// #include <Dense>

// NOTE: To be able to change the string comparison implementation
#define EQUAL(str1, str2) (str1 == str2)


namespace JsonBoost {

    namespace pt = boost::property_tree;
    using PTree = pt::ptree;

    void Simple_Parse()
    {
        constexpr std::string_view path {R"(../../BoostProject/data/test.json)"};

        boost::property_tree::ptree pt;
        boost::property_tree::read_json( path.data(), pt );

        BOOST_FOREACH( boost::property_tree::ptree::value_type const& rowPair, pt.get_child( "" ) )
        {
            std::cout << rowPair.first << ": " << std::endl;
            BOOST_FOREACH( boost::property_tree::ptree::value_type const& itemPair, rowPair.second )
            {
                std::cout << "\t" << itemPair.first << " ";
                BOOST_FOREACH( boost::property_tree::ptree::value_type const& node, itemPair.second )  {
                    std::cout << node.second.get_value<std::string>() << " ";
                }
                std::cout << std::endl;
            }
            std::cout << std::endl;
        }
        std::cout << std::endl;
    }


    void Parse(const PTree& node)
    {
        for (const auto& entry : node) {
            auto v = entry.second.get_value<double>();
            std::cout << v << std::endl;
        }
    }

    void Parse_EigenVector()
    {
        constexpr std::string_view path {R"(../data/test.json)"};
        boost::property_tree::ptree pt;
        boost::property_tree::read_json( path.data(), pt );

        for (const auto& value : pt.get_child( "" )) {
            if (EQUAL(value.first, "electron")) {
                for (const auto& v : value.second.get_child( "" )) {
                    if (EQUAL(v.first, "val")) {
                        Parse(v.second.get_child( "" ));
                    }
                }
            }
        }
    }

    template <typename T>
    struct my_id_translator
    {
        typedef T internal_type;
        typedef T external_type;

        boost::optional<T> get_value(const T &v) { return  v.substr(1, v.size() - 2) ; }
        boost::optional<T> put_value(const T &v) { return '"' + v +'"'; }
    };

    void WriteJsonTest_COUT()
    {
        boost::property_tree::ptree root;
        root.put<int>("height", 123);
        root.put("some.complex.path", "bonjour");

        boost::property_tree::ptree fish1;
        fish1.put_value("blue");
        boost::property_tree::ptree  fish2;
        fish2.put_value("yellow");
        root.push_back(std::make_pair("fish", fish1));
        root.push_back(std::make_pair("fish", fish2));

        auto& modellingData = root.put_child( "modellingData", boost::property_tree::ptree() );
        auto& tooth11 = modellingData.add_child("11", boost::property_tree::ptree());
        auto& axes = tooth11.add_child("axes", boost::property_tree::ptree());
        tooth11.add_child("origin", boost::property_tree::ptree());
        tooth11.add_child("quaternion", boost::property_tree::ptree());

        axes.put<bool>("nice", true);
        // axes.add_child("", boost::property_tree::ptree()).put_value(0.2092447823860213);

        /*
        axes.put_value(-0.9739150550883403);
        axes.put_value(0.2092447823860213);
        axes.put_value(0.08778431816960128);
        axes.put_value(0.2262163979858106);
        axes.put_value(0.9256145310580007);
        axes.put_value(0.30342030448966806);
        axes.put_value(-0.01776532491783859);
        axes.put_value(0.3153638547731215);
        axes.put_value(-0.9488045279894068);
        */

        boost::property_tree::write_json(std::cout, root);
    }

    void WriteJsonTest_ToFile() {
        boost::property_tree::ptree root;
        root.put<int>("height", 1223);
        root.put("some.complex.path", "bonjour");

        boost::property_tree::ptree fish1;
        fish1.put_value("blue");
        boost::property_tree::ptree  fish2;
        fish2.put_value("yellow");
        root.push_back(std::make_pair("fish", fish1));
        root.push_back(std::make_pair("fish", fish2));

        boost::property_tree::write_json(R"(/home/andtokm/Projects/CppProjects/BoostProject/data/out.json)", root);
    }
}

void JsonBoost::TestAll() 
{
    // Simple_Parse();
    Parse_EigenVector();

    // WriteJsonTest_COUT();
    // WriteJsonTest_COUT2();
    // WriteJsonTest_ToFile();
}
