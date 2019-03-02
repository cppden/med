/**
@file
traits for IE of integral values

@copyright Denis Priyomov 2016-2017
Distributed under the MIT License
(See accompanying file LICENSE or copy at https://opensource.org/licenses/MIT)
*/

#pragma once

#include <type_traits>

#include "units.hpp"

namespace med {

struct empty_traits {};

using num_octs_t = uint32_t;

namespace detail {

template <
		typename VT,
		std::size_t NUM_BITS  = sizeof(VT) * 8,
		class EXT_TRAITS = empty_traits
		>
struct bit_traits : EXT_TRAITS
{
	static constexpr std::size_t bits = NUM_BITS;
	using value_type = VT;
};

template <
		std::size_t MIN_BITS = 0,
		std::size_t MAX_BITS = MIN_BITS,
		class EXT_TRAITS = empty_traits
		>
struct bits_traits : EXT_TRAITS
{
	static constexpr std::size_t min_bits = MIN_BITS;
	static constexpr std::size_t max_bits = MAX_BITS;
	using value_type = uint8_t;
};

template <
		typename VT,
		std::size_t MIN_BYTES = sizeof(VT),
		std::size_t MAX_BYTES = MIN_BYTES,
		class EXT_TRAITS = empty_traits
		>
struct octet_traits : EXT_TRAITS
{
	static constexpr std::size_t min_octets = MIN_BYTES;
	static constexpr std::size_t max_octets = MAX_BYTES;
	using value_type = VT;
};

} //end: namespace detail

template <std::size_t MIN>
struct min : std::integral_constant<std::size_t, MIN> {};

template <std::size_t MAX>
struct max : std::integral_constant<std::size_t, MAX> {};

template <class VT, class = void, class = void, class = void>
struct base_traits;

template <class VT>
struct base_traits<VT, void, void, void> : detail::bit_traits<VT> {};
template <class VT, class EXT_TRAITS>
struct base_traits<VT, EXT_TRAITS, void, void> : detail::bit_traits<VT, sizeof(VT)*8, EXT_TRAITS> {};
template <class VT, std::size_t BITS>
struct base_traits<VT, bits<BITS>, void, void> : detail::bit_traits<VT, BITS> {};
template <class VT, std::size_t BITS, class EXT_TRAITS>
struct base_traits<VT, bits<BITS>, EXT_TRAITS, void> : detail::bit_traits<VT, BITS, EXT_TRAITS> {};

constexpr std::size_t bits_to_bytes(std::size_t num_bits)
{
	return (num_bits + 7) / 8;
}

//number of least significant bits = number of MSBits in the last octet
constexpr uint8_t least_bits(std::size_t num_bits)
{
	uint8_t const last_octet_bits = num_bits % 8;
	return last_octet_bits ? last_octet_bits : 8;
}


/******************************************************************************
* Class:		value_traits<BITS>
* Description:	select min integer type to fit BITS bits
* Notes:		BITS is number of bits
******************************************************************************/
template <class EXT_TRAITS, std::size_t BITS, typename Enabler = void> struct value_traits;

template <class EXT_TRAITS, std::size_t BITS>
struct value_traits<EXT_TRAITS, BITS, std::enable_if_t<(BITS > 0 && BITS <= 8)>>
		: base_traits<uint8_t, bits<BITS>, EXT_TRAITS> {};

template <class EXT_TRAITS, std::size_t BITS>
struct value_traits<EXT_TRAITS, BITS, std::enable_if_t<(BITS > 8 && BITS <= 16)>>
		: base_traits<uint16_t, bits<BITS>, EXT_TRAITS> {};

template <class EXT_TRAITS, std::size_t BITS>
struct value_traits<EXT_TRAITS, BITS, std::enable_if_t<(BITS > 16 && BITS <= 32)>>
		: base_traits<uint32_t, bits<BITS>, EXT_TRAITS> {};

template <class EXT_TRAITS, std::size_t BITS>
struct value_traits<EXT_TRAITS, BITS, std::enable_if_t<(BITS > 32 && BITS <= 64)>>
		: base_traits<uint64_t, bits<BITS>, EXT_TRAITS> {};

template <class EXT_TRAITS ,std::size_t BITS>
struct value_traits<EXT_TRAITS, BITS, std::enable_if_t<(BITS > 64)>> : EXT_TRAITS
{
	static constexpr std::size_t bits = BITS;
	struct value_type {uint8_t value[bits_to_bytes(BITS)];};
//	using param_type =  value_type const&;
};

}	//end:	namespace med
