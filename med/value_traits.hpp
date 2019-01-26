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

struct default_extension_traits {};

template <class EXT_TRAITS, typename VAL, std::size_t NUM_BITS = sizeof(VAL) * 8>
struct base_traits : EXT_TRAITS
{
	static constexpr std::size_t bits = NUM_BITS;
	using value_type = VAL;
};


constexpr std::size_t bits_to_bytes(std::size_t num_bits)
{
	return (num_bits + 7) / 8;
}


/******************************************************************************
* Class:		value_traits<LEN>
* Description:	select min integer type to fit LEN bits
* Notes:		LEN is number of bits
******************************************************************************/
template <class EXT_TRAITS, std::size_t LEN, typename Enabler = void> struct value_traits;

template <class EXT_TRAITS, std::size_t LEN>
struct value_traits<EXT_TRAITS, LEN, std::enable_if_t<(LEN > 0 && LEN <= 8)>>
		: base_traits<EXT_TRAITS, uint8_t, LEN> {};

template <class EXT_TRAITS, std::size_t LEN>
struct value_traits<EXT_TRAITS, LEN, std::enable_if_t<(LEN > 8 && LEN <= 16)>>
		: base_traits<EXT_TRAITS, uint16_t, LEN> {};

template <class EXT_TRAITS, std::size_t LEN>
struct value_traits<EXT_TRAITS, LEN, std::enable_if_t<(LEN > 16 && LEN <= 32)>>
		: base_traits<EXT_TRAITS, uint32_t, LEN> {};

template <class EXT_TRAITS, std::size_t LEN>
struct value_traits<EXT_TRAITS, LEN, std::enable_if_t<(LEN > 32 && LEN <= 64)>>
		: base_traits<EXT_TRAITS, uint64_t, LEN> {};

template <class EXT_TRAITS ,std::size_t LEN>
struct value_traits<EXT_TRAITS, LEN, std::enable_if_t<(LEN > 64)>> : EXT_TRAITS
{
	static constexpr std::size_t bits = LEN;
	struct value_type {uint8_t value[bits_to_bytes(LEN)];};
//	using param_type =  value_type const&;
};

}	//end:	namespace med
