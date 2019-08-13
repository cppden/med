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
#include "protobuf.hpp"
#include "sl/octet_info.hpp"

namespace med::protobuf {

template <class DEC_CTX>
struct decoder : sl::octet_info
{
	using state_type = typename DEC_CTX::buffer_type::state_type;
	using size_state = typename DEC_CTX::buffer_type::size_state;
	using allocator_type = typename DEC_CTX::allocator_type;

	DEC_CTX& ctx;

	explicit decoder(DEC_CTX& ctx_)
		: ctx{ctx_}
	{
	}

	allocator_type& get_allocator()              { return ctx.get_allocator(); }

	//state
	auto operator() (PUSH_SIZE const& ps)        { return ctx.buffer().push_size(ps.size); }
	template <class IE>
	bool operator() (PUSH_STATE, IE const&)      { return ctx.buffer().push_state(); }
	bool operator() (POP_STATE)                  { return ctx.buffer().pop_state(); }
	auto operator() (GET_STATE)                  { return ctx.buffer().get_state(); }
	template <class IE>
	bool operator() (CHECK_STATE, IE const&)     { return !ctx.buffer().empty(); }
	void operator() (ADVANCE_STATE const& ss)    { ctx.buffer().template advance<ADVANCE_STATE>(ss.delta); }

	//IE_VALUE
	//Little Endian Base 128: https://en.wikipedia.org/wiki/LEB128
	template <class IE>
	void operator() (IE& ie, IE_VALUE)
	{
		static_assert(0 == (IE::traits::bits % 8), "OCTET VALUE EXPECTED");
		CODEC_TRACE("->VAL[%s] %zu bits: %s", name<IE>(), IE::traits::bits, ctx.buffer().toString());
		typename IE::value_type val = ctx.buffer().template pop<IE>();
		if (val & 0x80)
		{
			val &= 0x7F;
			std::size_t count = 0;
			for (;;)
			{
				if (++count < MAX_VARINT_BYTES)
				{
					auto const byte = ctx.buffer().template pop<IE>();
					val |= static_cast<typename IE::value_type>(byte & 0x7F) << (7 * count);
					if (0 == (byte & 0x80)) { break; }
				}
				else
				{
					MED_THROW_EXCEPTION(invalid_value, name<IE>(), val, ctx.buffer())
				}
			};
		}

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
		CODEC_TRACE("<-VAL[%s]=%zx: %s", name<IE>(), std::size_t(val), ctx.buffer().toString());
	}

	//IE_OCTET_STRING
	template <class IE>
	void operator() (IE& ie, IE_OCTET_STRING)
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
};

template <class CTX>
auto make_decoder(CTX& ctx) { return decoder<CTX>{ctx}; }

}	//end: namespace med::protobuf
