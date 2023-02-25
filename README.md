[![Build](https://github.com/cppden/med/workflows/Linux/badge.svg)](https://github.com/cppden/med/actions?query=workflow%3ALinux)
[![Coverage](https://codecov.io/gh/cppden/med/branch/master/graph/badge.svg?token=FP0KOE0YAW)](https://codecov.io/gh/cppden/med)
[![License](https://img.shields.io/github/license/mashape/apistatus.svg)](../master/LICENSE)

# Meta-Encoder/Decoder

## Description
Zero-dependency (STL) header-only C++ library for definition of messages with compile-time generation of corresponding encoder/decoder/printer.
MED is extensible library which can be adopted to support many type of encoding rules. Currently it includes:
* extensible implementation of non-ASN.1 octet encoding rules;
* incomplete implementation of ASN.1 BER;
* initial implementation of Google ProtoBuf encoding rules;

See [overview](doc/Overview.md) for details and samples.

See [repos](https://github.com/cppden/gtpu) for examples of med usage.

# Usage
## Define your protocol with MED
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

## Encode
```cpp
//a buffer to be written with binary data of encoded message
uint8_t buffer[100];
//create encoding context to define input/output/auxiliar
med::encoder_context<> ctx{ buffer };
//the protocol to encode
PROTO proto;
//set particular message in the protocol
auto& msg = proto.ref<MSG2>();
//set particular fields in the message
msg.ref<BYTE>().set(0x12);
msg.ref<WORD>().set(0x3456);
//encode the protocol with octet-encoder into given context
encode(med::octet_encoder{ctx}, proto);
//now the buffer holds encoded message of size ctx.buffer().get_offset()
```

## Decode
```cpp
//a binary message received in a buffer of size num_bytes
PROTO proto;
med::decoder_context<> ctx;
ctx.reset(buffer, num_bytes);
decode(med::octet_decoder{ctx}, proto);

if (auto const* msg = proto.get<MSG1>())
{
	//read any message field needed
}
else if (auto const* msg = proto.get<MSG2>())
{
	//read any message field needed
}
```

## Print
```cpp
//decode first (see above)
decode(med::octet_decoder{ctx}, proto);
//use your sink to consume traces, e.g. to print them to console
your_sink sink{};
//trace protocol via your sink
med::print(sink, proto);
```

## Dependencies
Any modern C++ compiler with C++20 support (see CI for the selected ones).
