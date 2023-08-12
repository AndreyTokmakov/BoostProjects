//============================================================================
// Name        : Json.cpp
// Created on  : 12.10.2021.
// Author      : Tokmakov Andrey
// Version     : 1.0
// Copyright   : Your copyright notice
// Description : C++ Boost math src
//============================================================================

#include <boost/qvm/quat_traits.hpp>
#include <boost/qvm/quat_traits_array.hpp>
#include <boost/qvm/deduce_quat.hpp>
#include <boost/qvm/quat_access.hpp>
#include <boost/qvm/quat_operations.hpp>
#include <boost/qvm/quat.hpp>

#include <boost/qvm/vec_traits.hpp>
#include <boost/qvm/vec_traits_array.hpp>
#include <boost/qvm/deduce_vec.hpp>
#include <boost/qvm/vec_access.hpp>
#include <boost/qvm/swizzle.hpp>
#include <boost/qvm/swizzle2.hpp>
#include <boost/qvm/swizzle3.hpp>
#include <boost/qvm/swizzle4.hpp>
#include <boost/qvm/vec_operations.hpp>
#include <boost/qvm/vec_operations2.hpp>
#include <boost/qvm/vec_operations3.hpp>
#include <boost/qvm/vec_operations4.hpp>
#include <boost/qvm/quat_vec_operations.hpp>
#include <boost/qvm/vec_mat_operations.hpp>
#include <boost/qvm/map_vec_mat.hpp>
#include <boost/qvm/vec.hpp>

#include <boost/qvm/mat_traits.hpp>
#include <boost/qvm/mat_traits_array.hpp>
#include <boost/qvm/deduce_mat.hpp>
#include <boost/qvm/mat_access.hpp>
#include <boost/qvm/mat_operations.hpp>
#include <boost/qvm/mat_operations2.hpp>
#include <boost/qvm/mat_operations3.hpp>
#include <boost/qvm/mat_operations4.hpp>
#include <boost/qvm/map_mat_mat.hpp>
#include <boost/qvm/map_mat_vec.hpp>
#include <boost/qvm/mat.hpp>

#include <iostream>
#include <string>
#include <string_view>

#include "BoostMathTests.h"

namespace BoostMathTests::Quaternions {

    using namespace boost::qvm;

    template<typename _Ty>
    std::ostream& operator<<(std::ostream& stream, const quat<_Ty>& quat) {
        for (const auto& s: quat.a)
            stream << s << "  ";
        return stream;
    }

    void Test() {
        quat<float> rx = rotx_quat(3.14159f);
        std::cout << rx << std::endl;
    }

    void Test2() {
        vec<float,3> v = {0,0,7};

        [[maybe_unused]]
        mat<float,4,4> tr = translation_mat(v);
    }
}

void BoostMathTests::TestAll()
{
    // Quaternions::Test();
    Quaternions::Test2();
}