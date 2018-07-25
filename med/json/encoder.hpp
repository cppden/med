/**
@file
JSON encoder

@copyright Denis Priyomov 2018
Distributed under the MIT License
(See accompanying file LICENSE or visit https://github.com/cppden/med)
*/

#pragma once

#include "../debug.hpp"
#include "../name.hpp"
#include "../state.hpp"
#include "json.hpp"

namespace med::json {

template <class ENC_CTX>
struct encoder
{
	//required for length_encoder
	using state_type = typename ENC_CTX::buffer_type::state_type;
	enum : std::size_t { granularity = 1 };
	static constexpr auto encoder_type = codec_type::CONTAINER;

	ENC_CTX& ctx;

	explicit encoder(ENC_CTX& ctx_) : ctx{ ctx_ } { }

	//state
	auto operator() (GET_STATE const&)                       { return ctx.buffer().get_state(); }
	void operator() (SET_STATE const&, state_type const& st) { ctx.buffer().set_state(st); }

	MED_RESULT operator() (PUSH_STATE const&)                { return ctx.buffer().push_state(); }
	void operator() (POP_STATE const&)                       { ctx.buffer().pop_state(); }
	// MED_RESULT operator() (ADVANCE_STATE const& ss)
	// {
	// 	if (ctx.buffer().advance(ss.bits/granularity)) MED_RETURN_SUCCESS;
	// 	return ctx.error_ctx().set_error(error::OVERFLOW, "advance", ctx.buffer().size(), ss.bits);
	// }

	//errors
	template <typename... ARGS>
	MED_RESULT operator() (error e, ARGS&&... args)          { return ctx.error_ctx().set_error(e, std::forward<ARGS>(args)...); }

	//CONTAINER
	template <class IE>
	MED_RESULT operator() (IE const&, ENTRY_CONTAINER const&)
	{
		CODEC_TRACE("CONTAINER+[%s]: %s", name<IE>(), ctx.buffer().toString());
		if (uint8_t* out = ctx.buffer().template advance<1>())
		{
			out[0] = '{';
			MED_RETURN_SUCCESS;
		}
		return ctx.error_ctx().set_error(error::OVERFLOW, name<IE>(), ctx.buffer().size(), 1);
	}
	template <class IE>
	MED_RESULT operator() (IE const&, EXIT_CONTAINER const&)
	{
		CODEC_TRACE("CONTAINER-[%s]: %s", name<IE>(), ctx.buffer().toString());
		if (uint8_t* out = ctx.buffer().template advance<1>())
		{
			out[0] = '}';
			MED_RETURN_SUCCESS;
		}
		return ctx.error_ctx().set_error(error::OVERFLOW, name<IE>(), ctx.buffer().size(), 1);
	}
	template <class IE>
	MED_RESULT operator() (IE const&, NEXT_CONTAINER_ELEMENT const&)
	{
		CODEC_TRACE("CONTAINER,[%s]: %s", name<IE>(), ctx.buffer().toString());
		if (uint8_t* out = ctx.buffer().template advance<1>())
		{
			out[0] = ',';
			MED_RETURN_SUCCESS;
		}
		return ctx.error_ctx().set_error(error::OVERFLOW, name<IE>(), ctx.buffer().size(), 1);
	}

	//IE_VALUE
	template <class IE>
	MED_RESULT operator() (IE const& ie, IE_VALUE const&)
	{
		constexpr std::size_t len = [](){
			if constexpr (std::is_same_v<bool, typename IE::value_type>)
			{
				return 5; //max len of 'true'/'false'
			}
			else
			{
				return 32; //TODO: more precise estimation?
			}
		}();

		CODEC_TRACE("VAL[%s]: value_len=%zu %s", name<IE>(), len, ctx.buffer().toString());
		if (auto* out = (char*)ctx.buffer().template advance<len>())
		{
			if constexpr (std::is_same_v<bool, typename IE::value_type>)
			{
				if (ie.get())
				{
					octets<4,4>::copy(out, "true", 4);
					ctx.buffer().template advance<-1>();
				}
				else
				{
					octets<5,5>::copy(out, "false", 5);
				}
			}
			else if constexpr (std::is_floating_point_v<typename IE::value_type>)
			{
				int const written = std::snprintf(out, len, "%g", ie.get());
				ctx.buffer().template advance(written - len);
			}
			else if constexpr (std::is_signed_v<typename IE::value_type>)
			{
				int const written = std::snprintf(out, len, "%lld", (long long)ie.get());
				ctx.buffer().template advance(written - len);
			}
			else if constexpr (std::is_unsigned_v<typename IE::value_type>)
			{
				int const written = std::snprintf(out, len, "%llu", (unsigned long long)ie.get());
				ctx.buffer().template advance(written - len);
			}

			MED_RETURN_SUCCESS;
		}
		return ctx.error_ctx().set_error(error::OVERFLOW, name<IE>(), ctx.buffer().size(), IE::traits::bits);
	}

	//IE_OCTET_STRING
	template <class IE>
	MED_RESULT operator() (IE const& ie, IE_OCTET_STRING const&)
	{
		constexpr auto added = []() //+2 is for "quotes" or +3 for "quotes":
		{
			if constexpr (med::is_tag_v<IE>) { return 3; }
			else { return 2; }
		};

		if (auto* out = (char*)ctx.buffer().advance(ie.size() + added()))
		{
			*out++ = '"';
			octets<IE::min_octets, IE::max_octets>::copy(out, ie.data(), ie.size());
			CODEC_TRACE("STR[%s] %zu octets: %s", name<IE>(), ie.size(), ctx.buffer().toString());
			out[ie.size()] = '"';
			if constexpr (added() == 3)
			{
				out[ie.size() + 1] = ':';
			}
			MED_RETURN_SUCCESS;
		}
		return ctx.error_ctx().set_error(error::OVERFLOW, name<IE>(), ctx.buffer().size(), ie.size());
	}

private:
};

template <class ENC_CTX>
auto make_encoder(ENC_CTX& ctx) { return encoder<ENC_CTX>{ctx}; }

}	//end: namespace med::json
