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
#include "tag.hpp"

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
struct decoder
{
	//required for length_encoder
	using state_type = typename DEC_CTX::buffer_type::state_type;
	using size_state = typename DEC_CTX::buffer_type::size_state;
	using allocator_type = typename DEC_CTX::allocator_type;
	static constexpr std::size_t granularity = 8;

	DEC_CTX& ctx;

	explicit decoder(DEC_CTX& ctx_) : ctx{ ctx_ } { }
	allocator_type& get_allocator()                    { return ctx.get_allocator(); }

	//state
	auto operator() (PUSH_SIZE const& ps)              { return ctx.buffer().push_size(ps.size); }
	bool operator() (POP_STATE const&)                 { return ctx.buffer().pop_state(); }
	auto operator() (GET_STATE const&)                 { return ctx.buffer().get_state(); }

	//IE_NULL
	template <class IE>
	void operator() (IE&, IE_NULL const&)
	{
		using tv = tag_value<typename IE::traits, false>;
		uint8_t const* input = ctx.buffer().template advance<IE, tv::num_bytes()>();
		std::size_t vtag = 0;
		get_bytes_impl(input, vtag, std::make_index_sequence<tv::num_bytes()>{});
		CODEC_TRACE("NULL tag=%zX(%zX) bits=%zu", tv::value(), vtag, tv::num_bits());
		if (tv::value() == vtag)
		{
			if (uint8_t const length = ctx.buffer().template pop<IE>())
			{
				MED_THROW_EXCEPTION(invalid_value, name<IE>(), length, ctx.buffer());
			}
		}
		else
		{
			MED_THROW_EXCEPTION(unknown_tag, name<IE>(), vtag, ctx.buffer());
		}
	}

	//IE_VALUE
	template <class IE>
	void operator() (IE& ie, IE_VALUE const&)
	{
		using tv = tag_value<typename IE::traits, false>;
		uint8_t const* input = ctx.buffer().template advance<IE, tv::num_bytes()>();
		std::size_t vtag = 0;
		get_bytes_impl(input, vtag, std::make_index_sequence<tv::num_bytes()>{});
		CODEC_TRACE("tag=%zX(%zX) bits=%zu", tv::value(), vtag, tv::num_bits());
		if (tv::value() == vtag)
		{
			if constexpr (std::is_same_v<bool, typename IE::value_type>)
			{
				//X.690 8.2 Encoding of a boolean value
				input = ctx.buffer().template advance<IE, 1 + 1>(); //length + value
				if (input[0] == 1)
				{
					return ie.set_encoded(input[1] != 0);
				}
				MED_THROW_EXCEPTION(invalid_value, name<IE>(), input[0], ctx.buffer());
			}
			else if constexpr (std::is_integral_v<typename IE::value_type>)
			{
				//X.690 8.3 Encoding of an integer value
				//X.690 8.4 Encoding of an enumerated value
				if (uint8_t const len = ctx.buffer().template pop<IE>(); 0 < len && len < 127) //1..127 in one octet
				{
					CODEC_TRACE("\t%u octets: %s", len, ctx.buffer().toString());
					input = ctx.buffer().template advance<IE>(len); //value
					ie.set_encoded(get_bytes<typename IE::value_type>(input, len));
				}
				else
				{
					MED_THROW_EXCEPTION(invalid_value, name<IE>(), input[0], ctx.buffer());
				}
			}
			else if constexpr (std::is_floating_point_v<typename IE::value_type>)
			{
			}
		}
		else
		{
			MED_THROW_EXCEPTION(unknown_tag, name<IE>(), vtag, ctx.buffer());
		}
	}

