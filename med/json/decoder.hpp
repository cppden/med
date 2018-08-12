#pragma once
/**
@file
JSON decoder

@copyright Denis Priyomov 2018
Distributed under the MIT License
(See accompanying file LICENSE or visit https://github.com/cppden/med)
*/

#include "../debug.hpp"
#include "../name.hpp"
#include "../state.hpp"
#include "../tag_named.hpp"
#include "json.hpp"

namespace med::json {

template <typename C>
constexpr bool is_whitespace(C const c)
{
	return c == C(' ') || c == C('\n') || c == C('\r') || c == C('\t');
}

template <typename BUFFER>
inline bool skip_ws_after(BUFFER& buffer, int expected)
{
	auto p = buffer.begin(), pe = buffer.end();
	while (p != pe && is_whitespace(*p)) ++p;
	if (p != pe)
	{
		buffer.advance(p - buffer.begin());
		if (*p == expected)
		{
			++p;
			while (p != pe && is_whitespace(*p)) ++p;
			buffer.advance(p - buffer.begin());
			return true;
		}
	}
	return false;
}


template <class DEC_CTX>
struct decoder
{
	//required for length_encoder
	using state_type = typename DEC_CTX::buffer_type::state_type;
	using size_state = typename DEC_CTX::buffer_type::size_state;
	using allocator_type = typename DEC_CTX::allocator_type;
	enum : std::size_t { granularity = 1 };
	static constexpr auto codec_kind = codec_e::STRUCTURED;

	DEC_CTX& ctx;

	explicit decoder(DEC_CTX& ctx_) : ctx{ ctx_ } { }
	allocator_type& get_allocator()                    { return ctx.get_allocator(); }

	//state
	auto operator() (PUSH_SIZE const& ps)              { return ctx.buffer().push_size(ps.size); }
	bool operator() (PUSH_STATE const&)                { return ctx.buffer().push_state(); }
	bool operator() (POP_STATE const&)                 { return ctx.buffer().pop_state(); }
	auto operator() (GET_STATE const&)                 { return ctx.buffer().get_state(); }
	bool operator() (CHECK_STATE const&)               { return !ctx.buffer().empty(); }
//	MED_RESULT operator() (ADVANCE_STATE const& ss)
//	{
//		if (ctx.buffer().advance(ss.bits/granularity)) MED_RETURN_SUCCESS;
//		return ctx.error_ctx().set_error(error::OVERFLOW, "advance", ctx.buffer().size() * granularity, ss.bits);
//	}

	//errors
	template <typename... ARGS>
	MED_RESULT operator() (error e, ARGS&&... args)    { return ctx.error_ctx().set_error(e, std::forward<ARGS>(args)...); }

	//CONTAINER
	template <class IE>
	MED_RESULT operator() (IE const&, ENTRY_CONTAINER const&)
	{
		constexpr auto open_brace = []()
		{
			if constexpr (IE::ordered) return '[';
			else return '{';
		};

		if (skip_ws_after(ctx.buffer(), open_brace()))
		{
			CODEC_TRACE("CONTAINER%c[%s]: %s", open_brace(), name<IE>(), ctx.buffer().toString());
			MED_RETURN_SUCCESS;
		}
		return ctx.error_ctx().set_error(error::INVALID_VALUE, name<IE>(), open_brace());
	}
	template <class IE>
	MED_RESULT operator() (IE const&, HEADER_CONTAINER const&)
	{
		if (skip_ws_after(ctx.buffer(), ':'))
		{
			CODEC_TRACE("CONTAINER:[%s]: %s", name<IE>(), ctx.buffer().toString());
			MED_RETURN_SUCCESS;
		}
		MED_RETURN_ERROR(error::INVALID_VALUE, (*this), name<IE>(), ':');
	}
	template <class IE>
	MED_RESULT operator() (IE const&, EXIT_CONTAINER const&)
	{
		constexpr auto closing_brace = []()
		{
			if constexpr (IE::ordered) return ']';
			else return '}';
		};

		if (skip_ws_after(ctx.buffer(), closing_brace()))
		{
			CODEC_TRACE("CONTAINER%c[%s]: %s", closing_brace(), name<IE>(), ctx.buffer().toString());
			MED_RETURN_SUCCESS;
		}
		MED_RETURN_ERROR(error::INVALID_VALUE, (*this), name<IE>(), closing_brace(), ctx.buffer().get_offset());
	}

	template <class IE>
	MED_RESULT operator() (IE const&, NEXT_CONTAINER_ELEMENT const&)
	{
		if (skip_ws_after(ctx.buffer(), ','))
		{
			CODEC_TRACE("CONTAINER,[%s]: %s", name<IE>(), ctx.buffer().toString());
			MED_RETURN_SUCCESS;
		}
		if (ctx.buffer())
		{
			&& ']' == *ctx.buffer().begin())
		MED_RETURN_ERROR(error::INVALID_VALUE, (*this), name<IE>(), ',', ctx.buffer().get_offset());
	}

