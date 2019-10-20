/**
@file
octet encoder definition

@copyright Denis Priyomov 2016-2017
Distributed under the MIT License
(See accompanying file LICENSE or visit https://github.com/cppden/med)
*/

#pragma once

#include "name.hpp"
#include "state.hpp"
#include "length.hpp"
#include "octet_string.hpp"
#include "sl/octet_info.hpp"

namespace med {

template <class ENC_CTX>
struct octet_encoder : sl::octet_info
{
	//required for length_encoder
	using state_type = typename ENC_CTX::buffer_type::state_type;

	ENC_CTX& ctx;

	explicit octet_encoder(ENC_CTX& ctx_) : ctx{ ctx_ } { }

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
	bool operator() (PUSH_STATE, IE const&)           { return ctx.buffer().push_state(); }
	void operator() (POP_STATE)                       { ctx.buffer().pop_state(); }
	void operator() (ADVANCE_STATE const& ss)         { ctx.buffer().template advance<ADVANCE_STATE>(ss.delta); }
	void operator() (ADD_PADDING const& pad)          { ctx.buffer().template fill<ADD_PADDING>(pad.pad_size, pad.filler); }
	void operator() (SNAPSHOT const& ss)              { ctx.snapshot(ss); }

	template <class IE> constexpr std::size_t operator() (GET_LENGTH, IE const& ie)
	{
		if constexpr (detail::has_size<IE>::value)
		{
			CODEC_TRACE("length(%s) = %zu", name<IE>(), std::size_t(ie.size()));
			return ie.size();
		}
		else
		{
			std::size_t const len = bits_to_bytes(IE::traits::bits);
			CODEC_TRACE("length(%s) = %zu", name<IE>(), len);
			return len;
		}
	}

	//IE_TAG/IE_LEN
	template <class IE> void operator() (IE const& ie, IE_TAG)
		{ (*this)(ie, typename IE::ie_type{}); }
	template <class IE> void operator() (IE const& ie, IE_LEN)
		{ (*this)(ie, typename IE::ie_type{}); }

	//IE_NULL
	template <class IE> void operator() (IE const&, IE_NULL)
		{ CODEC_TRACE("NULL[%s]: %s", name<IE>(), ctx.buffer().toString()); }

	//IE_VALUE
	template <class IE> void operator() (IE const& ie, IE_VALUE)
	{
		static_assert(0 == (IE::traits::bits % 8), "OCTET VALUE EXPECTED");
		CODEC_TRACE("V=%#zxh %zu bits[%s]: %s", std::size_t(ie.get_encoded()), IE::traits::bits, name<IE>(), ctx.buffer().toString());
		uint8_t* out = ctx.buffer().template advance<IE, bits_to_bytes(IE::traits::bits)>();
		put_bytes(ie, out);
	}

	//IE_OCTET_STRING
	template <class IE> void operator() (IE const& ie, IE_OCTET_STRING)
	{
		uint8_t* out = ctx.buffer().template advance<IE>(ie.size());
		octets<IE::traits::min_octets, IE::traits::max_octets>::copy(out, ie.data(), ie.size());
		CODEC_TRACE("STR[%s] %zu octets: %s", name<IE>(), ie.size(), ctx.buffer().toString());
	}

private:
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
};

template <class C> explicit octet_encoder(C& ctx) -> octet_encoder<C>;
}	//end: namespace med
