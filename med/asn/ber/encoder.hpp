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
#include "octet_string.hpp"
#include "asn/ber/tag.hpp"
#include "asn/ber/length.hpp"
#include "asn/ber/info.hpp"


namespace med::asn::ber {

/*
template <class META_INFO, class ENCODER, class IE>
inline void ie_encode(ENCODER& encoder, IE const& ie)
{
	if constexpr (not meta::list_is_empty_v<META_INFO>)
	{
		using mi = meta::list_first_t<META_INFO>;
		using mi_rest = meta::list_rest_t<META_INFO>;
		CODEC_TRACE("%s[%s]: %s", __FUNCTION__, name<IE>(), class_name<mi>());

		if constexpr (mi::kind == mik::TAG)
		{
			encode_tag<mi>(encoder);
		}
		else if constexpr (mi::kind == mik::LEN)
		{
			//CODEC_TRACE("LV=? [%s]", name<IE>());
			auto const len = sl::ie_length<mi_rest>(ie, encoder);
			//CODEC_TRACE("LV=%zxh [%s]", len, name<IE>());
			encode_len<typename mi::length_type>(encoder, len);
		}

		ie_encode<mi_rest>(encoder, ie);
	}
	else
	{
		using ie_type = typename IE::ie_type;
		CODEC_TRACE("%s[%.30s]: %s", __FUNCTION__, class_name<IE>(), class_name<ie_type>());
		if constexpr (std::is_base_of_v<CONTAINER, ie_type>)
		{
			call_if<is_callable_with_v<ENCODER, ENTRY_CONTAINER>>::call(encoder, ENTRY_CONTAINER{}, ie);
			container_encoder<ENCODER>::encode(encoder, ie);
			call_if<is_callable_with_v<ENCODER, EXIT_CONTAINER>>::call(encoder, EXIT_CONTAINER{}, ie);
		}
		else
		{
			snapshot(encoder, ie);
			encoder(ie, ie_type{});
		}
	}
}
*/

template <class FUNC, class IE>
inline void encode_multi(FUNC& func, IE const& ie)
{
	using field_type = typename IE::field_type;
	//using mi = meta::produce_info_t<FUNC, IE>; //assuming MI of multi_field == MI of field
	using field_mi = meta::produce_info_t<FUNC, field_type>;
//	sl::ie_encode<mi>(*this, ie);

	CODEC_TRACE("%s *%zu", name<IE>(), ie.count());
	for (auto& field : ie)
	{
		if (field.is_set())
		{
			sl::ie_encode<field_mi>(func, field);
		}
		else
		{
			MED_THROW_EXCEPTION(missing_ie, name<field_type>(), ie.count(), ie.count() - 1)
		}
	}
}


template <class ENC_CTX>
struct encoder : info
{
	//required for length_encoder
	using state_type = typename ENC_CTX::buffer_type::state_type;

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
			auto const len = field_length(ie, *this);
			if (ss.validate_length(len))
			{
				ctx.buffer().set_state(ss);
			}
			else
			{
				MED_THROW_EXCEPTION(invalid_value, name<IE>(), len, ctx.buffer())
			}
		}
		else
		{
			MED_THROW_EXCEPTION(missing_ie, name<IE>(), 1, 0, ctx.buffer())
		}
	}

	template <class IE>
	bool operator() (PUSH_STATE, IE const&)             { return ctx.buffer().push_state(); }
	void operator() (POP_STATE)                         { ctx.buffer().pop_state(); }
	void operator() (ADVANCE_STATE const& ss)           { ctx.buffer().template advance<ADVANCE_STATE>(ss.delta); }
	void operator() (SNAPSHOT const& ss)                { ctx.snapshot(ss); }

	//calculate length of IE (can be either a data itself or TAG/LEN already extracted from META-INFO
	template <class IE>
	constexpr std::size_t operator() (GET_LENGTH, IE const& ie)
	{
		using ie_type = typename IE::ie_type;
		if constexpr (std::is_base_of_v<IE_NULL, ie_type>)
		{
			constexpr std::size_t val_size = 0;
			CODEC_TRACE("NULL[%s] len=%zu", name<IE>(), val_size);
			return val_size;
		}
		else if constexpr (std::is_base_of_v<IE_VALUE, ie_type>)
		{
			using value_type = typename IE::value_type;
			if constexpr (std::is_same_v<bool, value_type>)
			{
				constexpr std::size_t val_size = 1;
				CODEC_TRACE("BOOL[%s] len=%zu", name<IE>(), val_size);
				return val_size;
			}
			else if constexpr (std::is_integral_v<value_type>)
			{
#if defined(__GNUC__)
				//optimizer bug in GCC :( - shows up only when ie.get_encoded() = 0
				auto const val = ie.get_encoded();
				return val ? length::bytes<value_type>(val) : 1;
#else
				auto const val_size = length::bytes<value_type>(ie.get_encoded());
				CODEC_TRACE("INT[%s] len=%u for %zX", name<IE>(), val_size, std::size_t(ie.get_encoded()));
				return val_size;
#endif
			}
			else if constexpr (std::is_floating_point_v<value_type>)
			{
				MED_THROW_EXCEPTION(unknown_tag, name<IE>(), 0, ctx.buffer())
			}
			else
			{
				static_assert(std::is_void_v<value_type>, "NOT IMPLEMENTED?");
			}
		}
		else if constexpr (std::is_base_of_v<IE_OCTET_STRING, ie_type>)
		{
			std::size_t const val_size = ie.size();
			//std::size_t const len_size = length::bytes(val_size);
			CODEC_TRACE("OSTR[%s] len=%zu", name<IE>(), val_size);
			return val_size;
		}
		else if constexpr (std::is_base_of_v<IE_BIT_STRING, ie_type>)
		{
			std::size_t const val_size = ie.size() + 1;
			CODEC_TRACE("BSTR[%s] len=%zu", name<IE>(), val_size);
			return val_size;
		}
	}

	//IE_TAG
	template <class IE> void operator() (IE const& ie, IE_TAG)
	{
		//constexpr auto nbytes = IE::traits::num_bytes(); TODO: expose asn traits directly
		constexpr std::size_t nbytes = bits_to_bytes(IE::traits::bits);
		uint8_t* out = ctx.buffer().template advance<IE, nbytes>();
		CODEC_TRACE("tag[%s]=%zXh %zu bytes: %s", name<IE>(), std::size_t(ie.get()), nbytes, ctx.buffer().toString());
		put_bytes_impl<nbytes>(out, ie.get(), std::make_index_sequence<nbytes>{});
	}

	//IE_LEN
	template <class IE> void operator() (IE const& ie, IE_LEN)
	{
		CODEC_TRACE("len[%s]=%zXh: %s", name<IE>(), std::size_t(ie.get()), ctx.buffer().toString());
		ber_length<IE>(ie.get());
	}


	//IE_NULL
	template <class IE> constexpr void operator() (IE const&, IE_NULL) const
	{
		//X.690 8.8 Encoding of a null value
		//8.8.2 The contents octets shall not contain any octets.
		//NOTE – The length octet is zero.
		//length(encoded via IE_LEN) + no value
	}

	//IE_VALUE
	template <class IE> void operator() (IE const& ie, IE_VALUE)
	{
		//TODO: normally this is handled in sequence/set but ASN.1 has seq-of/set-of :(
		if constexpr (is_multi_field_v<IE>)
		{
			encode_multi(*this, ie);
		}
		else
		{
			using value_type = typename IE::value_type;
			if constexpr (std::is_same_v<bool, value_type>)
			{
				//X.690 8.2 Encoding of a boolean value
				ctx.buffer().template push<IE>(ie.get_encoded() ? 0xFF : 0x00);
				CODEC_TRACE("BOOL[%s]=%zxh: %s", name<IE>(), std::size_t(ie.get_encoded()), ctx.buffer().toString());
			}
			else if constexpr (std::is_integral_v<value_type>)
			{
				//X.690 8.3 Encoding of an integer value
				//X.690 8.4 Encoding of an enumerated value
				auto const len = length::bytes<value_type>(ie.get_encoded());
				uint8_t* out = ctx.buffer().template advance<IE>(len); //value
				//*out++ = len; //length in 1 byte, no sense in more than 9 (17 in future?) bytes for integer
				put_bytes(ie.get_encoded(), out, len); //value
				CODEC_TRACE("INT[%s]=%d %u bytes: %s", name<IE>(), ie.get_encoded(), len, ctx.buffer().toString());
			}
			else if constexpr (std::is_floating_point_v<value_type>)
			{
				//TODO: implement
				MED_THROW_EXCEPTION(unknown_tag, name<IE>(), 0, ctx.buffer())
			}
			else
			{
				static_assert(std::is_void_v<value_type>, "NOT IMPLEMENTED?");
			}
		}
	}

	//IE_BIT_STRING
	template <class IE> void operator() (IE const& ie, IE_BIT_STRING)
	{
		//X.690 8.6 Encoding of a bitstring value (not segmented only)
		//8.6.2.2 The initial octet shall encode, as an unsigned binary integer,
		// the number of unused bits in the final subsequent octet in the range [0..7].
		//8.6.2.3 If the bitstring is empty, there shall be no subsequent octets, and the initial
		// octet shall be zero.
		ctx.buffer().template push<IE>( uint8_t(8 - uint8_t(ie.get().least_bits())) );
		auto* out = ctx.buffer().template advance<IE>(ie.size());
		octets<IE::traits::min_bits/8, IE::traits::max_bits/8>::copy(out, ie.data(), ie.size());
		CODEC_TRACE("STR[%s] %zu bits: %s", name<IE>(), std::size_t(ie.get().num_of_bits()), ctx.buffer().toString());
	}

	//IE_OCTET_STRING
	template <class IE> void operator() (IE const& ie, IE_OCTET_STRING)
	{
		//X.690 8.7 Encoding of an octetstring value (not segmented only)
		auto* out = ctx.buffer().template advance<IE>(ie.size());
		octets<IE::traits::min_octets, IE::traits::max_octets>::copy(out, ie.data(), ie.size());
		CODEC_TRACE("STR[%s] %zu octets: %s", name<IE>(), ie.size(), ctx.buffer().toString());
	}

#ifndef UNIT_TEST
private:
#endif

	template <class IE>
	void ber_length(std::size_t len)
	{
		// X.690
		// 8.1.3.3 in definite form length octets consist of 1+ octets,
		// in short (8.1.3.4) or long form (8.1.3.5).
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
		constexpr std::size_t NUM_BYTES = bits_to_bytes(IE::traits::bits);
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
