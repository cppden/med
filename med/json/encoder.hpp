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
#include "sl/octet_info.hpp"
#include "json.hpp"

namespace med::json {

template <class ENC_CTX>
struct encoder : sl::octet_info
{
	//required for length_encoder
	using state_type = typename ENC_CTX::buffer_type::state_type;
	using allocator_type = typename ENC_CTX::allocator_type;


	explicit encoder(ENC_CTX& ctx_) : m_ctx{ ctx_ } { }
	ENC_CTX& get_context() noexcept                 { return m_ctx; }
	allocator_type& get_allocator() noexcept        { return get_context().get_allocator(); }

	//state
	auto operator() (GET_STATE)                       { return get_context().buffer().get_state(); }
	void operator() (SET_STATE, state_type const& st) { get_context().buffer().set_state(st); }

	template <class IE>
	bool operator() (PUSH_STATE, IE const&)           { return get_context().buffer().push_state(); }
	void operator() (POP_STATE)                       { get_context().buffer().pop_state(); }

	template <class IE>
	void operator() (ENTRY_CONTAINER, IE const&)
	{
		constexpr auto open_brace = []()
		{
			if constexpr (is_multi_field_v<IE>) return '[';
			else return '{';
		};

		CODEC_TRACE("entry<%c> [%s]: %s", open_brace(), name<IE>(), get_context().buffer().toString());
		get_context().buffer().template push<IE>(open_brace());
	}

	template <class IE>
	void operator() (HEADER_CONTAINER, IE const&)
	{
		CODEC_TRACE("head<:> %s: %s", name<IE>(), get_context().buffer().toString());
		get_context().buffer().template push<IE>(':');
	}

	template <class IE>
	void operator() (EXIT_CONTAINER, IE const&)
	{
		constexpr auto closing_brace = []()
		{
			if constexpr (is_multi_field_v<IE>) return ']';
			else return '}';
		};

		CODEC_TRACE("exit<%c> [%s]: %s", closing_brace(), name<IE>(), get_context().buffer().toString());
		get_context().buffer().template push<IE>(closing_brace());
	}

	template <class IE>
	void operator() (NEXT_CONTAINER_ELEMENT, IE const&)
	{
		CODEC_TRACE("CONTAINER,[%s]: %s", name<IE>(), get_context().buffer().toString());
		get_context().buffer().template push<IE>(',');
	}

	//IE_VALUE
	template <class IE>
	void operator() (IE const& ie, IE_VALUE)
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

		CODEC_TRACE("VAL[%s]: value_len=%zu %s", name<IE>(), len, get_context().buffer().toString());
		auto* out = get_context().buffer().template advance<IE, len>();
		if constexpr (std::is_same_v<bool, typename IE::value_type>)
		{
			if (ie.get())
			{
				octets<4,4>::copy(out, "true", 4);
				get_context().buffer().template advance<IE, -1>();
			}
			else
			{
				octets<5,5>::copy(out, "false", 5);
			}
		}
		else if constexpr (std::is_floating_point_v<typename IE::value_type>)
		{
			int const written = std::snprintf(out, len, "%g", ie.get());
			get_context().buffer().template advance<IE>(written - len);
		}
		else if constexpr (std::is_signed_v<typename IE::value_type>)
		{
			int const written = std::snprintf(out, len, "%lld", static_cast<long long>(ie.get()));
			get_context().buffer().template advance<IE>(written - len);
		}
		else if constexpr (std::is_unsigned_v<typename IE::value_type>)
		{
			int const written = std::snprintf(out, len, "%llu", static_cast<unsigned long long>(ie.get()));
			get_context().buffer().template advance<IE>(written - len);
		}
		else
		{
			MED_THROW_EXCEPTION(invalid_value, name<IE>(), 0, get_context().buffer());
		}
	}

	//IE_OCTET_STRING
	template <class IE>
	void operator() (IE const& ie, IE_OCTET_STRING)
	{
		auto* out = get_context().buffer().template advance<IE>(ie.size() + 2); //2 quotes
		*out++ = '"';
		octets<IE::traits::min_octets, IE::traits::max_octets>::copy(out, ie.data(), ie.size());
		CODEC_TRACE("STR[%s] %zu octets: %s", name<IE>(), ie.size(), get_context().buffer().toString());
		out[ie.size()] = '"';
	}

private:
	ENC_CTX& m_ctx;
};

}	//end: namespace med::json
