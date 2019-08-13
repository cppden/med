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
#include "state.hpp"
#include "octet_string.hpp"
#include "sl/octet_info.hpp"

namespace med::protobuf {

template <class ENC_CTX>
struct encoder : sl::octet_info
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
	bool operator() (PUSH_STATE, IE const&)           { return ctx.buffer().push_state(); }
	void operator() (POP_STATE)                       { ctx.buffer().pop_state(); }
	void operator() (ADVANCE_STATE const& ss)         { ctx.buffer().template advance<ADVANCE_STATE>(ss.delta); }
	void operator() (SNAPSHOT const& ss)              { ctx.snapshot(ss); }

	//IE_TAG/IE_LEN
	template <class IE> void operator() (IE const& ie, IE_TAG)
		{ (*this)(ie, typename IE::ie_type{}); }

	//IE_VALUE
	//Little Endian Base 128: https://en.wikipedia.org/wiki/LEB128
	template <class IE>
	void operator() (IE const& ie, IE_VALUE)
	{
		static_assert(0 == (IE::traits::bits % 8), "OCTET VALUE EXPECTED");
		auto value = ie.get_encoded();
		CODEC_TRACE("VAL[%s]=%#zx(%zu) %zu bits: %s", name<IE>(), std::size_t(value), std::size_t(value), IE::traits::bits, ctx.buffer().toString());
		//TODO: estimate exact size needed? will it be faster?
		while (value >= 0x80)
		{
			ctx.buffer().template push<IE>(value | 0x80);
			CODEC_TRACE("\twrote %#02x, value=%#zx", uint8_t(value|0x80), std::size_t(value >> 7));
			value >>= 7;
		}
		ctx.buffer().template push<IE>(value);
		CODEC_TRACE("\twrote value %02X", uint8_t(value));
	}

	//IE_OCTET_STRING
	template <class IE>
	void operator() (IE const& ie, IE_OCTET_STRING)
	{
		uint8_t* out = ctx.buffer().template advance<IE>(ie.size());
		octets<IE::traits::min_octets, IE::traits::max_octets>::copy(out, ie.data(), ie.size());
		CODEC_TRACE("STR[%s] %zu octets: %s", name<IE>(), ie.size(), ctx.buffer().toString());
	}

private:
};

template <class CTX>
auto make_encoder(CTX& ctx) { return encoder<CTX>{ctx}; }

}	//end: namespace med::protobuf
