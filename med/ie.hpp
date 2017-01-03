/**
@file
TODO: define.

@copyright Denis Priyomov 2016
Distributed under the MIT License
(See accompanying file LICENSE or copy at https://opensource.org/licenses/MIT)
*/

#pragma once

#include <limits>

#include "field.hpp"
#include "multi_field.hpp"


namespace med {

template <std::size_t MAX>
struct max : std::integral_constant<std::size_t, MAX> {};

template <std::size_t MIN>
struct min : std::integral_constant<std::size_t, MIN> {};

template <std::size_t N>
struct arity : std::integral_constant<std::size_t, N> {};

using inf = max< std::numeric_limits<std::size_t>::max() >;


template <typename PTR>
class buffer;


}	//end: namespace med
