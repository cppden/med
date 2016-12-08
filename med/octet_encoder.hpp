/*!
@file
TODO: define.

@copyright Denis Priyomov 2016
Distributed under the MIT License
(See accompanying file LICENSE or copy at https://opensource.org/licenses/MIT)
*/

#pragma once

#include "debug.hpp"
#include "name.hpp"
#include "octet_string.hpp"

namespace med {

template <class ENC_CTX>
struct octet_encoder
{
	//required for length_encoder
	using state_type = typename ENC_CTX::buffer_type::state_type;
	enum : std::size_t { granularity = 8 };

	ENC_CTX& ctx;

	explicit octet_encoder(ENC_CTX& ctx_)
		: ctx(ctx_)
	{
	}

	//state
	auto operator() (GET_STATE const&)                       { return ctx.buffer().get_state(); }
	void operator() (SET_STATE const&, state_type const& st) { ctx.buffer().set_state(st); }
	template <class IE>
	bool operator() (SET_STATE const&, IE const& ie)
	{
		if (auto const ss = ctx.snapshot(ie))
		{
			if (ss == get_length(ie))
			{
				ctx.buffer().set_state(ss);
				return true;
			}
			else
			{
				ctx.error_ctx().set_error(error::INCORRECT_VALUE, name<IE>(), 0);
			}
		}
		else
		{
			ctx.error_ctx().set_error(error::MISSING_IE, name<IE>(), 0);
		}
		return false;
	}
	bool operator() (PUSH_STATE const&)                { return ctx.buffer().push_state(); }
	bool operator() (POP_STATE const&)                 { ctx.buffer().pop_state(); return true; }
	bool operator() (ADVANCE_STATE const& ss)
	{
		if (ctx.buffer().advance(ss.bits/granularity)) return true;
		ctx.error_ctx().spaceError("advance", ctx.buffer().size() * granularity, ss.bits);
		return false;
	}
	bool operator() (ADD_PADDING const& pad)
	{
		if (ctx.buffer().fill(pad.bits/granularity, pad.filler)) return true;
		ctx.error_ctx().spaceError("padding", ctx.buffer().size() * granularity, pad.bits);
		return false;
	}
	void operator() (SNAPSHOT const& ss)               { ctx.snapshot(ss); }

	//errors
	void operator() (error e, char const* psz = nullptr, std::size_t val0 = 0, std::size_t val1 = 0, std::size_t val2 = 0)
	{
		ctx.error_ctx().set_error(e, psz, val0, val1, val2);
	}

	//IE_VALUE
	template <class IE>
	bool operator() (IE const& ie, IE_VALUE const&)
	{
		static_assert(0 == (IE::traits::bits % granularity), "OCTET VALUE EXPECTED");
		CODEC_TRACE("VAL[%s]=%#zx %u bits: %s", name<IE>(), std::size_t(ie.get()), IE::traits::bits, ctx.buffer().toString());
		if (uint8_t* out = ctx.buffer().advance(IE::traits::bits / granularity))
		{
			put_bytes(ie, out);
			return true;
		}
		else
		{
			ctx.error_ctx().spaceError(name<IE>(), ctx.buffer().size() * granularity, IE::traits::bits);
			return false;
		}
	}

	//IE_OCTET_STRING
	template <class IE>
	bool operator() (IE const& ie, IE_OCTET_STRING const&)
	{
		if ( uint8_t* out = ctx.buffer().advance(ie.get_length()) )
		{
			octets<IE::min_octets, IE::max_octets>::copy(out, ie.data(), ie.get_length());
			CODEC_TRACE("STR[%s] %u octets: %s", name<IE>(), ie.get_length(), ctx.buffer().toString());
			return true;
		}
		else
		{
			ctx.error_ctx().spaceError(name<IE>(), ctx.buffer().size() * granularity, ie.get_length() * granularity);
			return false;
		}
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
		enum { NUM_BYTES = IE::traits::bits / granularity };
		put_bytes_impl<NUM_BYTES>(output, ie.get_encoded(), std::make_index_sequence<NUM_BYTES>{});
	}
};

template <class ENC_CTX>
auto make_octet_encoder(ENC_CTX& ctx) { return octet_encoder<ENC_CTX>{ctx}; }

}	//end: namespace med