	//IE_VALUE
	template <class IE>
	MED_RESULT operator() (IE& ie, IE_VALUE const&)
	{
		auto* p = ctx.buffer().template advance<1>();
		if (p)
		{
			//CODEC_TRACE("->VAL[%s]: %.*s", name<IE>(), 5, p);
			if constexpr (std::is_same_v<bool, typename IE::value_type>)
			{
				if ('f' == p[0] && nullptr != ctx.buffer().template advance<4>()) //false
				{
					static constexpr char const* s = "false";
					if (s[1] == p[1] && s[2] == p[2] && s[3] == p[3] && s[4] == p[4])
					{
						CODEC_TRACE("%s=%s", name<IE>(), s);
						ie.set_encoded(false);
						MED_RETURN_SUCCESS;
					}
				}
				else if ('t' == p[0] && nullptr != ctx.buffer().template advance<3>()) //true
				{
					static constexpr char const* s = "true";
					if (s[1] == p[1] && s[2] == p[2] && s[3] == p[3])
					{
						CODEC_TRACE("%s=%s", name<IE>(), s);
						ie.set_encoded(true);
						MED_RETURN_SUCCESS;
					}
				}
			}
			else if constexpr (std::is_floating_point_v<typename IE::value_type>)
			{
				return convert<long double>(p, "%Lg%n", ie);
			}
			else if constexpr (std::is_signed_v<typename IE::value_type>)
			{
				return convert<long long>(p, "%lld%n", ie);
			}
			else if constexpr (std::is_unsigned_v<typename IE::value_type>)
			{
				return convert<unsigned long long>(p, "%llu%n", ie);
			}
		}

		if (p)
		{
			MED_RETURN_ERROR(error::INVALID_VALUE, (*this), name<IE>(), p[0], p - ctx.buffer().get_start());
		}
		else
		{
			MED_RETURN_ERROR(error::OVERFLOW, (*this), name<IE>(), 0, 3);
		}
	}

	//IE_OCTET_STRING
	template <class IE>
	MED_RESULT operator() (IE& ie, IE_OCTET_STRING const&)
	{
		//CODEC_TRACE("->STR[%s]: %s", name<IE>(), ctx.buffer().toString());
		auto* p = ctx.buffer().begin(), pe = ctx.buffer().end();
		if (p != pe && '"' == *p++)
		{
			auto* const ps = p; //start of string
			if constexpr (has_hash_type_v<IE>)
			{
				using hs = med::hash<typename IE::hash_type>;
				auto hash_val = hs::init;
				while (p != pe)
				{
					auto const c = *p++;
					if ('"' != c)
					{
						hash_val = hs::update(c, hash_val);
					}
					else
					{
						if (ie.set_encoded(p - ps - 1, ps))
						{
							CODEC_TRACE("hash(%.*s)=%#zx", int(ie.size()), (char*)ie.data(), std::size_t(hash_val));
							ie.set_hash(hash_val);
							ctx.buffer().offset(p - ps + 1);
							MED_RETURN_SUCCESS;
						}
					}
				}
			}
			else
			{
				while (p != pe)
				{
					//TODO: consider escaped \"
					if ('"' == *p++)
					{
						if (ie.set_encoded(p - ps - 1, ps))
						{
							CODEC_TRACE("len(%.*s)=%zu", int(ie.size()), (char*)ie.data(), ie.size());
							ctx.buffer().offset(p - ps + 1);
							MED_RETURN_SUCCESS;
						}
					}
				}
			}
		}
		MED_RETURN_ERROR(error::INVALID_VALUE, (*this), name<IE>(), ie.size(), ctx.buffer().get_offset());
	}

private:
	template <typename VAL, class IE>
	auto convert(char const* p, char const* fmt, IE& ie)
	{
		VAL val; int pos;
		if (1 == std::sscanf(p, fmt, &val, &pos) && ctx.buffer().template advance(pos - 1))
		{
			CODEC_TRACE("%s[%s]=%.*s", name<IE>(), fmt, pos, p);
			if constexpr (not std::is_same_v<VAL, typename IE::value_type>)
			{
				if (std::numeric_limits<typename IE::value_type>::min() >= val
				&& std::numeric_limits<typename IE::value_type>::max() <= val)
				{
					ie.set_encoded(static_cast<typename IE::value_type>(val));
				}
			}
			else
			{
				ie.set_encoded(val);
			}
			MED_RETURN_SUCCESS;
		}
		MED_RETURN_ERROR(error::INVALID_VALUE, (*this), name<IE>(), p[0], p - ctx.buffer().get_start());
	}
};

} //end: namespace med::json
