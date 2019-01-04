/**
@file
octet decoder definition

@copyright Denis Priyomov 2016-2017
Distributed under the MIT License
(See accompanying file LICENSE or visit https://github.com/cppden/med)
*/

#pragma once

#include <utility>

#include "debug.hpp"
#include "error.hpp"
#include "state.hpp"
#include "name.hpp"
#include "ie_type.hpp"

namespace med {

template <class DEC_CTX>
struct octet_decoder
{
	using state_type = typename DEC_CTX::buffer_type::state_type;
	using size_state = typename DEC_CTX::buffer_type::size_state;
	using allocator_type = typename DEC_CTX::allocator_type;
	static constexpr std::size_t granularity = 8;

	DEC_CTX& ctx;

	explicit octet_decoder(DEC_CTX& ctx_)
		: ctx(ctx_)
	{
	}

	allocator_type& get_allocator()                    { return ctx.get_allocator(); }

	//state
	auto operator() (PUSH_SIZE const& ps)              { return ctx.buffer().push_size(ps.size); }
	template <class IE>
	bool operator() (PUSH_STATE const&, IE const&)     { return ctx.buffer().push_state(); }
	bool operator() (POP_STATE const&)                 { return ctx.buffer().pop_state(); }
	auto operator() (GET_STATE const&)                 { return ctx.buffer().get_state(); }
	template <class IE>
	bool operator() (CHECK_STATE const&, IE const&)    { return !ctx.buffer().empty(); }
	void operator() (ADVANCE_STATE const& ss)
	{
		if (nullptr == ctx.buffer().advance(ss.bits/granularity))
		{ MED_RETURN_ERROR(error::OVERFLOW, (*this), "advance", ss.bits/granularity); }
	}
	void operator() (ADD_PADDING const& pad)
	{
		CODEC_TRACE("padding %zu bytes", pad.bits/granularity);
		if (nullptr == ctx.buffer().advance(pad.bits/granularity))
		{ MED_RETURN_ERROR(error::OVERFLOW, (*this), "padding", pad.bits/granularity); }
	}

	//errors
	template <typename... ARGS>
	void operator() (error e, ARGS&&... args)
	{
		return ctx.error_ctx().set_error(ctx.buffer(), e, std::forward<ARGS>(args)...);
	}

	//IE_VALUE
	template <class IE>
	void operator() (IE& ie, IE_VALUE const&)
	{
		static_assert(0 == (IE::traits::bits % granularity), "OCTET VALUE EXPECTED");
		CODEC_TRACE("->VAL[%s] %zu bits: %s", name<IE>(), IE::traits::bits, ctx.buffer().toString());
		if (uint8_t const* pval = ctx.buffer().template advance<IE::traits::bits / granularity>())
		{
			auto const val = get_bytes<IE>(pval);
			if constexpr (std::is_same_v<bool, decltype(ie.set_encoded(val))>)
			{
				if (not ie.set_encoded(val))
				{
					MED_RETURN_ERROR(error::INVALID_VALUE, (*this), name<IE>(), val, ctx.buffer().get_offset());
				}
			}
			else
			{
				ie.set_encoded(val);
			}
			CODEC_TRACE("<-VAL[%s]=%zx: %s", name<IE>(), std::size_t(val), ctx.buffer().toString());
		}
		else
		{
			MED_RETURN_ERROR(error::OVERFLOW, (*this), name<IE>(), IE::traits::bits/granularity);
		}
	}

	//IE_OCTET_STRING
	template <class IE>
	void operator() (IE& ie, IE_OCTET_STRING const&)
	{
		CODEC_TRACE("STR[%s] <-(%zu bytes): %s", name<IE>(), ctx.buffer().size(), ctx.buffer().toString());
		if (ie.set_encoded(ctx.buffer().size(), ctx.buffer().begin()))
		{
			CODEC_TRACE("STR[%s] -> len = %zu bytes", name<IE>(), std::size_t(ie.size()));
			if (nullptr == ctx.buffer().advance(ie.size()))
			{ MED_RETURN_ERROR(error::OVERFLOW, (*this), name<IE>(), ie.size()); }
		}
		else
		{
			MED_RETURN_ERROR(error::INVALID_VALUE, (*this), name<IE>(), ie.size(), ctx.buffer().get_offset());
		}
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
};

template <class ENC_CTX>
auto make_octet_decoder(ENC_CTX& ctx) { return octet_decoder<ENC_CTX>{ctx}; }

}	//end: namespace med
