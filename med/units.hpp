/**
@file
helper class to specify number of bits or bytes.

@copyright Denis Priyomov 2016-2018
Distributed under the MIT License
(See accompanying file LICENSE or visit https://github.com/cppden/med)
*/

#pragma once

#include <cstdint>
#include <type_traits>
#include <limits>

namespace med {

enum class nbits : std::size_t {};
enum class nbytes : std::size_t {};

template <std::size_t N> struct bits {};

template <std::size_t N> struct bytes {};

template <std::size_t N> struct min {};

template <std::size_t N> struct max : std::integral_constant<std::size_t, N> {};

template <std::size_t N> struct arity : std::integral_constant<std::size_t, N> {};
template <std::size_t N> struct pmax : std::integral_constant<std::size_t, N> {};
using inf = pmax< std::numeric_limits<std::size_t>::max() >;

} //namespace med
