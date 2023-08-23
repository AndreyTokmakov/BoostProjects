/**============================================================================
Name        : Tokenizer.cpp
Created on  : 23.08.2023
Author      : Andrei Tokmakov
Version     : 1.0
Copyright   : Your copyright notice
Description : Tokenizer
============================================================================**/

#include "Tokenizer.h"

#include <boost/tokenizer.hpp>
#include <string_view>
#include <iostream>

namespace Tokenizer
{
    using char_tokenizer = boost::tokenizer<boost::char_separator<char>>;
    using escaped_tokenizer = boost::tokenizer<boost::escaped_list_separator<char>>;
    using offset_separator = boost::tokenizer<boost::offset_separator>;

    void TokenizerString()
    {
        const std::string s = "Boost C++ Libraries";
        char_tokenizer tok {s};
        for (char_tokenizer::iterator it = tok.begin(); it != tok.end(); ++it)
            std::cout << *it << '\n';
    }

    void TokenizerString_Separator()
    {
        constexpr std::string_view s= "Boost C++ Libraries";
        boost::char_separator<char> sep{" "};
        char_tokenizer tok{s, sep};
        for (const auto &t : tok)
            std::cout << t << '\n';
    }

    void TokenizerString_Separator2()
    {
        constexpr std::string_view s = "Boost C++ Libraries";
        boost::char_separator<char> sep{" ", "+"};
        char_tokenizer tok{s, sep};
        for (const auto &t : tok)
            std::cout << t << '\n';
    }

    void TokenizerString_Separator_EmptyTokens()
    {
        constexpr std::string_view s = "Boost C++ Libraries";
        boost::char_separator<char> sep{" ", "+", boost::keep_empty_tokens};
        char_tokenizer tok{s, sep};
        for (const auto &t : tok)
            std::cout << t << '\n';
    }

    void TokenizerString_EscapedTokenizer()
    {
        constexpr std::string_view s = "Boost,\"C++ Libraries\"";
        escaped_tokenizer tok{s};
        for (const auto &t : tok)
            std::cout << t << '\n';
    }

    void TokenizerString_OffsetTokenizer()
    {
        constexpr std::string_view s = "Boost_C++_Libraries";
        int offsets[] = {5, 5, 9};
        boost::offset_separator sep{offsets, offsets + 3};
        offset_separator tok{s, sep};
        for (const auto &t : tok)
            std::cout << t << '\n';
    }
}

void Tokenizer::TestAll([[maybe_unused]] int argc,
                        [[maybe_unused]] char** argv)
{
    // TokenizerString();
    // TokenizerString_Separator();
    // TokenizerString_Separator2();
    // TokenizerString_Separator_EmptyTokens();
    // TokenizerString_EscapedTokenizer();
    TokenizerString_OffsetTokenizer();
};