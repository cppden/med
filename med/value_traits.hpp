/**
@file
traits for IE of integral values

@copyright Denis Priyomov 2016-2017
Distributed under the MIT License
(See accompanying file LICENSE or copy at https://opensource.org/licenses/MIT)
*/

#pragma once

#include <type_traits>
#include <array>

#include "units.hpp"
#include "traits.hpp"

namespace med {

using num_octs_t = uint32_t;

constexpr std::size_t bits_to_bytes(std::size_t num_bits)
{
	return (num_bits + 7) / 8;
}

//number of least significant bits = number of MSBits in the last octet
constexpr uint8_t calc_least_bits(std::size_t num_bits)
{
	uint8_t const last_octet_bits = num_bits % 8;
	return last_octet_bits ? last_octet_bits : 8;
}

namespace detail {

template <typename T, std::size_t BITS, std::size_t OFS, class... EXT_TRAITS>
struct bit_traits : EXT_TRAITS...
{
	static constexpr std::size_t bits = BITS;
	static constexpr std::size_t offset = OFS;
	using value_type = T;
};

template <
		typename VT,
		std::size_t MIN_BYTES = sizeof(VT),
		std::size_t MAX_BYTES = MIN_BYTES,
		class... EXT_TRAITS
		>
struct octet_traits : EXT_TRAITS...
{
	static constexpr std::size_t min_octets = MIN_BYTES;
	static constexpr std::size_t max_octets = MAX_BYTES;
	using value_type = VT;
};

//infer value_type to hold given bits
template <std::size_t BITS, typename Enabler = void> struct bits_infer;

template <std::size_t BITS> requires (BITS == 0)
struct bits_infer<BITS>
{
	using type = void;
};

template <std::size_t BITS> requires (BITS > 0 && BITS <= 8)
struct bits_infer<BITS>
{
	using type = uint8_t;
};

template <std::size_t BITS> requires (BITS > 8 && BITS <= 16)
struct bits_infer<BITS>
{
	using type = uint16_t;
};

template <std::size_t BITS> requires (BITS > 16 && BITS <= 32)
struct bits_infer<BITS>
{
	using type = uint32_t;
};

template <std::size_t BITS> requires (BITS > 32 && BITS <= 64)
struct bits_infer<BITS>
{
	using type = uint64_t;
};

template <std::size_t BITS> requires (BITS > 64)
struct bits_infer<BITS>
{
	using type = std::array<uint8_t, bits_to_bytes(BITS)>;
};

} //end: namespace detail

/******************************************************************************
* Class:		value_traits
* Description:	select min integer type to fit BITS bits
* Notes:		BITS is number of bits
******************************************************************************/
template <class T, class... EXT_TRAITS>
struct value_traits : detail::bit_traits<T, sizeof(T)*8, 0, EXT_TRAITS...> {};

template <std::size_t BITS, std::size_t OFS, class... EXT_TRAITS>
struct value_traits<bits<BITS, OFS>, EXT_TRAITS...>
	: detail::bit_traits<typename detail::bits_infer<BITS>::type, BITS, OFS, EXT_TRAITS...> {};

template <std::size_t BYTES, class... EXT_TRAITS>
struct value_traits<bytes<BYTES>, EXT_TRAITS...>
	: detail::bit_traits<typename detail::bits_infer<BYTES*8>::type, BYTES*8, 0, EXT_TRAITS...> {};

}	//end:	namespace med
