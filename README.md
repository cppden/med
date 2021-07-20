[![Build](https://github.com/cppden/med/workflows/Linux/badge.svg)](https://github.com/cppden/med/actions?query=workflow%3ALinux)
[![Coverage](https://codecov.io/gh/cppden/med/branch/master/graph/badge.svg?token=FP0KOE0YAW)](https://codecov.io/gh/cppden/med)
[![License](https://img.shields.io/github/license/mashape/apistatus.svg)](../master/LICENSE)

# Meta-Encoder/Decoder

## Description
Zero-dependency (STL) header-only C++ library for definition of messages with generation of corresponding encoder and decoder via meta-programming.
MED is extensible library which can be adopted to support many type of encoding rules. Currently it includes:
* extensible implementation of non-ASN.1 octet encoding rules;
* initial implementation of Google ProtoBuf encoding rules;
* initial implementation of JSON encoding rules.

See [overview](doc/Overview.md) for details and samples.

See [repos](https://github.com/cppden/gtpu) for real-world examples of med usage.

## Code sample
```cpp
#include "med/med.hpp"

template <std::size_t TAG>
using T = med::value<med::fixed<TAG, uint8_t>>;
template <typename ...T>
using M = med::mandatory<T...>;
template <typename ...T>
using O = med::optional<T...>;
using L = med::length_t<med::value<uint8_t>>;

struct BYTE : med::value<uint8_t> {};
struct WORD : med::value<uint16_t> {};
struct TRIBYTE : med::value<med::bytes<3>> {};
struct IP4 : med::value<uint32_t>
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

struct DWORD : med::value<uint32_t> {};
struct URL : med::octet_string<med::min<5>, med::max<10>>, med::with_snapshot {};

struct MSG1 : med::sequence<
	M< BYTE >,
	M< T<0x21>, WORD >,
	M< L, URL >,
	O< T<0x49>, TRIBYTE >,
	O< T<0x89>, IP4 >,
	O< T<0x03>, DWORD >
>{};

struct MSG2 : med::set<
	M< tag<0x0b>,    BYTE >,
	M< tag<0x21>, L, WORD >,
	O< tag<0x49>, L, TRIBYTE >,
	O< tag<0x89>,    IP4 >,
	O< tag<0x22>, L, URL >
>{};

struct PROTO : med::choice<
	M<T<1>, MSG1>,
	M<T<2>, MSG2>
>{};
```

## Dependencies
Any modern C++ compiler with C++17 support (see travis for the selected ones).