	//IE_OCTET_STRING
	template <class IE>
	void operator() (IE& ie, IE_BIT_STRING const&)
	{
		using tv = tag_value<typename IE::traits, false>;
		uint8_t const* input = ctx.buffer().template advance<IE, tv::num_bytes()>();
		std::size_t vtag = 0;
		get_bytes_impl(input, vtag, std::make_index_sequence<tv::num_bytes()>{});
		CODEC_TRACE("tag=%zX(%zX) bits=%zu", tv::value(), vtag, tv::num_bits());
		if (tv::value() == vtag)
		{
			auto const bytes = get_length<IE>() - 1;
			input = ctx.buffer().template advance<IE, 1>(); //num of unused bits [0..7]
			std::size_t unused_bits = 0;
			get_bytes_impl(input, unused_bits, std::make_index_sequence<1>{});
			std::size_t const num_bits = bytes * 8 - unused_bits;
			CODEC_TRACE("\tSTR[%s] %zu bits: %s", name<IE>(), num_bits, ctx.buffer().toString());
			if (ie.set_encoded(num_bits, ctx.buffer().begin()))
			{
				ctx.buffer().template advance<IE>(bytes);
			}
			else
			{
				MED_THROW_EXCEPTION(invalid_value, name<IE>(), num_bits, ctx.buffer());
			}
		}
		else
		{
			MED_THROW_EXCEPTION(unknown_tag, name<IE>(), vtag, ctx.buffer());
		}
	}

	//IE_OCTET_STRING
	template <class IE>
	void operator() (IE& ie, IE_OCTET_STRING const&)
	{
		using tv = tag_value<typename IE::traits, false>;
		uint8_t const* input = ctx.buffer().template advance<IE, tv::num_bytes()>();
		std::size_t vtag = 0;
		get_bytes_impl(input, vtag, std::make_index_sequence<tv::num_bytes()>{});
		CODEC_TRACE("tag=%zX(%zX) bits=%zu", tv::value(), vtag, tv::num_bits());
		if (tv::value() == vtag)
		{
			auto const len = get_length<IE>();
			CODEC_TRACE("\tSTR[%s] %zu octets: %s", name<IE>(), std::size_t(len), ctx.buffer().toString());
			if (ie.set_encoded(len, ctx.buffer().begin()))
			{
				ctx.buffer().template advance<IE>(len);
			}
			else
			{
				MED_THROW_EXCEPTION(invalid_value, name<IE>(), len, ctx.buffer());
			}
		}
		else
		{
			MED_THROW_EXCEPTION(unknown_tag, name<IE>(), vtag, ctx.buffer());
		}
	}

#ifndef UNIT_TEST
private:
#endif

	template <class IE>
	std::size_t get_length()
	{
		uint8_t bytes = ctx.buffer().template pop<IE>();
		//short form
		if (bytes < 0x80) { return bytes; }

		bytes &= 0x7F;
		if (bytes)
		{
			return get_bytes<std::size_t>(ctx.buffer().template advance<IE>(bytes), bytes);
		}
		else //indefinite form (X.690 8.1.3.6.1)
		{
			MED_THROW_EXCEPTION(invalid_value, name<IE>(), 0x80, ctx.buffer());
		}
	}

	template <typename VALUE>
	static constexpr void get_byte(uint8_t const*, VALUE&) { }

	template <typename VALUE, std::size_t OFS, std::size_t... Is>
	static void get_byte(uint8_t const* input, VALUE& value)
	{
		value = (value << granularity) | *input;
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
		constexpr std::size_t NUM_BYTES = IE::traits::bits / granularity;
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
		case 2: return signextend<T, 16>((T(input[0]) << 8) | input[1]);
		case 3: return signextend<T, 24>((T(input[0]) << 16) | (T(input[1]) << 8) | input[2]);
		case 4: return signextend<T, 32>((T(input[0]) << 24) | (T(input[1]) << 16) | (T(input[2]) << 8) | input[3]);
		default:
			{
				T value = *input++;
				while (--num_bytes) { value <<= 8; value |= *input++; }
				return value;
			}
		}
	}
};

} //end: namespace med::asn::ber
