/*!
@file
TODO: define.

@copyright Denis Priyomov 2016
Distributed under the MIT License
(See accompanying file LICENSE or copy at https://opensource.org/licenses/MIT)
*/

#pragma once

#include <utility>

#include "state.hpp"
#include "name.hpp"
#include "ie_type.hpp"

namespace med {

template <class DEC_CTX>
struct octet_decoder
{
	using state_type = typename DEC_CTX::buffer_type::state_type;
	using size_state = typename DEC_CTX::buffer_type::size_state;
	enum : std::size_t { granularity = 8 };

	DEC_CTX& ctx;

	explicit octet_decoder(DEC_CTX& ctx_)
		: ctx(ctx_)
	{
	}

	//state
	auto push_size(std::size_t size)                   { return ctx.buffer().push_size(size); }
	bool push_state()                                  { return ctx.buffer().push_state(); }
	void pop_state()                                   { ctx.buffer().pop_state(); }
	auto get_state() const                             { return ctx.buffer().get_state(); }
	bool eof() const                                   { return ctx.buffer().empty(); }
	bool advance(int bits)
	{
		if (ctx.buffer().advance(bits/granularity)) return true;
		ctx.error_ctx().spaceError("advance", ctx.buffer().size() * granularity, bits);
		return false;
	}

	//errors
	void operator() (error e, char const* psz = nullptr, std::size_t val0 = 0, std::size_t val1 = 0, std::size_t val2 = 0)
	{
		//TODO: use ctx.buffer().get_offset()
		ctx.error_ctx().set_error(e, psz, val0, val1, val2);
	}

	//IE_VALUE
	template <class IE>
	bool operator() (IE& ie, IE_VALUE const&)
	{
		static_assert(0 == (IE::traits::bits % granularity), "OCTET VALUE EXPECTED");
		//CODEC_TRACE("->VAL[%s] %u bits: %s", name<IE>(), IE::traits::bits, ctx.buffer().toString());
		if (uint8_t const* pval = ctx.buffer().advance(IE::traits::bits / granularity))
		{
			auto const val = get_bytes<IE>(pval);
			if (set_value(ie, val))
			{
				CODEC_TRACE("<-VAL[%s]=%zx: %s", name<IE>(), std::size_t(val), ctx.buffer().toString());
				return true;
			}
			else
			{
				ctx.error_ctx().valueError(name<IE>(), val, ctx.buffer().get_offset());
			}
		}
		else
		{
			ctx.error_ctx().spaceError(name<IE>(), ctx.buffer().size() * granularity, IE::traits::bits);
		}
		return false;
	}

	//IE_OCTET_STRING
	template <class IE>
	bool operator() (IE& ie, IE_OCTET_STRING const&)
	{
		CODEC_TRACE("STR[%s] <-(%u bytes): %s", name<IE>(), ctx.buffer().size(), ctx.buffer().toString());
		if (ie.set_encoded(ctx.buffer().size(), ctx.buffer().begin()))
		{
			CODEC_TRACE("STR[%s] -> len = %u bytes", name<IE>(), ie.get_length());
			if (ctx.buffer().advance(ie.get_length())) return true;
			ctx.error_ctx().spaceError(name<IE>(), ctx.buffer().size() * granularity, ie.get_length() * granularity);
		}
		else
		{
			ctx.error_ctx().valueError(name<IE>(), ie.get_length(), ctx.buffer().get_offset());
		}
		return false;
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
		enum { NUM_BYTES = IE::traits::bits / granularity };
		typename IE::value_type value{};
		get_bytes_impl(input, value, std::make_index_sequence<NUM_BYTES>{});
		return value;
	}
};

template <class ENC_CTX>
auto make_octet_decoder(ENC_CTX& ctx) { return octet_decoder<ENC_CTX>{ctx}; }

}	//end: namespace med
