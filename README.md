[![Build](https://github.com/cppden/med/workflows/Linux/badge.svg)](https://github.com/cppden/med/actions?query=workflow%3ALinux)
[![Coverage](https://codecov.io/gh/cppden/med/branch/master/graph/badge.svg?token=FP0KOE0YAW)](https://codecov.io/gh/cppden/med)
[![License](https://img.shields.io/github/license/mashape/apistatus.svg)](../master/LICENSE)

# Meta-Encoder/Decoder

## Description
Zero-dependency (STL) header-only C++ library for definition of messages with generation of corresponding encoder and decoder via meta-programming.
MED is extensible library which can be adopted to support many type of encoding rules. Currently it includes:
* complete implementation of non-ASN.1 octet encoding rules;
* initial implementation of Google ProtoBuf encoding rules;
* initial implementation of JSON encoding rules.

See [overview](doc/Overview.md) for details and samples.

See [repos](https://github.com/cppden/gtpu) for real-world examples of med usage.

## Code sample
```cpp
#include "med/med.hpp"

template <std::size_t TAG>
using tag = med::value<med::fixed<TAG, uint8_t>>;
template <typename ...T>
using M = med::mandatory<T...>;
template <typename ...T>
using O = med::optional<T...>;
using L = med::length_t<med::value<uint8_t>>;

struct FIELD1 : med::value<uint8_t>
{
	static constexpr char const* name() { return "byte"; }
};

struct FIELD2 : med::value<uint16_t>
{
	static constexpr char const* name() { return "word"; }
};

struct FIELD3 : med::value<med::bytes<3>>
{
	static constexpr char const* name() { return "3byte"; }
};

struct FIELD4 : med::value<uint32_t>
{
	static constexpr char const* name() { return "ip-addr"; }
	std::string print() const
	{
		char sz[16];
		uint32_t ip = get();
		std::snprintf(sz, sizeof(sz), "%u.%u.%u.%u", uint8_t(ip >> 24), uint8_t(ip >> 16), uint8_t(ip >> 8), uint8_t(ip));
		return sz;
	}
};

struct FIELD5 : med::value<uint32_t>
{
	static constexpr char const* name() { return "dword"; }
};

struct VFLD1 : med::octet_string<med::min<5>, med::max<10>>, med::with_snapshot
{
	static constexpr char const* name() { return "url"; }
};

struct MSG1 : med::sequence<
	M< FIELD1 >, //V
	M< tag<0x21>, FIELD2 >, //TV
	M< L, FIELD5 >, //LV(fixed)
	O< tag<0x49>, FIELD3 >,
	O< tag<0x89>, FIELD4 >,
	O< tag<  3>, MSG1 >
>
{
	static constexpr char const* name() { return "Msg-One"; }
};

struct MSG2 : med::set< med::value<uint16_t>,
	M< tag<0x0b>,    FIELD1 >, //<TV>
	M< tag<0x21>, L, FIELD2 >, //<TLV>
	O< tag<0x49>, L, FIELD3 >, //[TLV]
	O< tag<0x89>,    FIELD4 >, //[TV]
	O< tag<0x22>, L, VFLD1 >   //[TLV(var)]
>
{
	static constexpr char const* name() { return "Msg-Set"; }
};


struct PROTO : med::choice< med::value<uint8_t>
	, med::tag<1, MSG1>
	, med::tag<2, MSG2>
>
{
};
```

## Dependencies
Any modern C++ compiler with C++17 support (see travis for the selected ones).
