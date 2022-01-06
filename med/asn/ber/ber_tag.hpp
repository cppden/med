#pragma once
/**
@file
ASN.1 BER tag definition

@copyright Denis Priyomov 2018
Distributed under the MIT License
(See accompanying file LICENSE or visit https://github.com/cppden/med)
*/

#include "asn/ids.hpp"
#include "ber_length.hpp"


namespace med::asn::ber {

namespace detail {

/* X.690
8.1.2 Identifier octets
8.1.2.1 The identifier octets encode the ASN.1 tag (class and number) of the type of the data value.
8.1.2.2 For tags in [0..30], the identifier octets comprise a single octet encoded as follows:
	a) bits 8 and 7 represent the class of the tag as specified in Table 1;
	b) bit 6 represent the encoding: 0 - primitive, 1 - constructed;
	c) bits 5 to 1 encode the number of the tag as a binary integer with bit 5 as MSB.
8.1.2.4 For tags > 30, the identifier comprise a leading octet followed by 1+ subsequent octets.
8.1.2.4.1 The leading octet shall be encoded as follows:
	a) bits 8,7,6 encoded as in 8.1.2.2;
	c) bits 5 to 1 shall be encoded as 0b11111.
8.1.2.4.2 The subsequent octets encode the number of the tag as follows:
	a) bit 8 of each octet shall be set to 1 unless it is the last octet;
	b) bits 7 to 1 of the first subsequent octet, followed by bits 7 to 1 of the second subsequent
	octet, followed in turn by bits 7 to 1 of each further octet, up to and including the last
	subsequent octet in the identifier octets shall be the encoding of an unsigned binary integer
	equal to the tag number, with bit 7 of the 1st subsequent octet as the MSB;
	c) bits 7 to 1 of the first subsequent octet shall not all be zero.
*/

constexpr std::size_t TAG_MAX_IN_BYTE = 0b011111; //31


constexpr uint8_t tag_bytes(std::size_t value)
{
	if (value < TAG_MAX_IN_BYTE)
	{
		return 1;
	}
	else
	{
		return 1 + least_bytes_encoded(value);
	}
}

constexpr std::size_t encode_unsigned(std::size_t value, std::size_t init = 0)
{
	std::size_t encoded = init;

	//TODO: rewrite?
	auto const num_bits = detail::calc_least_bits(value);
	for (std::size_t v = value << (sizeof(value) * 8 - num_bits), //bits value to write out
		 bits = num_bits, //total number of bits in value to write
		 step = (bits % 7) ? (bits % 7) : 7; //number of bits in single step
		 bits;
		 bits -= step, step = 7)
	{
		encoded <<= 8;
		encoded |= ((bits > 7 ? 0x80 : 0) | uint8_t(v >> (sizeof(v) * 8 - step)));
		v <<= step;
	}

	return encoded;
}

constexpr std::size_t tag_encoded(tg_class tagclass, bool constructed, std::size_t value)
{
	constexpr std::size_t CLASS_SHIFT = 6;
	constexpr std::size_t PC_BIT = 1 << 5; //Primitive or Constructed encoding

	std::size_t tag = (uint8_t(tagclass) << CLASS_SHIFT) | (constructed ? PC_BIT : 0);
	if (value < TAG_MAX_IN_BYTE)
	{
		tag |= value;
	}
	else
	{
		tag |= TAG_MAX_IN_BYTE;
		tag = encode_unsigned(value, tag);

//		constexpr auto num_bits = detail::calc_least_bits(TRAITS::AsnTagValue);
//		for (std::size_t v = TRAITS::AsnTagValue << (sizeof(std::size_t) * 8 - num_bits), //bits value to write out
//			 bits = num_bits, //total number of bits in value to write
//			 step = (bits % 7) ? (bits % 7) : 7; //number of bits in single step
//			 bits;
//			 bits -= step, step = 7)
//		{
//			tag <<= 8;
//			tag |= ((bits > 7 ? 0x80 : 0) | uint8_t(v >> (sizeof(v) * 8 - step)));
//			v <<= step;
//		}
	}
	return tag;
}

} //end: namespace detail

template <class TRAITS, bool CONSTRUCTED>
struct tag_value
{
	static constexpr uint8_t num_bytes = detail::tag_bytes(TRAITS::AsnTagValue);
	static constexpr std::size_t value = detail::tag_encoded(TRAITS::AsnTagClass, CONSTRUCTED, TRAITS::AsnTagValue);
};

} //end: namespace med::asn::ber

