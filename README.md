# Meta-Encoder/Decoder

## Overview
```cpp
#include "med/codec.hpp"

template <std::size_t TAG>
using tag = med::cvalue<8, TAG>;
template <typename ...T>
using M = med::mandatory<T...>;
template <typename ...T>
using O = med::optional<T...>;
using L = med::value<8>;

struct FIELD1 : med::value<8>
{
	static constexpr char const* name() { return "byte"; }
};

struct FIELD2 : med::value<16>
{
	static constexpr char const* name() { return "word"; }
};

struct FIELD3 : med::value<24>
{
	static constexpr char const* name() { return "3byte"; }
};

struct FIELD4 : med::value<32>
{
	static constexpr char const* name() { return "ip-addr"; }
	std::string print() const
	{
		char sz[16];
		uint32_t ip = get();
		snprintf(sz, sizeof(sz)-1, "%u.%u.%u.%u", uint8_t(ip >> 24), uint8_t(ip >> 16), uint8_t(ip >> 8), uint8_t(ip));
		return sz;
	}
};

struct FIELD5 : med::value<32>
{
	static constexpr char const* name() { return "dword"; }
};

struct VFLD1 : med::octet_string<med::min<5>, med::max<10>>, med::with_snapshot
{
	static constexpr char const* name() { return "url"; }
};

struct MSG1 : med::sequence<
	M< FIELD1 >,
	M< tag<0x21>, FIELD2 >,
	O< tag<0x49>, FIELD3 >
>
{
	static constexpr char const* name() { return "Msg-One"; }
};

struct MSG2 : med::sequence<
	M< FIELD1 >, //V
	M< tag<0x21>, FIELD2 >, //TV
	M< L, FIELD5 >, //LV(fixed)
	O< tag<0x49>, FIELD3 >,
	O< tag<0x89>, FIELD4 >,
	O< tag<  3>, MSG1 >
>
{
	static constexpr char const* name() { return "Msg-Two"; }
};

struct MSG3 : med::sequence<
	M< L, FIELD1 >, //LV(fixed)
	M< tag<0x21>, L, FIELD2 >, //TLV(fixed)
	O< tag<0x49>, L, FIELD3 >, //TLV(fixed)
	O< tag<0x22>, L, VFLD1 > //TLV(var)
>
{
	static constexpr char const* name() { return "Msg-Three"; }
};

struct MSG4 : med::set< med::value<16>,
	M< tag<0x0b>,    FIELD1 >, //<TV>
	M< tag<0x21>, L, FIELD2 >, //<TLV>
	O< tag<0x49>, L, FIELD3 >, //[TLV]
	O< tag<0x89>,    FIELD4 >, //[TV]
	O< tag<0x22>, L, VFLD1 >   //[TLV(var)]
>
{
	static constexpr char const* name() { return "Msg-Set"; }
};


struct PROTO : med::choice< med::value<8>
	, med::tag<1, MSG1>
	, med::tag<2, MSG2>
	, med::tag<3, MSG3>
	, med::tag<4, MSG4>
>
{
};
```

## Description
C++ library for non-ASN.1 based definition of messages with generating corresponding encoder and decoder via meta-programming.

## Dependencies 
Any modern C++ compiler with C++14 support.
> Tested on gcc-5.1 and clang-3.8.
