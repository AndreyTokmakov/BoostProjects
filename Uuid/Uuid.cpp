/**============================================================================
Name        : Uuid.cpp
Created on  : 03.09.2022
Author      : Tokmakov Andrey
Version     : 1.0
Copyright   : Your copyright notice
Description : Uuid
============================================================================**/

#include "Uuid.h"

#include <iostream>
#include <string>

#include <thread>

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/lexical_cast.hpp>

namespace Generators {

    void Generates_Random_UUID()
    {
        boost::uuids::random_generator generator;

        boost::uuids::uuid uuid = generator();
        std::cout << uuid << '\n';

        boost::uuids::uuid uuid1 = generator();
        std::cout << uuid1 << '\n';
    }

    void Uuid_Attributes()
    {
        boost::uuids::random_generator generator;
        boost::uuids::uuid uuid = generator();

        std::cout << uuid << '\n';
        std::cout << uuid.size() << '\n';
        std::cout << std::boolalpha << uuid.is_nil() << '\n';
        std::cout << uuid.variant() << '\n';
        std::cout << uuid.version() << '\n';
    }

    void String_Generator()
    {
        boost::uuids::string_generator string_generator;
        boost::uuids::uuid uuid = string_generator("CF77C981-F61B-7817-10FF-D916FCC3EAA4");

        std::cout << uuid << '\n';
        std::cout << uuid.variant() << '\n';
    }


    void String_Generator_Tests() {
        boost::uuids::string_generator generator;

        boost::uuids::uuid u1 = generator("{01234567-89ab-cdef-0123-456789abcdef}");
        std::cout << u1 << '\n';

        boost::uuids::uuid u2 = generator(L"01234567-89ab-cdef-0123-456789abcdef");
        std::cout << u2 << '\n';

        boost::uuids::uuid u3 = generator(std::string("0123456789abcdef0123456789abcdef"));
        std::cout << u3 << '\n';

        boost::uuids::uuid u4 = generator(std::wstring(L"01234567-89AB-CDEF-0123-456789ABCDEF"));
        std::cout << u4 << '\n';
    }

    void Name_Generator()
    {
        boost::uuids::string_generator string_generator;
        boost::uuids::uuid uuid = string_generator("CF77C981-F61B-7817-10FF-D916FCC3EAA4");

        boost::uuids::name_generator name_gen(uuid);
        std::cout << name_gen("theboostcpplibraries.com") << '\n';
    }


    void Lexical_Cast()
    {
        boost::uuids::random_generator generator;
        boost::uuids::uuid uuid = generator();

        std::cout << boost::lexical_cast<std::string>(uuid) << '\n';
    }
}

void Uuid::TestAll() {

    Generators::Generates_Random_UUID();
    // Generators::Uuid_Attributes();

    // Generators::String_Generator();
    // Generators::String_Generator_Tests();

    // Generators::Name_Generator();

    // Generators::Lexical_Cast();

}