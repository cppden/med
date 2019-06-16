#pragma once
/**
@file
ASN.1 BER encoder definition

@copyright Denis Priyomov 2018
Distributed under the MIT License
(See accompanying file LICENSE or visit https://github.com/cppden/med)
*/

#include "debug.hpp"
#include "name.hpp"
#include "state.hpp"
#include "tag.hpp"
#include "length.hpp"
#include "octet_string.hpp"


namespace med::asn::ber {

template <class ENC_CTX>
struct encoder
{
	//required for length_encoder
	using state_type = typename ENC_CTX::buffer_type::state_type;
	static constexpr std::size_t granularity = 8;

	ENC_CTX& ctx;

	explicit encoder(ENC_CTX& ctx_) : ctx{ ctx_ } { }

	//state
	auto operator() (GET_STATE)                       { return ctx.buffer().get_state(); }
	void operator() (SET_STATE, state_type const& st) { ctx.buffer().set_state(st); }
	template <class IE>
	void operator() (SET_STATE, IE const& ie)
	{
		if (auto const ss = ctx.snapshot(ie))
		{
			if (ss.validate_length(get_length(ie)))
			{
				ctx.buffer().set_state(ss);
			}
			else
			{
				MED_THROW_EXCEPTION(invalid_value, name<IE>(), get_length(ie), ctx.buffer());
			}
		}
		else
		{
			MED_THROW_EXCEPTION(missing_ie, name<IE>(), 1, 0, ctx.buffer());
		}
	}

	template <class IE>
	bool operator() (PUSH_STATE, IE const&)             { return ctx.buffer().push_state(); }
	void operator() (POP_STATE)                         { ctx.buffer().pop_state(); }
	void operator() (ADVANCE_STATE const& ss)           { ctx.buffer().template advance<ADVANCE_STATE>(ss.bits/granularity); }
	void operator() (SNAPSHOT const& ss)                { ctx.snapshot(ss); }

	//IE_NULL
	template <class IE>
	void operator() (IE const&, IE_NULL)
	{
		using tv = tag_value<typename IE::traits, false>;
		uint8_t* out = ctx.buffer().template advance<IE, tv::num_bytes()>();
		put_bytes_impl<tv::num_bytes()>(out, tv::value(), std::make_index_sequence<tv::num_bytes()>{});
		//X.690 8.8 Encoding of a null value
		//8.8.2 The contents octets shall not contain any octets.
		//NOTE – The length octet is zero.
		ctx.buffer().template push<IE>(0); //length + no value
	}

	//IE_VALUE
	template <class IE>
	void operator() (IE const& ie, IE_VALUE)
	{
		//static_assert(0 == (IE::traits::bits % granularity), "OCTET VALUE EXPECTED");
		using tv = tag_value<typename IE::traits, false>;
		CODEC_TRACE("VAL[%s]=%#zx %zu bits", name<IE>(), std::size_t(ie.get_encoded()), IE::traits::bits);
		//CODEC_TRACE("tag=%zX bits=%zu", tv::value(), tv::tag_num_bits());
		uint8_t* out = ctx.buffer().template advance<IE, tv::num_bytes()>();

		put_bytes_impl<tv::num_bytes()>(out, tv::value(), std::make_index_sequence<tv::num_bytes()>{});
		if constexpr (std::is_same_v<bool, typename IE::value_type>)
		{
			//X.690 8.2 Encoding of a boolean value
			uint8_t* out = ctx.buffer().template advance<IE, 1 + 1>(); //length + value
			out[0] = 1; //length
			out[1] = ie.get_encoded() ? 0xFF : 0x00; //value
		}
		else if constexpr (std::is_integral_v<typename IE::value_type>)
		{
			//X.690 8.3 Encoding of an integer value
			//X.690 8.4 Encoding of an enumerated value
			auto const len = length::bytes<typename IE::value_type>(ie.get_encoded());
			uint8_t* out = ctx.buffer().template advance<IE>(1 + len); //length + value
			*out++ = len; //length in 1 byte, no sense in more than 9 (17 in future?) bytes for integer
			put_bytes(ie.get_encoded(), out, len); //value
			CODEC_TRACE("\t%u octets for %d: %s", len, ie.get_encoded(), ctx.buffer().toString());
		}
		else if constexpr (std::is_floating_point_v<typename IE::value_type>)
		{
			//TODO: implement
			MED_THROW_EXCEPTION(unknown_tag, name<IE>(), 0, ctx.buffer());
		}
		else
		{
			MED_THROW_EXCEPTION(unknown_tag, name<IE>(), 0, ctx.buffer());
		}
	}

