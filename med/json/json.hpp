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
#include "../set.hpp"
#include "../sequence.hpp"
#include "../buffer.hpp"
#include "../encoder_context.hpp"

namespace med::json {

using boolean = med::value<bool>;
using integer = med::value<int>;
using longint = med::value<long long int>;
using unsignedint = med::value<unsigned int>;
using unsignedlong = med::value<unsigned long long int>;
using number  = med::value<double>;
//using null = med::value<void>;
using string  = med::ascii_string<>;

template <class ...IEs>
struct object : set<value<std::size_t>, IEs...>{};

template <class ...IEs>
struct array : sequence<IEs...>{};

using encoder_context = med::encoder_context<buffer<char*>>;

} //end: namespace med::json
