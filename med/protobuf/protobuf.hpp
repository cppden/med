/**
@file
Google Protobuf definitions

@copyright Denis Priyomov 2018
Distributed under the MIT License
(See accompanying file LICENSE or visit https://github.com/cppden/med)
*/

#pragma once

#include <cstdint>

#include "../value.hpp"


namespace med::protobuf {

enum class wire_type : uint8_t
{
	VARINT    = 0, //int32, int64, uint32, uint64, sint32, sint64, bool, enum
	BITS_64   = 1, //fixed64, sfixed64, double
	LEN_DELIM = 2, //string, bytes, embedded messages, packed repeated fields
	GRP_START = 3, //groups (deprecated)
	GRP_END   = 4, //groups (deprecated)
	BITS_32   = 5, //fixed32, sfixed32, float
};

constexpr std::size_t MAX_VARINT_BYTES = 10;
constexpr std::size_t MAX_VARINT32_BYTES = 5;

using field_type = uint32_t;

constexpr auto field_tag(std::size_t field_number, wire_type type)
{
	return static_cast<field_type>((field_number << 3) | static_cast<uint8_t>(type));
}

using int32  = value<int32_t>;
using int64  = value<int64_t>;
using uint32 = value<uint32_t>;
using uint64 = value<uint64_t>;


} //end: namespace med::protobuf
