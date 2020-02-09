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
#include "ber_tag.hpp"
#include "ber_length.hpp"
#include "ber_info.hpp"
#include "../asn.hpp"


namespace med::asn::ber {

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
		if constexpr (is_seqof_v<IE>)
		{
			//NOTE: unrolling via field_length to process META-INFO if any
			std::size_t len = 0;
			for (auto& field : ie)
			{
				if (field.is_set()) { len += field_length(field, *this); }
			}
			return len;
		}
		else if constexpr (is_oid_v<IE>)
		{
			//NOTE: directly unrolling since no META-INFO for OID's subidentifiers
			std::size_t len = 0;
			for (auto& field : ie)
			{
				if (field.is_set())
				{
					len += detail::least_bytes_encoded(field.get());
					CODEC_TRACE("INT[%s] %zu len=%zu", name<IE>(), std::size_t(field.get()), len);
				}
			}
			return len;
		}
		else //single-field IEs
		{
			using ie_type = typename IE::ie_type;
			//CODEC_TRACE("GET-LEN [%s] ie-type=%s", name<IE>(), class_name<ie_type>());

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
					CODEC_TRACE("INT[%s] len=%zu", name<IE>(), std::size_t(val));
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
			else
			{
				static_assert(std::is_void_v<IE>, "NOT IMPLEMENTED?");
			}
		}
	}

	//IE_TAG
	template <class IE> void operator() (IE const& ie, IE_TAG)
	{
		constexpr std::size_t nbytes = bits_to_bytes(IE::traits::bits);
		uint8_t* out = ctx.buffer().template advance<IE, nbytes>();
		CODEC_TRACE("tag[%s]=%zXh %zu bytes: %s", name<IE>(), std::size_t(ie.get()), nbytes, ctx.buffer().toString());
		put_bytes<nbytes>(ie.get(), out);
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
		if constexpr (is_seqof_v<IE>)
		{
			CODEC_TRACE("SEQOF[%s] *%zu", name<IE>(), ie.count());
			sl::encode_multi(*this, ie);
		}
		else if constexpr (is_oid_v<IE>)
		{
			CODEC_TRACE("OID[%s] *%zu", name<IE>(), ie.count());
			for (auto& field : ie)
			{
				if (field.is_set())
				{
					auto const encoded = detail::encode_unsigned(field.get());
					auto const len = detail::least_bytes_encoded(field.get());
					uint8_t* out = ctx.buffer().template advance<IE>(len); //value
					put_bytes(encoded, out, len); //value
				}
				else
				{
					MED_THROW_EXCEPTION(missing_ie, name<IE>(), ie.count(), ie.count() - 1)
				}
			}
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
				CODEC_TRACE("INT[%s]=%lld %u bytes: %s", name<IE>(), (long long)ie.get_encoded(), len, ctx.buffer().toString());
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

	template <std::size_t NUM_BYTES, typename T>
	static constexpr void put_byte(uint8_t*, T const) { }

	template <std::size_t NUM_BYTES, typename T, std::size_t OFS, std::size_t... Is>
	static void put_byte(uint8_t* output, T const value)
	{
		output[OFS] = uint8_t(value >> ((NUM_BYTES - OFS - 1) << 3));
		put_byte<NUM_BYTES, T, Is...>(output, value);
	}

	template<std::size_t NUM_BYTES, typename T, std::size_t... Is>
	static void put_bytes_impl(uint8_t* output, T const value, std::index_sequence<Is...>)
	{
		put_byte<NUM_BYTES, T, Is...>(output, value);
	}

	template <std::size_t NUM_BYTES>
	static void put_bytes(std::size_t value, uint8_t* output)
	{
		put_bytes_impl<NUM_BYTES>(output, value, std::make_index_sequence<NUM_BYTES>{});
	}

	template <class IE>
	static void put_bytes(IE const& ie, uint8_t* output)
	{
		put_bytes<bits_to_bytes(IE::traits::bits)>(ie.get_encoded(), output);
	}

	template <typename T>
	static void put_bytes(T const value, uint8_t* output, uint8_t num_bytes)
	{
		switch (num_bytes)
		{
		case 1: put_bytes<1>(value, output); break;
		case 2: if constexpr (sizeof(T) >= 2) { put_bytes<2>(value, output); break; }
		case 3: if constexpr (sizeof(T) >= 3) { put_bytes<3>(value, output); break; }
		case 4: if constexpr (sizeof(T) >= 4) { put_bytes<4>(value, output); break; }
		case 5: if constexpr (sizeof(T) >= 5) { put_bytes<5>(value, output); break; }
		case 6: if constexpr (sizeof(T) >= 6) { put_bytes<6>(value, output); break; }
		case 7: if constexpr (sizeof(T) >= 7) { put_bytes<7>(value, output); break; }
		case 8: if constexpr (sizeof(T) >= 8) { put_bytes<8>(value, output); break; }
		default: MED_THROW_EXCEPTION(invalid_value, __FUNCTION__, num_bytes)
		}
	}
};

}	//end: namespace med::asn::ber
