#pragma once
/**
@file
ASN.1 BER decoder definition

@copyright Denis Priyomov 2018
Distributed under the MIT License
(See accompanying file LICENSE or visit https://github.com/cppden/med)
*/

#include "debug.hpp"
#include "name.hpp"
#include "state.hpp"
#include "octet_string.hpp"
#include "asn/ber/tag.hpp"
#include "asn/ber/info.hpp"

namespace med::asn::ber {

//http://graphics.stanford.edu/~seander/bithacks.html#FixedSignExtend
template <typename T, uint8_t B>
constexpr auto signextend(T x) -> std::enable_if_t<std::is_signed_v<T>, T>
{
	struct {T x:B;} s {};
	return s.x = x;
}
template <typename T, uint8_t B>
constexpr auto signextend(T x) -> std::enable_if_t<std::is_unsigned_v<T>, T>
{
	return x;
}

template <class DEC_CTX>
struct decoder : info
{
	//required for length_encoder
	using state_type = typename DEC_CTX::buffer_type::state_type;
	using size_state = typename DEC_CTX::buffer_type::size_state;
	using allocator_type = typename DEC_CTX::allocator_type;

	DEC_CTX& ctx;

	explicit decoder(DEC_CTX& ctx_) : ctx{ ctx_ } { }
	allocator_type& get_allocator()             { return ctx.get_allocator(); }

	//state
	auto operator() (PUSH_SIZE const& ps)       { return ctx.buffer().push_size(ps.size); }
	template <class IE>
	bool operator() (PUSH_STATE, IE const&)     { return ctx.buffer().push_state(); }
	bool operator() (POP_STATE)                 { return ctx.buffer().pop_state(); }
	auto operator() (GET_STATE)                 { return ctx.buffer().get_state(); }
	template <class IE>
	bool operator() (CHECK_STATE, IE const&)    { return !ctx.buffer().empty(); }

	//IE_TAG
	template <class IE> [[nodiscard]] std::size_t operator() (IE&, IE_TAG)
	{
		//constexpr auto nbytes = IE::traits::num_bytes(); TODO: expose asn traits directly
		constexpr std::size_t nbytes = bits_to_bytes(IE::traits::bits);
		uint8_t const* input = ctx.buffer().template advance<IE, nbytes>();
		std::size_t vtag = 0;
		get_bytes_impl(input, vtag, std::make_index_sequence<nbytes>{});
		CODEC_TRACE("T=%zxh [%s] %zu bytes: %s", vtag, name<IE>(), nbytes, ctx.buffer().toString());
		return vtag;
	}
	//IE_LEN
	template <class IE> void operator() (IE& ie, IE_LEN)
	{
		CODEC_TRACE("LEN[%s]: %s", name<IE>(), ctx.buffer().toString());
		(*this)(ie, typename IE::ie_type{});
	}

	//IE_NULL
	template <class IE> void operator() (IE&, IE_NULL)
	{
		//check_tag<IE, false>();
		if (uint8_t const length = ctx.buffer().template pop<IE>())
		{
			MED_THROW_EXCEPTION(invalid_value, name<IE>(), length, ctx.buffer())
		}
	}

	//IE_VALUE
	template <class IE> void operator() (IE& ie, IE_VALUE)
	{
		//check_tag<IE, false>();
		CODEC_TRACE("V[%s]: %s", name<IE>(), ctx.buffer().toString());
		if constexpr (std::is_same_v<bool, typename IE::value_type>)
		{
			//X.690 8.2 Encoding of a boolean value
			auto* input = ctx.buffer().template advance<IE, 1 + 1>(); //length + value
			if (input[0] == 1) //length is always 1
			{
				return ie.set_encoded(input[1] != 0);
			}
			MED_THROW_EXCEPTION(invalid_value, name<IE>(), input[0], ctx.buffer())
		}
		else if constexpr (std::is_integral_v<typename IE::value_type>)
		{
			//X.690 8.3 Encoding of an integer value
			//X.690 8.4 Encoding of an enumerated value
			if (uint8_t const len = ctx.buffer().template pop<IE>(); 0 < len && len < 127) //1..127 in one octet
			{
				CODEC_TRACE("\t%u octets: %s", len, ctx.buffer().toString());
				auto* input = ctx.buffer().template advance<IE>(len); //value
				ie.set_encoded(get_bytes<typename IE::value_type>(input, len));
			}
			else
			{
				MED_THROW_EXCEPTION(invalid_value, name<IE>(), len, ctx.buffer())
			}
		}
		else if constexpr (std::is_floating_point_v<typename IE::value_type>)
		{
			MED_THROW_EXCEPTION(unknown_tag, name<IE>(), 0, ctx.buffer())
		}
	}

	//IE_BIT_STRING
	template <class IE> void operator() (IE& ie, IE_BIT_STRING)
	{
		//check_tag<IE, false>();
		auto const bytes = ber_length<IE>() - 1;
		auto* input = ctx.buffer().template advance<IE, 1>(); //num of unused bits [0..7]
		std::size_t unused_bits = 0;
		get_bytes_impl(input, unused_bits, std::make_index_sequence<1>{});
		std::size_t const num_bits = bytes * 8 - unused_bits;
		CODEC_TRACE("\tBSTR[%s] %zu bits: %s", name<IE>(), num_bits, ctx.buffer().toString());
		if (ie.set_encoded(num_bits, ctx.buffer().begin()))
		{
			ctx.buffer().template advance<IE>(bytes);
		}
		else
		{
			MED_THROW_EXCEPTION(invalid_value, name<IE>(), num_bits, ctx.buffer())
		}
	}

