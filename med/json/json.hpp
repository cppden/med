/**
@file
Google JSON definitions

@copyright Denis Priyomov 2018
Distributed under the MIT License
(See accompanying file LICENSE or visit https://github.com/cppden/med)
*/

#pragma once

#include <cstdint>

#include "../value.hpp"
#include "../octet_string.hpp"

namespace med::json {

using boolean = med::value<bool>;
using integer = med::value<int>;
//using longint = med::value<long long int>;
//using unsignedint = med::value<int>;
//using unsignedlong = med::value<long long int>;
//using number  = med::value<double>;
//using null = med::value<void>;
using string  = med::ascii_string<>;
//using uint64 = med::value<uint64_t>;


} //end: namespace med::json
