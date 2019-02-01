/**
@file
helper class to specify number of bits or bytes.

@copyright Denis Priyomov 2016-2018
Distributed under the MIT License
(See accompanying file LICENSE or visit https://github.com/cppden/med)
*/

#pragma once

#include <cstdint>

namespace med {

template <std::size_t NUM>
struct bits
{
	//static_assert(NUM > 0 && NUM <= 64, "INVALID NUMBER OF BITS");
	static constexpr std::size_t size = NUM;
};

template <std::size_t NUM>
struct bytes
{
	//static_assert(NUM > 0 && NUM <= 8, "INVALID NUMBER OF BYTES");
	static constexpr std::size_t size = NUM;
};

} //namespace med
