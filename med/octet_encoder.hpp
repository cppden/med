/**
@file
octet encoder definition

@copyright Denis Priyomov 2016-2017
Distributed under the MIT License
(See accompanying file LICENSE or visit https://github.com/cppden/med)
*/

#pragma once

#include "name.hpp"
#include "state.hpp"
#include "length.hpp"
#include "octet_string.hpp"
#include "sl/octet_info.hpp"
#include "padding.hpp"

namespace med {

template <class ENC_CTX>
struct octet_encoder : sl::octet_info
{
	//required for length_encoder
	using state_type = typename ENC_CTX::buffer_type::state_type;
	template <class... PA>
	using padder_type = octet_padder<PA...>;
	using allocator_type = typename ENC_CTX::allocator_type;

	explicit octet_encoder(ENC_CTX& ctx_) : m_ctx{ ctx_ } { }
	ENC_CTX& get_context() noexcept                   { return m_ctx; }
	allocator_type& get_allocator()                   { return get_context().get_allocator(); }

	//state
	auto operator() (GET_STATE)                       { return get_context().buffer().get_state(); }
	void operator() (SET_STATE, state_type const& st) { get_context().buffer().set_state(st); }
	template <class IE>
	void operator() (SET_STATE, IE const& ie)
	{
		if (auto const ss = get_context().get_snapshot(ie))
		{
			auto const len = field_length(ie, *this);
			if (ss.validate_length(len))
			{
				get_context().buffer().set_state(ss);
			}
			else
			{
				MED_THROW_EXCEPTION(invalid_value, name<IE>(), len, get_context().buffer())
			}
		}
		else
		{
			MED_THROW_EXCEPTION(missing_ie, name<IE>(), 1, 0, get_context().buffer())
		}
	}

	template <class IE>
	bool operator() (PUSH_STATE, IE const&)           { return get_context().buffer().push_state(); }
	void operator() (POP_STATE)                       { get_context().buffer().pop_state(); }
	void operator() (ADVANCE_STATE ss)                { get_context().buffer().template advance<ADVANCE_STATE>(ss.delta); }
	void operator() (ADD_PADDING pad)                 { get_context().buffer().template fill<ADD_PADDING>(pad.pad_size, pad.filler); }
	void operator() (SNAPSHOT ss)                     { get_context().put_snapshot(ss); }

	template <class IE> constexpr std::size_t operator() (GET_LENGTH, IE const& ie) const noexcept
	{
		if constexpr (AMultiField<IE>)
		{
			std::size_t len = 0;
			for (auto& v : ie) { len += field_length(v, *this); }
			CODEC_TRACE("length(%s)*%zu = %zu", name<IE>(), ie.count(), len);
			return len;
		}
		else if constexpr (AHasSize<IE>)
		{
			CODEC_TRACE("length(%s) = %zu", name<IE>(), std::size_t(ie.size()));
			return ie.size();
		}
		else
		{
			std::size_t const len = bits_to_bytes(IE::traits::bits);
			CODEC_TRACE("length(%s) = %zu", name<IE>(), len);
			return len;
		}
	}

	//IE_TAG/IE_LEN
	template <class IE> void operator() (IE const& ie, IE_TAG)
		{ (*this)(ie, typename IE::ie_type{}); }
	template <class IE> void operator() (IE const& ie, IE_LEN)
		{ (*this)(ie, typename IE::ie_type{}); }

	//IE_NULL
	template <class IE> void operator() (IE const&, IE_NULL)
		{ CODEC_TRACE("NULL[%s]: %s", name<IE>(), get_context().buffer().toString()); }

	//IE_VALUE
	template <class IE> void operator() (IE const& ie, IE_VALUE)
	{
		static_assert(0 == (IE::traits::bits % 8), "OCTET VALUE EXPECTED");
		uint8_t* out = get_context().buffer().template advance<IE, bits_to_bytes(IE::traits::bits)>();
		put_bytes(ie, out);
		CODEC_TRACE("V=%zXh %zu bits[%s]: %s", std::size_t(ie.get_encoded()), IE::traits::bits, name<IE>(), get_context().buffer().toString());
	}

	//IE_OCTET_STRING
	template <class IE> void operator() (IE const& ie, IE_OCTET_STRING)
	{
		uint8_t* out = get_context().buffer().template advance<IE>(ie.size());
		octets<IE::traits::min_octets, IE::traits::max_octets>::copy(out, ie.data(), ie.size());
		CODEC_TRACE("STR[%s] %zu octets: %s", name<IE>(), ie.size(), get_context().buffer().toString());
	}

private:
	ENC_CTX& m_ctx;
};

}	//end: namespace med
