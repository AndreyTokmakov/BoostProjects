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
        constexpr std::string_view path {R"(../data/test.json)"};

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

    void WriteJsonTest_ToFile()
    {
        constexpr std::string_view path {R"(../data/out.json)"};

        boost::property_tree::ptree root;
        root.put<int>("height", 1223);
        root.put("some.complex.path", "bonjour");

        boost::property_tree::ptree fish1;
        fish1.put_value("blue");
        boost::property_tree::ptree  fish2;
        fish2.put_value("yellow");
        root.push_back(std::make_pair("fish", fish1));
        root.push_back(std::make_pair("fish", fish2));

        boost::property_tree::write_json(path.data(), root);
    }
}

namespace JsonBoost::AccessingData
{
    void accessing_data ()
    {
        PTree pt;
        pt.put("C:.Windows.System", "20 files");

        PTree &c = pt.get_child("C:");
        PTree &windows = c.get_child("Windows");
        PTree &system = windows.get_child("System");
        std::cout << system.get_value<std::string>() << '\n';
    }

    void accessing_data2()
    {
        PTree pt;
        pt.put(PTree::path_type{"C:\\Windows\\System", '\\'}, 20);
        pt.put(PTree::path_type{"C:\\Windows\\Cursors", '\\'}, 50);

        PTree &windows = pt.get_child(PTree::path_type{"C:\\Windows", '\\'});
        int files = 0;
        for (const auto &p : windows) {
            std::cout << "Path: " << p.first << ", files count: " << p.second.get_value<int>() << std::endl;
            files += p.second.get_value<int>();
        }
        std::cout << files << '\n';
    }

    struct string_to_int_translator
    {
        using internal_type = std::string;
        using external_type = int;

        boost::optional<int> get_value(const std::string &s)
        {
            char *c;
            long l = std::strtol(s.c_str(), &c, 10);
            return boost::make_optional(c != s.c_str(), static_cast<int>(l));
        }
    };

    void accessing_data_translator()
    {
        PTree pt;
        pt.put(PTree::path_type{"C:\\Windows\\System", '\\'}, "20 files");
        pt.put(PTree::path_type{"C:\\Windows\\Cursors", '\\'}, "50 files");

        string_to_int_translator tr {};
        const int files = pt.get<int>(PTree::path_type{"c:\\windows\\system", '\\'}, tr) +
                pt.get<int>(PTree::path_type{"c:\\windows\\cursors", '\\'}, tr);

        std::cout << files << '\n';
    }

    void get_optional()
    {
        {
            PTree root;
            root.put("C:", "20 files");
            boost::optional<std::string> c = root.get_optional<std::string>("C:");
            if (c.is_initialized()) {
                std::cout << "Value: " << c.value() << std::endl;
            }
        }

        {
            PTree root;
            root.put("D:", "20 files");
            boost::optional<std::string> c = root.get_optional<std::string>("C:");
            if (!c.is_initialized()) {
                std::cout << "Not exists\n";
            }
        }
    }

    void get_child_optional()
    {
        PTree root;
        root.put("C:.Windows.System", "Some_Secret");
        if (boost::optional<PTree&> node = root.get_child_optional("C:"); node) {
            if (boost::optional<PTree&> windows = node.value().get_child_optional("Windows"); windows) {
                if (boost::optional<std::string> sys = windows.value().get_optional<std::string>("System"); sys)
                {
                    std::cout << "System: " << sys.value() << std::endl;
                }
            }
        }
    }

    void various_member_functions ()
    {
        PTree root;
        {
            root.put("C:.Windows.System", "20 files");

            boost::optional<std::string> c = root.get_optional<std::string>("C:");
            std::cout << std::boolalpha << c.is_initialized() << '\n';

            root.put_child("D:.Program Files", PTree{"50 files"});
            root.add_child("D:.Program Files", PTree{"60 files"});
        }

        PTree d = root.get_child("D:");
        for (const auto &p : d)
            std::cout << p.second.get_value<std::string>() << '\n';

        boost::optional<PTree&> e = root.get_child_optional("E:");
        std::cout << e.is_initialized() << '\n';
    }
}

void JsonBoost::TestAll() 
{
    // Simple_Parse();
    // Parse_EigenVector();

    // WriteJsonTest_COUT();
    // WriteJsonTest_COUT2();
    // WriteJsonTest_ToFile();

    // AccessingData::accessing_data();
    // AccessingData::accessing_data2();
    // AccessingData::accessing_data_translator();
    // AccessingData::various_member_functions();


    // AccessingData::get_optional();
    AccessingData::get_child_optional();
}
