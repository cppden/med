/**
@file
Google Protobuf encoder definition

@copyright Denis Priyomov 2018
Distributed under the MIT License
(See accompanying file LICENSE or visit https://github.com/cppden/med)
*/

#pragma once

#include "debug.hpp"
#include "name.hpp"
#include "octet_string.hpp"

namespace med::protobuf {

template <class ENC_CTX>
struct encoder
{
	//required for length_encoder
	using state_type = typename ENC_CTX::buffer_type::state_type;
	static constexpr std::size_t granularity = 8;
	static constexpr auto encoder_type = codec_type::PLAIN;

	ENC_CTX& ctx;

	explicit encoder(ENC_CTX& ctx_) : ctx{ ctx_ } { }

	//state
	auto operator() (GET_STATE const&)                       { return ctx.buffer().get_state(); }
	void operator() (SET_STATE const&, state_type const& st) { ctx.buffer().set_state(st); }
	template <class IE>
	MED_RESULT operator() (SET_STATE const&, IE const& ie)
	{
		if (auto const ss = ctx.snapshot(ie))
		{
			if (ss.validate_length(get_length(ie)))
			{
				ctx.buffer().set_state(ss);
				MED_RETURN_SUCCESS;
			}
			else
			{
				return ctx.error_ctx().set_error(error::INVALID_VALUE, name<IE>(), 0);
			}
		}
		else
		{
			return ctx.error_ctx().set_error(error::MISSING_IE, name<IE>(), 0);
		}
	}

	MED_RESULT operator() (PUSH_STATE const&)           { return ctx.buffer().push_state(); }
	void operator() (POP_STATE const&)                  { ctx.buffer().pop_state(); }
	MED_RESULT operator() (ADVANCE_STATE const& ss)
	{
		if (ctx.buffer().advance(ss.bits/granularity)) MED_RETURN_SUCCESS;
		return ctx.error_ctx().set_error(error::OVERFLOW, "advance", ctx.buffer().size() * granularity, ss.bits);
	}
	MED_RESULT operator() (ADD_PADDING const& pad)
	{
		if (ctx.buffer().fill(pad.bits/granularity, pad.filler)) MED_RETURN_SUCCESS;
		return ctx.error_ctx().set_error(error::OVERFLOW, "padding", ctx.buffer().size() * granularity, pad.bits);
	}
	void operator() (SNAPSHOT const& ss)                { ctx.snapshot(ss); }


	//errors
	template <typename... ARGS>
	MED_RESULT operator() (error e, ARGS&&... args)
	{
		return ctx.error_ctx().set_error(e, std::forward<ARGS>(args)...);
	}

	//IE_VALUE
	//Little Endian Base 128: https://en.wikipedia.org/wiki/LEB128
	template <class IE>
	MED_RESULT operator() (IE const& ie, IE_VALUE const&)
	{
		static_assert(0 == (IE::traits::bits % granularity), "OCTET VALUE EXPECTED");
		auto value = ie.get_encoded();
		CODEC_TRACE("VAL[%s]=%#zx(%zu) %zu bits: %s", name<IE>(), std::size_t(value), std::size_t(value), IE::traits::bits, ctx.buffer().toString());
		//TODO: estimate exact size needed? will it be faster?
		while (value >= 0x80)
		{
			if (uint8_t* const output = ctx.buffer().template advance<1>())
			{
				*output = static_cast<uint8_t>(value | 0x80);
			}
			else
			{
				return ctx.error_ctx().set_error(error::OVERFLOW, name<IE>(), ctx.buffer().size() * granularity, IE::traits::bits);
			}
			CODEC_TRACE("\twrote %#02x, value=%#zx", uint8_t(value|0x80), std::size_t(value >> 7));
			value >>= 7;
		}
		if (uint8_t* const output = ctx.buffer().template advance<1>())
		{
			*output = static_cast<uint8_t>(value);
			CODEC_TRACE("\twrote value %02X", uint8_t(value));
			MED_RETURN_SUCCESS;
		}
		else
		{
			return ctx.error_ctx().set_error(error::OVERFLOW, name<IE>(), ctx.buffer().size() * granularity, IE::traits::bits);
		}
	}

	//IE_OCTET_STRING
	template <class IE>
	MED_RESULT operator() (IE const& ie, IE_OCTET_STRING const&)
	{
		if ( uint8_t* out = ctx.buffer().advance(ie.size()) )
		{
			octets<IE::min_octets, IE::max_octets>::copy(out, ie.data(), ie.size());
			CODEC_TRACE("STR[%s] %zu octets: %s", name<IE>(), ie.size(), ctx.buffer().toString());
			MED_RETURN_SUCCESS;
		}
		return ctx.error_ctx().set_error(error::OVERFLOW, name<IE>(), ctx.buffer().size() * granularity, ie.size() * granularity);
	}

private:
};

template <class CTX>
auto make_encoder(CTX& ctx) { return encoder<CTX>{ctx}; }

}	//end: namespace med::protobuf
