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
#include "sl/octet_info.hpp"
#include "json.hpp"

namespace med::json {

template <typename C>
constexpr bool is_whitespace(C const c)
{
	return c == C(' ') || c == C('\n') || c == C('\r') || c == C('\t');
}

//returns last non-ws value or 0 for EoF
template <typename BUFFER>
inline int skip_ws(BUFFER& buffer)
{
	auto p = buffer.begin(), pe = buffer.end();
	while (p != pe && is_whitespace(*p)) ++p;
	if (p != pe)
	{
		buffer.template advance<>(p - buffer.begin());
		return int(*p);
	}
	return 0;
}

template <typename BUFFER>
inline bool skip_ws_after(BUFFER& buffer, int expected)
{
	auto p = buffer.begin(), pe = buffer.end();
	while (p != pe && is_whitespace(*p)) ++p;
	if (p != pe)
	{
		buffer.template advance<>(p - buffer.begin());
		if (*p == expected)
		{
			++p;
			while (p != pe && is_whitespace(*p)) ++p;
			buffer.template advance<>(p - buffer.begin());
			return true;
		}
	}
	return false;
}


template <class DEC_CTX>
struct decoder : sl::octet_info
{
	//required for length_encoder
	using state_type = typename DEC_CTX::buffer_type::state_type;
	using size_state = typename DEC_CTX::buffer_type::size_state;
	using allocator_type = typename DEC_CTX::allocator_type;


	explicit decoder(DEC_CTX& ctx_) : m_ctx{ ctx_ } { }
	DEC_CTX& get_context() noexcept                 { return m_ctx; }
	allocator_type& get_allocator()                 { return get_context().get_allocator(); }

	//state
	auto operator() (PUSH_SIZE ps)                  { return get_context().buffer().push_size(ps.size, ps.commit); }
	template <class IE>
	bool operator() (PUSH_STATE, IE const&)         { return this->template test_eof<IE>() && get_context().buffer().push_state(); }
	bool operator() (POP_STATE)                     { return get_context().buffer().pop_state(); }
	auto operator() (GET_STATE)                     { return get_context().buffer().get_state(); }
	template <class IE>
	bool operator() (CHECK_STATE, IE const&)        { return this->template test_eof<IE>() && not get_context().buffer().empty(); }

	template <class IE>
	void operator() (ENTRY_CONTAINER, IE const&)
	{
		constexpr auto open_brace = []()
		{
			if constexpr (is_multi_field_v<IE>) return '[';
			else return '{';
		};

		if (not skip_ws_after(get_context().buffer(), open_brace()))
		{
			MED_THROW_EXCEPTION(invalid_value, name<IE>(), open_brace(), get_context().buffer());
		}
		CODEC_TRACE("entry<%c> [%s]: %s", open_brace(), name<IE>(), get_context().buffer().toString());
	}
	template <class IE>
	void operator() (HEADER_CONTAINER, IE const&)
	{
		if (not skip_ws_after(get_context().buffer(), ':'))
		{
			MED_THROW_EXCEPTION(invalid_value, name<IE>(), ':', get_context().buffer());
		}
		CODEC_TRACE("head<:> [%s]: %s", name<IE>(), get_context().buffer().toString());
	}
	template <class IE>
	void operator() (EXIT_CONTAINER, IE const&)
	{
		constexpr auto closing_brace = []()
		{
			if constexpr (is_multi_field_v<IE>) return ']';
			else return '}';
		};

		if (not skip_ws_after(get_context().buffer(), closing_brace()))
		{
			MED_THROW_EXCEPTION(invalid_value, name<IE>(), closing_brace(), get_context().buffer());
		}
		CODEC_TRACE("exit<%c> [%s]: %s", closing_brace(), name<IE>(), get_context().buffer().toString());
	}

	template <class IE>
	void operator() (NEXT_CONTAINER_ELEMENT, IE const&)
	{
		if (not skip_ws_after(get_context().buffer(), ','))
		{
			MED_THROW_EXCEPTION(invalid_value, name<IE>(), ',', get_context().buffer());
		}
		CODEC_TRACE("CONTAINER-,-[%s]: %s", name<IE>(), get_context().buffer().toString());
	}

	//IE_VALUE
	template <class IE>
	void operator() (IE& ie, IE_VALUE)
	{
		auto* p = get_context().buffer().template advance<IE, 1>();
		//CODEC_TRACE("->VAL[%s]: %.*s", name<IE>(), 5, p);
		if constexpr (std::is_same_v<bool, typename IE::value_type>)
		{
			get_context().buffer().template advance<IE, 3>(); //min len of [t]rue, [f]alse
			if ('t' == p[0] && 'r' == p[1] && 'u' == p[2] && 'e' == p[3]) //true
			{
				CODEC_TRACE("%s=true", name<IE>());
				ie.set_encoded(true);
				return;
			}
			else
			{
				get_context().buffer().template pop<IE>(); //+1 over len(true)
				if ('f' == p[0] && 'a' == p[1] && 'l' == p[2] && 's' == p[3] && 'e' == p[4])
				{
					CODEC_TRACE("%s=false", name<IE>());
					ie.set_encoded(false);
					return;
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

		MED_THROW_EXCEPTION(invalid_value, name<IE>(), p[0], get_context().buffer());
	}

	//IE_OCTET_STRING
	template <class IE>
	void operator() (IE& ie, IE_OCTET_STRING)
	{
		//CODEC_TRACE("->STR[%s]: %s", name<IE>(), get_context().buffer().toString());
		auto p = get_context().buffer().begin(), pe = get_context().buffer().end();
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
							get_context().buffer().offset(p - ps + 1);
							return;
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
							get_context().buffer().offset(p - ps + 1);
							return;
						}
					}
				}
			}
		}
		MED_THROW_EXCEPTION(invalid_value, name<IE>(), ie.size(), get_context().buffer());
	}

private:
	template <class IE>
	bool test_eof()
	{
		constexpr auto closing_brace = []()
		{
			if constexpr (is_multi_field_v<IE>) return ']';
			else return '}';
		};

		if (skip_ws(get_context().buffer()) == closing_brace())
		{
			CODEC_TRACE("EOF %c [%s]: %s", closing_brace(), name<IE>(), get_context().buffer().toString());
			return false;
		}
		CODEC_TRACE("not EOF %c [%s]: %s", closing_brace(), name<IE>(), get_context().buffer().toString());
		return true;
	}

	template <typename VAL, class IE>
	auto convert(char const* p, char const* fmt, IE& ie)
	{
		VAL val;
		int pos;
		if (1 == std::sscanf(p, fmt, &val, &pos) && get_context().buffer().advance(pos - 1))
		{
			CODEC_TRACE("%s[%s]=%.*s", name<IE>(), fmt, pos, p);
			if constexpr (not std::is_same_v<VAL, typename IE::value_type>)
			{
				if (val >= std::numeric_limits<typename IE::value_type>::min()
				&& val <= std::numeric_limits<typename IE::value_type>::max())
				{
					ie.set_encoded(static_cast<typename IE::value_type>(val));
					return;
				}
			}
			else
			{
				ie.set_encoded(val);
				return;
			}
		}
		MED_THROW_EXCEPTION(invalid_value, name<IE>(), p[0], get_context().buffer());
	}

	DEC_CTX& m_ctx;
};

} //end: namespace med::json
