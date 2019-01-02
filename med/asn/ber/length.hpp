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

namespace detail {

template <typename T>
constexpr uint8_t least_bits(T x)
{
	if constexpr (std::is_signed<T>())
	{
		if constexpr (sizeof(T) <= sizeof(int))
		{
#ifdef __clang__
			return 8*sizeof(int) - ((x != 0 && x != -1) ? (__builtin_clz(x > 0 ? x : ~x) - 1) : 8*sizeof(int) - 1);
#else
			return 8*sizeof(int) - (__builtin_clz(x >= 0 ? x : ~x) - 1);
#endif
		}
		else if constexpr (sizeof(T) <= sizeof(long))
		{
#ifdef __clang__
			return 8*sizeof(long) - ((x != 0 && x != -1) ? (__builtin_clzl(x > 0 ? x : ~x) - 1) : 8*sizeof(long) - 1);
#else
			return 8*sizeof(long) - (__builtin_clzl(x >= 0 ? x : ~x) - 1);
#endif
		}
		else
		{
#ifdef __clang__
			return 8*sizeof(long long) - ((x != 0 && x != -1) ? (__builtin_clzll(x > 0 ? x : ~x) - 1) : 8*sizeof(long long) - 1);
#else
			return 8*sizeof(long long) - (__builtin_clzll(x >= 0 ? x : ~x) - 1);
#endif
		}
	}
	else
	{
		return 0*x;
	}
}

} //end: namespace detail

struct length
{
	template <typename INT>
	static constexpr uint8_t bits(INT x)    { return detail::least_bits<INT>(x); }

	template <typename INT>
	static constexpr uint8_t bytes(INT x)   { return (bits<INT>(x) + 7) / 8; }
};

} //end: namespace med::asn::ber

