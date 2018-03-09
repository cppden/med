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

template <uint8_t NUM>
struct bits
{
	static_assert(NUM > 0 && NUM <= 64, "INVALID NUMBER OF BITS");
	static constexpr uint8_t size = NUM;
};

template <uint8_t NUM>
struct bytes
{
	static_assert(NUM > 0 && NUM <= 8, "INVALID NUMBER OF BYTES");
	static constexpr uint8_t size = NUM;
};

} //namespace med
