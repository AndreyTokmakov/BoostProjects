//============================================================================
// Name        : Spirit.h
// Created on  : 25.06.2022
// Author      : Tokmakov Andrei
// Version     : 1.0
// Copyright   : Your copyright notice
// Description : C++ Boost Spirit tests
//============================================================================



#include "Spirit.h"

#include <iostream>
#include <iomanip>
#include <stdexcept>

#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/phoenix_core.hpp>
#include <boost/spirit/include/phoenix_operator.hpp>

// https://panthema.net/2018/0912-Boost-Spirit-Tutorial/examples/

namespace Spirit::Parsers
{
    using namespace boost::spirit;

    void ParseTest() {
        const std::string input { "aaa\nbbb\nccc\n"};
        auto iter = input.begin();
        bool match = qi::parse(iter, input.end(), ascii::digit);

        std::cout << std::boolalpha << match << '\n';
        if (input.end() != iter)
            std::cout << std::string{iter, input.end()} << '\n';
    }

    void test1()
    {
        const std::string input = "12345";
        int out_int {0};

        qi::parse(input.begin(), input.end(), /* input string (iterators) */
                  qi::int_,/* parser grammar */
                  out_int); /* output fields */

        std::cout << "test1() parse result: "
                  << out_int << std::endl;
    }

    /** Parse an integer followed by a space and a string **/
    void test2()
    {
        const std::string input = "76131 Karlsruhe";
        int out_int {0};
        std::string out_string;

        qi::parse(input.begin(), input.end(), /* input string (iterators) */
                  qi::int_ >> ' ' >> *qi::char_,   /* parser grammar */
                  out_int, out_string);           /* output fields */

        std::cout << "test2() parse result: "
                  << out_int << " " << out_string << std::endl;
    }


    /** Parse a bracketed list of integers: **/
    void test3()
    {
        const std::string input = "[12345,42,5,]";
        std::vector<int> output;

        qi::parse(input.begin(), input.end(), /* input string (iterators) */
                  '[' >> *(qi::int_ >> ',') >> ']',/* parser gramma r*/
                  output); /* output list */

        std::cout << "test3() parse result: size " << output.size() << std::endl;
        for (const auto i : output)
            std::cout << i << std::endl;
    }


    // Helper to run a parser, check for errors, and capture the results.
    template <typename Parser, typename ... Args>
    void ParseOrDie(const std::string& input, const Parser& p, Args&& ... args)
    {
        std::string::const_iterator begin = input.begin(), end = input.end();
        bool ok = qi::parse(begin, end, p, std::forward<Args>(args) ...);
        if (!ok || begin != end) {
            std::cout << "Unparseable: " << std::quoted(std::string(begin, end)) << std::endl;
            throw std::runtime_error("Parse error");
        }
    }

    void test4()
    {
        const std::string input = "[12345,42,5]";
        std::vector<int> output;

        ParseOrDie(input,
                   '[' >> (qi::int_ % ',') >> ']', /* parser grammar with '%' operator */
                   output); /* output list */

        std::cout << "test4() parse result: size " << output.size() << std::endl;
        for (const auto i : output)
            std::cout << i << std::endl;
    }
}

namespace Spirit::Grammar {
    namespace qi = boost::spirit::qi;

    template <typename Parser, typename ... Args>
    void ParseOrDie(const std::string& input, const Parser& p, Args&& ... args)
    {
        std::string::const_iterator begin = input.begin(), end = input.end();
        const bool ok = qi::parse(begin, end, p, std::forward<Args>(args) ...);
        if (!ok || begin != end) {
            std::cout << "Unparseable: " << std::quoted(std::string(begin, end)) << std::endl;
            throw std::runtime_error("Parse error");
        }
    }

    struct ArithmeticGrammar1 : public qi::grammar<std::string::const_iterator> {
        // string iterator to parse
        using Iterator = std::string::const_iterator;

        // List of rule objects in the grammar. Templates just like qi::grammar.
        qi::rule<Iterator> start;

        ArithmeticGrammar1() : ArithmeticGrammar1::base_type(start) {
            // construct the grammar: just set "start" for now.
            start = qi::int_;
        }
    };

    void test1()
    {
        std::string input = "12345";

        ArithmeticGrammar1 g;
        ParseOrDie(input, g);
    }

    //------------------------------------------------------------------

    struct ArithmeticGrammar2 : public qi::grammar<std::string::const_iterator,int()>
    {
        using Iterator = std::string::const_iterator;
        // List of rule objects in the grammar. Each rule can have a return type.
        qi::rule<Iterator, int()> start;

        ArithmeticGrammar2() : ArithmeticGrammar2::base_type(start) {
            start %= qi::int_;
        }
    };

    void test2()
    {
        const std::string input = "12345";
        int result {0};

        // note that the grammar object does not contain any return values.
        ParseOrDie(input, ArithmeticGrammar2(), result);
        std::cout << "Parse result: '" << input << "' -> " << result << std::endl;
    }

    //------------------------------------------------------------------

    struct ArithmeticGrammar3 : public qi::grammar<std::string::const_iterator, int()> {
        using Iterator = std::string::const_iterator;

        // List of rule objects in the grammar. Now there are four rules and each returns an integer value.
        qi::rule<Iterator, int()> start, group, product, factor;

        ArithmeticGrammar3() : ArithmeticGrammar3::base_type(start) {
            start   = product >> *('+' >> product);
            product = factor >> *('*' >> factor);
            factor  = qi::int_ | group;
            group   = '(' >> start >> ')';
        }
    };

    void test3()
    {
        const std::string input = "1+2*3";
        int result {0};

        ParseOrDie(input, ArithmeticGrammar3(), result);
        std::cout << "test3() parse result: " << result << std::endl;
    }
}



void Spirit::TestAll()
{
    using namespace Parsers;

    ParseTest();

    // Parsers::test1();
    // Parsers::test2();
    // Parsers::test3();
    // Parsers::test4();


    // Grammar::test1();
    // Grammar::test2();
    // Grammar::test3();
}