	//IE_OCTET_STRING
	template <class IE> void operator() (IE& ie, IE_OCTET_STRING)
	{
		//check_tag<IE, false>();
		auto const len = ber_length<IE>();
		CODEC_TRACE("\tOSTR[%s] %zu octets: %s", name<IE>(), std::size_t(len), ctx.buffer().toString());
		if (!ie.set_encoded(len, ctx.buffer().begin()))
		{
			MED_THROW_EXCEPTION(invalid_value, name<IE>(), len, ctx.buffer())
		}
		ctx.buffer().template advance<IE>(len);
	}

	template <class IE> void operator() (ENTRY_CONTAINER, IE const&)
	{
		//X.690 8.9 Encoding of a sequence value (not segmented only)
		//check_tag<IE, true>();
		CODEC_TRACE("\tSEQ[%s] %zu octets: %s", name<IE>(), ber_length<IE>(), ctx.buffer().toString());
		//CODEC_TRACE("entry [%s] len=%zu: %s", name<IE>(), len, ctx.buffer().toString());
		//TODO: need to encode length...
	}

#ifndef UNIT_TEST
private:
#endif

	template <class IE>
	std::size_t ber_length()
	{
		uint8_t bytes = ctx.buffer().template pop<IE>();
		//short form
		if (bytes < 0x80) { return bytes; }

		bytes &= 0x7F;
		if (bytes)
		{
			return get_bytes<std::size_t>(ctx.buffer().template advance<IE>(bytes), bytes);
		}
		else //indefinite form (X.690 8.1.3.6)
		//8.1.3.6 length octets indicate that the contents octets are terminated by end-of-contents octets
		//(two zero octets), and shall consist of a single octet.
		{
			MED_THROW_EXCEPTION(invalid_value, name<IE>(), 0x80, ctx.buffer())
		}
	}

	template <typename VALUE>
	static constexpr void get_byte(uint8_t const*, VALUE&) { }

	template <typename VALUE, std::size_t OFS, std::size_t... Is>
	static void get_byte(uint8_t const* input, VALUE& value)
	{
		value = (value << 8) | *input;
		get_byte<VALUE, Is...>(++input, value);
	}

	template<typename VALUE, std::size_t... Is>
	static void get_bytes_impl(uint8_t const* input, VALUE& value, std::index_sequence<Is...>)
	{
		get_byte<VALUE, Is...>(input, value);
	}

	template <class IE>
	static auto get_bytes(uint8_t const* input)
	{
		constexpr std::size_t NUM_BYTES = bits_to_bytes(IE::traits::bits);
		typename IE::value_type value{};
		get_bytes_impl(input, value, std::make_index_sequence<NUM_BYTES>{});
		return value;
	}

	template <typename T>
	static T get_bytes(uint8_t const* input, uint8_t num_bytes)
	{
		//TODO: template to unwrap w/o duck-typing and allow char/short/int/long...
		switch (num_bytes)
		{
		case 1: return signextend<T, 8>(input[0]);
		case 2: return signextend<T, 16>((T(input[0]) <<  8) | (T(input[1])) );
		case 3: return signextend<T, 24>((T(input[0]) << 16) | (T(input[1]) <<  8) | (T(input[2])) );
		case 4: return signextend<T, 32>((T(input[0]) << 24) | (T(input[1]) << 16) | (T(input[2]) << 8) | (T(input[3])) );
//		case 5: return signextend<T, 40>((T(input[0]) << 32) | (T(input[1]) << 24) | (T(input[2]) << 16) | (T(input[3])<< 8) | (T(input[4])) );
//		case 6: return signextend<T, 48>((T(input[0]) << 40) | (T(input[1]) << 32) | (T(input[2]) << 24) | (T(input[3])<<16) | (T(input[4])<< 8) | (T(input[5])) );
//		case 7: return signextend<T, 56>((T(input[0]) << 48) | (T(input[1]) << 40) | (T(input[2]) << 32) | (T(input[3])<<24) | (T(input[4])<<16) | (T(input[5])<< 8) | (T(input[6])) );
//		case 8: return signextend<T, 64>((T(input[0]) << 56) | (T(input[1]) << 48) | (T(input[2]) << 40) | (T(input[3])<<32) | (T(input[4])<<24) | (T(input[5])<<16) | (T(input[6])<<8) | (T(input[7])));
		default:
			{
				std::size_t value = *input++;
				while (--num_bytes) { value <<= 8; value |= *input++; }
				return value;
			}
		}
	}

	template <class IE, bool CONSTRUCTED>
	void check_tag()
	{
		using tv = tag_value<typename IE::traits, CONSTRUCTED>;
		uint8_t const* input = ctx.buffer().template advance<IE, tv::num_bytes()>();
		std::size_t vtag = 0;
		get_bytes_impl(input, vtag, std::make_index_sequence<tv::num_bytes()>{});
		if (tv::value() != vtag)
		{
			CODEC_TRACE("tag[%s] exp=%zX got=%zu %zu bytes", name<IE>(), tv::value(), vtag, tv::num_bytes());
			MED_THROW_EXCEPTION(unknown_tag, name<IE>(), vtag, ctx.buffer())
		}
		CODEC_TRACE("tag[%s]=%zX %zu bytes", name<IE>(), tv::value(), tv::num_bytes());
	}
};

} //end: namespace med::asn::ber
