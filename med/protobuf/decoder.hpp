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

	explicit decoder(DEC_CTX& ctx_) : m_ctx{ctx_} { }
	DEC_CTX& get_context() noexcept             { return m_ctx; }
	allocator_type& get_allocator()             { return get_context().get_allocator(); }

	//state
	auto operator() (PUSH_SIZE ps)              { return get_context().buffer().push_size(ps.size, ps.commit); }
	template <class IE>
	bool operator() (PUSH_STATE, IE const&)     { return get_context().buffer().push_state(); }
	bool operator() (POP_STATE)                 { return get_context().buffer().pop_state(); }
	auto operator() (GET_STATE)                 { return get_context().buffer().get_state(); }
	template <class IE>
	bool operator() (CHECK_STATE, IE const&)    { return !get_context().buffer().empty(); }
	void operator() (ADVANCE_STATE ss)          { get_context().buffer().template advance<ADVANCE_STATE>(ss.delta); }

	//IE_TAG
	template <class IE> [[nodiscard]] auto operator() (IE&, IE_TAG)
	{
		CODEC_TRACE("TAG[%s]: %s", name<IE>(), get_context().buffer().toString());
		as_writable_t<IE> ie;
		(*this)(ie, typename as_writable_t<IE>::ie_type{});
		return ie.get_encoded();
	}

	//IE_VALUE
	//Little Endian Base 128: https://en.wikipedia.org/wiki/LEB128
	template <class IE>
	void operator() (IE& ie, IE_VALUE)
	{
		static_assert(0 == (IE::traits::bits % 8), "OCTET VALUE EXPECTED");
		CODEC_TRACE("->VAL[%s] %zu bits: %s", name<IE>(), IE::traits::bits, get_context().buffer().toString());
		typename IE::value_type val = get_context().buffer().template pop<IE>();
		if (val & 0x80)
		{
			val &= 0x7F;
			std::size_t count = 0;
			for (;;)
			{
				if (++count < MAX_VARINT_BYTES)
				{
					auto const byte = get_context().buffer().template pop<IE>();
					val |= static_cast<typename IE::value_type>(byte & 0x7F) << (7 * count);
					if (0 == (byte & 0x80)) { break; }
				}
				else
				{
					MED_THROW_EXCEPTION(invalid_value, name<IE>(), val, get_context().buffer())
				}
			}
		}

		if constexpr (std::is_same_v<bool, decltype(ie.set_encoded(val))>)
		{
			if (not ie.set_encoded(val))
			{
				MED_THROW_EXCEPTION(invalid_value, name<IE>(), val, get_context().buffer())
			}
		}
		else
		{
			ie.set_encoded(val);
		}
		CODEC_TRACE("<-VAL[%s]=%zx: %s", name<IE>(), std::size_t(val), get_context().buffer().toString());
	}

	//IE_OCTET_STRING
	template <class IE>
	void operator() (IE& ie, IE_OCTET_STRING)
	{
		CODEC_TRACE("STR[%s] <-(%zu bytes): %s", name<IE>(), get_context().buffer().size(), get_context().buffer().toString());
		if (ie.set_encoded(get_context().buffer().size(), get_context().buffer().begin()))
		{
			CODEC_TRACE("STR[%s] -> len = %zu bytes", name<IE>(), std::size_t(ie.size()));
			get_context().buffer().template advance<IE>(ie.size());
		}
		else
		{
			MED_THROW_EXCEPTION(invalid_value, name<IE>(), ie.size(), get_context().buffer())
		}
	}

private:
	DEC_CTX& m_ctx;
};

template <class C> explicit decoder(C&) -> decoder<C>;

}	//end: namespace med::protobuf
