#pragma once
/**
@file
ASN.1 BER tag definition

@copyright Denis Priyomov 2018
Distributed under the MIT License
(See accompanying file LICENSE or visit https://github.com/cppden/med)
*/

#include <cstdint>
#include <type_traits>


namespace med::asn::ber {

//no support for indefinite length as seems useless so far
constexpr std::size_t bits_in_byte = 8;

namespace detail {

template <typename T>
constexpr auto least_bits(T x) -> std::enable_if_t<std::is_integral_v<T>, uint8_t>
{
	if constexpr (std::is_signed<T>())
	{
		if constexpr (sizeof(T) <= sizeof(int))
		{
#ifdef __clang__
			return bits_in_byte * sizeof(int) - ((x != 0 && x != -1)
				? (__builtin_clz(x > 0 ? x : ~x) - 1)
				: bits_in_byte * sizeof(int) - 1);
#else
			return bits_in_byte * sizeof(int) - (__builtin_clz(x >= 0 ? x : ~x) - 1);
#endif
		}
		else if constexpr (sizeof(T) <= sizeof(long))
		{
#ifdef __clang__
			return bits_in_byte * sizeof(long) - ((x != 0 && x != -1)
				? (__builtin_clzl(x > 0 ? x : ~x) - 1)
				: bits_in_byte * sizeof(long) - 1);
#else
			return bits_in_byte * sizeof(long) - (__builtin_clzl(x >= 0 ? x : ~x) - 1);
#endif
		}
		else
		{
#ifdef __clang__
			return bits_in_byte * sizeof(long long) - ((x != 0 && x != -1)
				? (__builtin_clzll(x > 0 ? x : ~x) - 1)
				: bits_in_byte * sizeof(long long) - 1);
#else
			return bits_in_byte * sizeof(long long) - (__builtin_clzll(x >= 0 ? x : ~x) - 1);
#endif
		}
	}
	else
	{
		if constexpr (sizeof(T) <= sizeof(int))
		{
			return bits_in_byte * sizeof(int) - (x ? __builtin_clz(x) : bits_in_byte * sizeof(int) - 1);
		}
		else if constexpr (sizeof(T) <= sizeof(long))
		{
			return bits_in_byte * sizeof(long) - (x ? __builtin_clzl(x) : bits_in_byte * sizeof(long) - 1);
		}
		else
		{
			return bits_in_byte * sizeof(long long) - (x ? __builtin_clzll(x) : bits_in_byte * sizeof(long long) - 1);
		}
	}
}

} //end: namespace detail

struct length
{
	template <typename INT>
	static constexpr uint8_t bits(INT x)    { return detail::least_bits<INT>(x); }

	template <typename INT>
	static constexpr uint8_t bytes(INT x)   { return (bits<INT>(x) + bits_in_byte - 1) / bits_in_byte; }
};

} //end: namespace med::asn::ber

