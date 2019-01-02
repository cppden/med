#pragma once
/**
@file
ASN.1 BER tag definition

@copyright Denis Priyomov 2018
Distributed under the MIT License
(See accompanying file LICENSE or visit https://github.com/cppden/med)
*/

#include "../ids.hpp"


namespace med::asn::ber {


constexpr std::size_t TAG_INVALID = std::size_t(~0);

/* X.690
8.1.2 Identifier octets
8.1.2.1 The identifier octets shall encode the ASN.1 tag (class and number) of the type of the data value.
8.1.2.2 For tags with a number ranging from zero to 30 (inclusive), the identifier octets shall comprise
	a single octet encoded as follows:
	a) bits 8 and 7 shall be encoded to represent the class of the tag as specified in Table 1;
	b) bit 6 shall be a zero or a one according to the rules of 8.1.2.5;
	c) bits 5 to 1 shall encode the number of the tag as a binary integer with bit 5 as the most significant bit.
8.1.2.4 For tags with a number greater than or equal to 31, the identifier shall comprise a leading
	octet followed by one or more subsequent octets.
8.1.2.4.1 The leading octet shall be encoded as follows:
	a) bits 8 and 7 shall be encoded to represent the class of the tag as listed in Table 1;
	b) bit 6 shall be a zero or a one according to the rules of 8.1.2.5;
	c) bits 5 to 1 shall be encoded as 11111 2 .
8.1.2.4.2 The subsequent octets shall encode the number of the tag as follows:
	a) bit 8 of each octet shall be set to one unless it is the last octet of the identifier octets;
	b) bits 7 to 1 of the first subsequent octet, followed by bits 7 to 1 of the second subsequent
	octet, followed in turn by bits 7 to 1 of each further octet, up to and including the last
	subsequent octet in the identifier octets shall be the encoding of an unsigned binary integer
	equal to the tag number, with bit 7 of the first subsequent octet as the most significant bit;
	c) bits 7 to 1 of the first subsequent octet shall not all be zero.
*/

template <class TRAITS, bool CONSTRUCTED>
struct tag_value
{
	static constexpr std::size_t CLASS_SHIFT = 6;
	static constexpr std::size_t PC_BIT = 1 << 5; //Primitive or Constructed encoding
	static constexpr std::size_t MAX_IN_BYTE = 0b011111; //31

	static constexpr std::size_t num_bits()
	{
		if constexpr (TRAITS::asn_tag_value < MAX_IN_BYTE)
		{
			return 8;
		}
		else
		{
			return sizeof(long long) * 8 - __builtin_clzll(TRAITS::asn_tag_value);
		}
	}

	static constexpr std::size_t num_bytes()
	{
		if constexpr (TRAITS::asn_tag_value < MAX_IN_BYTE)
		{
			return 1;
		}
		else
		{
			return 1 + (num_bits() + 6) / 7;
		}
	}

	static constexpr std::size_t value()
	{
		std::size_t tag = uint8_t(TRAITS::asn_tag_class) << CLASS_SHIFT;
		if constexpr (CONSTRUCTED) { tag |= PC_BIT; }
		if constexpr (TRAITS::asn_tag_value < MAX_IN_BYTE)
		{
			tag |= TRAITS::asn_tag_value;
		}
		else
		{
			tag |= MAX_IN_BYTE;

			for (std::size_t v = TRAITS::asn_tag_value << (sizeof(std::size_t) * 8 - num_bits()), //bits value to write out
				 bits = num_bits(), //total number of bits in value to write
				 step = (bits % 7) ? (bits % 7) : 7; //number of bits in single step
				 bits;
				 bits -= step, step = 7)
			{
				tag <<= 8;
				tag |= ((bits > 7 ? 0x80 : 0) | uint8_t(v >> (sizeof(v) * 8 - step)));
				v <<= step;
			}
		}
		return tag;
	}
};

} //end: namespace med::asn::ber

