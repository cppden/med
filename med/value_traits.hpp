/**
@file
traits for IE of integral values

@copyright Denis Priyomov 2016
Distributed under the MIT License
(See accompanying file LICENSE or copy at https://opensource.org/licenses/MIT)
*/

#pragma once

#include <type_traits>
#include <cstdint>

namespace med {

template<std::size_t BITS>
struct bits_to_bytes : std::integral_constant<std::size_t, (BITS + 7) / 8 > {};

/******************************************************************************
* Class:		value_traits<LEN>
* Description:	select min integer type to fit LEN bits
* Notes:		LEN is number of bits
******************************************************************************/
template <std::size_t LEN, typename Enabler = void> struct value_traits;

template <std::size_t LEN>
struct value_traits<LEN, std::enable_if_t<(LEN <= 8)>>
{
	enum { bits = LEN };
	using value_type = uint8_t;
};

template <std::size_t LEN>
struct value_traits<LEN, std::enable_if_t<(LEN > 8 and LEN <= 16)>>
{
	enum { bits = LEN };
	using value_type = uint16_t;
};

template <std::size_t LEN>
struct value_traits<LEN, std::enable_if_t<(LEN > 16 and LEN <= 32)>>
{
	enum { bits = LEN };
	using value_type = uint32_t;
};

template <std::size_t LEN>
struct value_traits<LEN, std::enable_if_t<(LEN > 32 and LEN <= 64)>>
{
	enum { bits = LEN };
	using value_type = uint64_t;
};

template <std::size_t LEN>
struct value_traits<LEN, std::enable_if_t<(LEN > 64)>>
{
	enum { bits = LEN };
	struct value_type {uint8_t value[bits_to_bytes<LEN>::value];};
//	using param_type =  value_type const&;
};

}	//end:	namespace med
