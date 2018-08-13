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

namespace med::protobuf {

template <class DEC_CTX>
struct decoder
{
	using state_type = typename DEC_CTX::buffer_type::state_type;
	using size_state = typename DEC_CTX::buffer_type::size_state;
	using allocator_type = typename DEC_CTX::allocator_type;
	static constexpr std::size_t granularity = 8;

	DEC_CTX& ctx;

	explicit decoder(DEC_CTX& ctx_)
		: ctx{ctx_}
	{
	}

	allocator_type& get_allocator()                     { return ctx.get_allocator(); }

	//state
	auto operator() (PUSH_SIZE const& ps)               { return ctx.buffer().push_size(ps.size); }
	template <class IE>
	bool operator() (PUSH_STATE const&, IE const&)      { return ctx.buffer().push_state(); }
	bool operator() (POP_STATE const&)                  { return ctx.buffer().pop_state(); }
	auto operator() (GET_STATE const&)                  { return ctx.buffer().get_state(); }
	template <class IE>
	bool operator() (CHECK_STATE const&, IE const&)     { return !ctx.buffer().empty(); }
	MED_RESULT operator() (ADVANCE_STATE const& ss)
	{
		if (ctx.buffer().advance(ss.bits/granularity)) MED_RETURN_SUCCESS;
		return ctx.error_ctx().set_error(error::OVERFLOW, "advance", ctx.buffer().size() * granularity, ss.bits);
	}
	MED_RESULT operator() (ADD_PADDING const& pad)
	{
		CODEC_TRACE("padding %zu bytes", pad.bits/granularity);
		if (ctx.buffer().advance(pad.bits/granularity)) MED_RETURN_SUCCESS;
		return ctx.error_ctx().set_error(error::OVERFLOW, "padding", ctx.buffer().size() * granularity, pad.bits);
	}

	//errors
	template <typename... ARGS>
	MED_RESULT operator() (error e, ARGS&&... args)
	{
		return ctx.error_ctx().set_error(e, std::forward<ARGS>(args)...);
		//TODO: use ctx.buffer().get_offset()
	}

	//IE_VALUE
	//Little Endian Base 128: https://en.wikipedia.org/wiki/LEB128
	template <class IE>
	MED_RESULT operator() (IE& ie, IE_VALUE const&)
	{
		static_assert(0 == (IE::traits::bits % granularity), "OCTET VALUE EXPECTED");
		CODEC_TRACE("->VAL[%s] %zu bits: %s", name<IE>(), IE::traits::bits, ctx.buffer().toString());
		if (uint8_t const* input = ctx.buffer().template advance<1>())
		{
			typename IE::value_type val = *input;
			if (val & 0x80)
			{
				val &= 0x7F;
				std::size_t count = 0;
				for (;;)
				{
					input = ctx.buffer().template advance<1>();
					if (input)
					{
						if (++count < MAX_VARINT_BYTES)
						{
							auto const byte = *input;
							val |= static_cast<typename IE::value_type>(byte & 0x7F) << (7 * count);
							if (0 == (byte & 0x80)) { break; }
						}
						else
						{
							MED_RETURN_ERROR(error::INVALID_VALUE, (*this), name<IE>(), val, ctx.buffer().get_offset());
						}
					}
					else
					{
						return ctx.error_ctx().set_error(error::OVERFLOW, name<IE>(), ctx.buffer().size() * granularity, IE::traits::bits);
					}
				};
			}

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
			MED_RETURN_SUCCESS;
		}
		else
		{
			return ctx.error_ctx().set_error(error::OVERFLOW, name<IE>(), ctx.buffer().size() * granularity, IE::traits::bits);
		}
	}

	//IE_OCTET_STRING
	template <class IE>
	MED_RESULT operator() (IE& ie, IE_OCTET_STRING const&)
	{
		CODEC_TRACE("STR[%s] <-(%zu bytes): %s", name<IE>(), ctx.buffer().size(), ctx.buffer().toString());
		if (ie.set_encoded(ctx.buffer().size(), ctx.buffer().begin()))
		{
			CODEC_TRACE("STR[%s] -> len = %zu bytes", name<IE>(), std::size_t(ie.size()));
			if (ctx.buffer().advance(ie.size())) MED_RETURN_SUCCESS;
			return ctx.error_ctx().set_error(error::OVERFLOW, name<IE>(), ctx.buffer().size() * granularity, ie.size() * granularity);
		}
		MED_RETURN_ERROR(error::INVALID_VALUE, (*this), name<IE>(), ie.size(), ctx.buffer().get_offset());
	}

private:
};

template <class CTX>
auto make_decoder(CTX& ctx) { return decoder<CTX>{ctx}; }

}	//end: namespace med::protobuf
