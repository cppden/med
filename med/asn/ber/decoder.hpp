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

	//errors
	template <typename... ARGS>
	MED_RESULT operator() (error e, ARGS&&... args)
	{
		return ctx.error_ctx().set_error(ctx.buffer(), e, std::forward<ARGS>(args)...);
	}

	//IE_VALUE
	template <class IE>
	MED_RESULT operator() (IE& ie, IE_VALUE const&)
	{
		using tv = tag_value<typename IE::traits, false>;
		if (uint8_t const* input = ctx.buffer().template advance<tv::num_bytes()>())
		{
			std::size_t vtag = 0;
			get_bytes_impl(input, vtag, std::make_index_sequence<tv::num_bytes()>{});
			CODEC_TRACE("tag=%zX(%zX) bits=%zu", tv::value(), vtag, tv::num_bits());
			if (tv::value() == vtag)
			{
				if constexpr (std::is_same_v<bool, typename IE::value_type>)
				{
					if (uint8_t const* input = ctx.buffer().template advance<1 + 1>()) //length + value
					{
						if (input[0] == 1)
						{
							ie.set_encoded(input[1] != 0);
							MED_RETURN_SUCCESS;
						}
						MED_RETURN_ERROR(error::INVALID_VALUE, (*this), name<IE>(), input[0]);
					}
					MED_RETURN_ERROR(error::OVERFLOW, (*this), name<IE>(), IE::traits::bits/granularity);
				}
				else if constexpr (std::is_integral_v<typename IE::value_type>)
				{
					//TODO: drop support of NO-EXCEPTION to avoid such macoronni
					if (uint8_t const* input = ctx.buffer().template advance<1>()) //length
					{
						if (uint8_t const len = input[0]; 0 < len && len < 127) //1..127 in one octet
						{
							CODEC_TRACE("\t%u octets: %s", len, ctx.buffer().toString());
							if (uint8_t const* input = ctx.buffer().advance(len)) //value
							{
								ie.set_encoded(get_bytes<typename IE::value_type>(input, len));
								MED_RETURN_SUCCESS;
							}
						}
						MED_RETURN_ERROR(error::INVALID_VALUE, (*this), name<IE>(), input[0]);
					}
					MED_RETURN_ERROR(error::OVERFLOW, (*this), name<IE>(), IE::traits::bits/granularity);
				}
				else if constexpr (std::is_floating_point_v<typename IE::value_type>)
				{
				}
			}
			MED_RETURN_ERROR(error::UNKNOWN_TAG, (*this), name<IE>(), vtag);
		}
		else
		{
			MED_RETURN_ERROR(error::OVERFLOW, (*this), name<IE>(), tv::num_bytes());
		}
	}

	//IE_OCTET_STRING
	template <class IE>
	MED_RESULT operator() (IE& ie, IE_OCTET_STRING const&)
	{
		//CODEC_TRACE("->STR[%s]: %s", name<IE>(), ctx.buffer().toString());
		MED_RETURN_ERROR(error::INVALID_VALUE, (*this), name<IE>(), ie.size(), ctx.buffer().get_offset());
	}

private:
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

	std::size_t get_length()
	{
		if (uint8_t const* input = ctx.buffer().template advance<1>())
		{
			auto const bytes = *input;
			if (bytes < 0x80) //short form
			{
				return bytes;
			}
			else if (uint8_t const* input = ctx.buffer().advance(bytes & 0x7F))
			{
				return get_bytes<std::size_t>(input, bytes);
			}
		}
#ifdef MED_EXCEPTIONS
//#error clean this up
#endif
		return 0;
	}

};

} //end: namespace med::asn::ber