/**
@file
Google Protobuf encoder definition

@copyright Denis Priyomov 2018
Distributed under the MIT License
(See accompanying file LICENSE or visit https://github.com/cppden/med)
*/

#pragma once

#include "debug.hpp"
#include "name.hpp"
#include "state.hpp"
#include "octet_string.hpp"
#include "sl/octet_info.hpp"

namespace med::protobuf {

template <class ENC_CTX>
struct encoder : sl::octet_info
{
	//required for length_encoder
	using state_type = typename ENC_CTX::buffer_type::state_type;
	using allocator_type = typename ENC_CTX::allocator_type;

	explicit encoder(ENC_CTX& ctx_) : m_ctx{ ctx_ }   { }
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
	void operator() (SNAPSHOT ss)                     { get_context().put_snapshot(ss); }

	//IE_TAG/IE_LEN
	template <class IE> void operator() (IE const& ie, IE_TAG)
		{ (*this)(ie, typename IE::ie_type{}); }

	//IE_VALUE
	//Little Endian Base 128: https://en.wikipedia.org/wiki/LEB128
	template <class IE>
	void operator() (IE const& ie, IE_VALUE)
	{
		static_assert(0 == (IE::traits::bits % 8), "OCTET VALUE EXPECTED");
		auto value = ie.get_encoded();
		CODEC_TRACE("VAL[%s]=%#zx(%zu) %zu bits: %s", name<IE>(), std::size_t(value), std::size_t(value), IE::traits::bits, get_context().buffer().toString());
		//TODO: estimate exact size needed? will it be faster?
		while (value >= 0x80)
		{
			get_context().buffer().template push<IE>(value | 0x80);
			CODEC_TRACE("\twrote %#02x, value=%#zx", uint8_t(value|0x80), std::size_t(value >> 7));
			value >>= 7;
		}
		get_context().buffer().template push<IE>(value);
		CODEC_TRACE("\twrote value %02X", uint8_t(value));
	}

	//IE_OCTET_STRING
	template <class IE>
	void operator() (IE const& ie, IE_OCTET_STRING)
	{
		uint8_t* out = get_context().buffer().template advance<IE>(ie.size());
		octets<IE::traits::min_octets, IE::traits::max_octets>::copy(out, ie.data(), ie.size());
		CODEC_TRACE("STR[%s] %zu octets: %s", name<IE>(), ie.size(), get_context().buffer().toString());
	}

private:
	ENC_CTX& m_ctx;
};

template <class C> explicit encoder(C&) -> encoder<C>;

}	//end: namespace med::protobuf