	//IE_BIT_STRING
	template <class IE>
	void operator() (IE const& ie, IE_BIT_STRING)
	{
		//X.690 8.6 Encoding of a bitstring value (not segmented only)
		using tv = tag_value<typename IE::traits, false>;
		uint8_t* out = ctx.buffer().template advance<IE, tv::num_bytes()>();
		put_bytes_impl<tv::num_bytes()>(out, tv::value(), std::make_index_sequence<tv::num_bytes()>{});

		//8.6.2.2 The initial octet shall encode, as an unsigned binary integer,
		// the number of unused bits in the final subsequent octet in the range [0..7].
		//8.6.2.3 If the bitstring is empty, there shall be no subsequent octets, and the initial
		// octet shall be zero.
		put_length<IE>(ie.size() + 1);
		ctx.buffer().template push<IE>( uint8_t(8 - uint8_t(ie.get().least_bits())) );
		out = ctx.buffer().template advance<IE>(ie.size());
		octets<IE::traits::min_bits*8, IE::traits::max_bits*8>::copy(out, ie.data(), ie.size());
		CODEC_TRACE("STR[%s] %zu bits: %s", name<IE>(), std::size_t(ie.get().num_of_bits()), ctx.buffer().toString());
	}

	//IE_OCTET_STRING
	template <class IE>
	void operator() (IE const& ie, IE_OCTET_STRING)
	{
		//X.690 8.7 Encoding of an octetstring value (not segmented only)
		using tv = tag_value<typename IE::traits, false>;
		uint8_t* out = ctx.buffer().template advance<IE, tv::num_bytes()>();
		put_bytes_impl<tv::num_bytes()>(out, tv::value(), std::make_index_sequence<tv::num_bytes()>{});

		put_length<IE>(ie.size());
		out = ctx.buffer().template advance<IE>(ie.size());
		octets<IE::traits::min_octets, IE::traits::max_octets>::copy(out, ie.data(), ie.size());
		CODEC_TRACE("STR[%s] %zu octets: %s", name<IE>(), ie.size(), ctx.buffer().toString());
	}

	template <class IE>
	void operator() (ENTRY_CONTAINER, IE const&)
	{
		CODEC_TRACE("entry [%s]: %s", name<IE>(), ctx.buffer().toString());
		//X.690 8.9 Encoding of a sequence value (not segmented only)
		using tv = tag_value<typename IE::traits, true>;
		uint8_t* out = ctx.buffer().template advance<IE, tv::num_bytes()>();
		put_bytes_impl<tv::num_bytes()>(out, tv::value(), std::make_index_sequence<tv::num_bytes()>{});
		//TODO: need to encode length...
	}

#ifndef UNIT_TEST
private:
#endif

	template <class IE>
	void put_length(std::size_t len)
	{
		// X.690
		// 8.1.3.3 in definite form length octets consist of 1+ octets, in short (8.1.3.4) or long form (8.1.3.5).
		if (len < 0x80)
		{
			// 8.1.3.4 short form can only be used when length <= 127.
			ctx.buffer().template push<IE>(len);
		}
		else
		{
			// 8.1.3.5 In the long form an initial octet is encoded as follows:
			// a) bit 8 shall be one;
			// b) bits 7 to 1 encode the number of subsequent octets;
			// c) the value 0b11111111 shall not be used.
			// Subsequent octets encode as unsigned binary integer equal to the the length value.
			uint8_t const bytes = length::bytes(len);
			uint8_t* out = ctx.buffer().template advance<IE>(1 + bytes);
			*out++ = bytes | 0x80;
			put_bytes(len, out, bytes);
		}
	}

	template <std::size_t NUM_BYTES, typename VALUE>
	static constexpr void put_byte(uint8_t*, VALUE const) { }

	template <std::size_t NUM_BYTES, typename VALUE, std::size_t OFS, std::size_t... Is>
	static void put_byte(uint8_t* output, VALUE const value)
	{
		output[OFS] = static_cast<uint8_t>(value >> ((NUM_BYTES - OFS - 1) << 3));
		put_byte<NUM_BYTES, VALUE, Is...>(output, value);
	}

	template<std::size_t NUM_BYTES, typename VALUE, std::size_t... Is>
	static void put_bytes_impl(uint8_t* output, VALUE const value, std::index_sequence<Is...>)
	{
		put_byte<NUM_BYTES, VALUE, Is...>(output, value);
	}

	template <class IE>
	static void put_bytes(IE const& ie, uint8_t* output)
	{
		constexpr std::size_t NUM_BYTES = IE::traits::bits / granularity;
		put_bytes_impl<NUM_BYTES>(output, ie.get_encoded(), std::make_index_sequence<NUM_BYTES>{});
	}

	template <typename T>
	static void put_bytes(T value, uint8_t* output, uint8_t num_bytes)
	{
		switch (num_bytes)
		{
		case 1:
			output[0] = static_cast<uint8_t>(value);
			break;
		case 2:
			output[0] = static_cast<uint8_t>(value >> 8);
			output[1] = static_cast<uint8_t>(value);
			break;
		case 3:
			output[0] = static_cast<uint8_t>(value >> 16);
			output[1] = static_cast<uint8_t>(value >> 8);
			output[2] = static_cast<uint8_t>(value);
			break;
		case 4:
			output[0] = static_cast<uint8_t>(value >> 24);
			output[1] = static_cast<uint8_t>(value >> 16);
			output[2] = static_cast<uint8_t>(value >> 8);
			output[3] = static_cast<uint8_t>(value);
			break;
			//TODO: template to support char/short/int/long...
		default:
			for (int s = (num_bytes - 1) * 8; s >= 0; s -= 8)
			{
				*output++ = static_cast<uint8_t>(value >> s);
			}
			break;
		}
	}
};

}	//end: namespace med::asn::ber
