/**
@file
traits for IE of integral values

@copyright Denis Priyomov 2016-2017
Distributed under the MIT License
(See accompanying file LICENSE or copy at https://opensource.org/licenses/MIT)
*/

#pragma once

#include <type_traits>
#include <cstdint>

namespace med {

constexpr std::size_t bits_to_bytes(std::size_t num_bits)
{
	return (num_bits + 7) / 8;
}


/******************************************************************************
* Class:		value_traits<LEN>
* Description:	select min integer type to fit LEN bits
* Notes:		LEN is number of bits
******************************************************************************/
template <std::size_t LEN, typename Enabler = void> struct value_traits;

template <std::size_t LEN>
struct value_traits<LEN, std::enable_if_t<(LEN > 0 && LEN <= 8)>>
{
	static constexpr std::size_t bits = LEN;
	using value_type = uint8_t;
};

template <std::size_t LEN>
struct value_traits<LEN, std::enable_if_t<(LEN > 8 && LEN <= 16)>>
{
	static constexpr std::size_t bits = LEN;
	using value_type = uint16_t;
};

template <std::size_t LEN>
struct value_traits<LEN, std::enable_if_t<(LEN > 16 && LEN <= 32)>>
{
	static constexpr std::size_t bits = LEN;
	using value_type = uint32_t;
};

template <std::size_t LEN>
struct value_traits<LEN, std::enable_if_t<(LEN > 32 && LEN <= 64)>>
{
	static constexpr std::size_t bits = LEN;
	using value_type = uint64_t;
};

template <std::size_t LEN>
struct value_traits<LEN, std::enable_if_t<(LEN > 64)>>
{
	static constexpr std::size_t bits = LEN;
	struct value_type {uint8_t value[bits_to_bytes(LEN)];};
//	using param_type =  value_type const&;
};


template <typename VAL>
struct integral_traits
{
	static_assert(std::is_integral<VAL>(), "EXPECTED INTEGRAL TYPE");
	static constexpr std::size_t bits = sizeof(VAL) * 8;
	using value_type = VAL;
};

}	//end:	namespace med
