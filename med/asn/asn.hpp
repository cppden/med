#pragma once

/**
@file
ASN.1

@copyright Denis Priyomov 2018
Distributed under the MIT License
(See accompanying file LICENSE or visit https://github.com/cppden/med)
*/

#include "ids.hpp"
#include "../value.hpp"
#include "../octet_string.hpp"

namespace med::asn {

using boolean = med::value<bool, traits<tag_value::BOOLEAN>>;
using null = med::empty;

using integer = med::value<int, traits<tag_value::INTEGER>>;
using uinteger = med::value<unsigned int, traits<tag_value::INTEGER>>;
using s_int = med::value<short int, traits<tag_value::INTEGER>>;
using su_int = med::value<short unsigned int, traits<tag_value::INTEGER>>;
using l_int = med::value<long int, traits<tag_value::INTEGER>>;
using lu_int = med::value<long unsigned int, traits<tag_value::INTEGER>>;

using real = med::value<double, traits<tag_value::REAL>>;
//using bit_string = med::bit_string<traits<tag_value::BIT_STRING>>;
using octet_string = med::octet_string<traits<tag_value::OCTET_STRING>>;

} //end: namespace med::asn

