/**
@file
octet decoder definition

@copyright Denis Priyomov 2016-2017
Distributed under the MIT License
(See accompanying file LICENSE or visit https://github.com/cppden/med)
*/

#pragma once

#include <utility>

#include "exception.hpp"
#include "state.hpp"
#include "name.hpp"
#include "ie_type.hpp"
#include "sl/octet_info.hpp"

namespace med {

template <class DEC_CTX>
struct octet_decoder : sl::octet_info
{
	using state_type = typename DEC_CTX::buffer_type::state_type;
	using size_state = typename DEC_CTX::buffer_type::size_state;
	using allocator_type = typename DEC_CTX::allocator_type;

	DEC_CTX& ctx;

	explicit octet_decoder(DEC_CTX& c) : ctx{c} { }

	allocator_type& get_allocator()             { return ctx.get_allocator(); }

	//state
	auto operator() (PUSH_SIZE const& ps)       { return ctx.buffer().push_size(ps.size); }
	template <class IE>
	bool operator() (PUSH_STATE, IE const&)     { return ctx.buffer().push_state(); }
	bool operator() (POP_STATE)                 { return ctx.buffer().pop_state(); }
	auto operator() (GET_STATE)                 { return ctx.buffer().get_state(); }
	template <class IE>
	bool operator() (CHECK_STATE, IE const&)    { return !ctx.buffer().empty(); }
	void operator() (ADVANCE_STATE const& ss)   { ctx.buffer().template advance<ADVANCE_STATE>(ss.delta); }
	void operator() (ADD_PADDING const& pad)
	{
		CODEC_TRACE("padding %zu bytes", pad.pad_size);
		ctx.buffer().template advance<ADD_PADDING>(pad.pad_size);
	}

	//IE_TAG
	template <class IE> [[nodiscard]] std::size_t operator() (IE&, IE_TAG)
	{
		typename IE::writable ie;
		(*this)(ie, typename IE::writable::ie_type{});
		return ie.get_encoded();
	}
	//IE_LEN
	template <class IE> void operator() (IE& ie, IE_LEN)
	{
		(*this)(ie, typename IE::ie_type{});
	}

	//IE_NULL
	template <class IE> void operator() (IE&, IE_NULL)
	{
		CODEC_TRACE("NULL[%s]: %s", name<IE>(), ctx.buffer().toString());
	}

	//IE_VALUE
	template <class IE> void operator() (IE& ie, IE_VALUE)
	{
		static_assert(0 == (IE::traits::bits % 8), "OCTET VALUE EXPECTED");
		//CODEC_TRACE("->VAL[%s] %zu bits: %s", name<IE>(), IE::traits::bits, ctx.buffer().toString());
		uint8_t const* pval = ctx.buffer().template advance<IE, bits_to_bytes(IE::traits::bits)>();
		auto const val = get_bytes<IE>(pval);
		if constexpr (std::is_same_v<bool, decltype(ie.set_encoded(val))>)
		{
			if (not ie.set_encoded(val))
			{
				MED_THROW_EXCEPTION(invalid_value, name<IE>(), val, ctx.buffer())
			}
		}
		else
		{
			ie.set_encoded(val);
		}
		CODEC_TRACE("VAL=%zxh [%s]: %s", std::size_t(val), name<IE>(), ctx.buffer().toString());
	}

	//IE_OCTET_STRING
	template <class IE> void operator() (IE& ie, IE_OCTET_STRING)
	{
		CODEC_TRACE("STR[%s] <-(%zu bytes): %s", name<IE>(), ctx.buffer().size(), ctx.buffer().toString());
		if (ie.set_encoded(ctx.buffer().size(), ctx.buffer().begin()))
		{
			CODEC_TRACE("STR[%s] -> len = %zu bytes", name<IE>(), std::size_t(ie.size()));
			ctx.buffer().template advance<IE>(ie.size());
		}
		else
		{
			MED_THROW_EXCEPTION(invalid_value, name<IE>(), ie.size(), ctx.buffer())
		}
	}

private:
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
};

}	//end: namespace med
