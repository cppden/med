/**
@file
octet decoder definition

@copyright Denis Priyomov 2016-2017
Distributed under the MIT License
(See accompanying file LICENSE or visit https://github.com/cppden/med)
*/

#pragma once

#include "bytes.hpp"
#include "exception.hpp"
#include "state.hpp"
#include "name.hpp"
#include "ie_type.hpp"
#include "sl/octet_info.hpp"
#include "padding.hpp"

namespace med {

template <class DEC_CTX>
struct octet_decoder : sl::octet_info
{
	using state_type = typename DEC_CTX::buffer_type::state_type;
	using size_state = typename DEC_CTX::buffer_type::size_state;
	using allocator_type = typename DEC_CTX::allocator_type;
	template <class... PA>
	using padder_type = octet_padder<PA...>;

	explicit octet_decoder(DEC_CTX& c) : m_ctx{c} { }
	DEC_CTX& get_context() noexcept             { return m_ctx; }
	allocator_type& get_allocator()             { return get_context().get_allocator(); }

	//state
	auto operator() (PUSH_SIZE ps)              { return get_context().buffer().push_size(ps.size, ps.commit); }
	template <class IE>
	bool operator() (PUSH_STATE, IE const&)     { return get_context().buffer().push_state(); }
	void operator() (POP_STATE)                 { get_context().buffer().pop_state(); }
	auto operator() (GET_STATE)                 { return get_context().buffer().get_state(); }
	template <class IE>
	bool operator() (CHECK_STATE, IE const&)    { return !get_context().buffer().empty(); }
	void operator() (ADVANCE_STATE ss)          { get_context().buffer().template advance<ADVANCE_STATE>(ss.delta); }
	void operator() (ADD_PADDING pad)           { get_context().buffer().template advance<ADD_PADDING>(pad.pad_size); }

	//IE_TAG
	template <class IE> [[nodiscard]] auto operator() (IE&, IE_TAG)
	{
		as_writable_t<IE> ie;
		(*this)(ie, typename as_writable_t<IE>::ie_type{});
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
		CODEC_TRACE("NULL[%s]: %s", name<IE>(), get_context().buffer().toString());
	}

	//IE_VALUE
	template <class IE> void operator() (IE& ie, IE_VALUE)
	{
		using value_t = typename IE::value_type;
		constexpr auto NUM_BITS = IE::traits::bits + IE::traits::offset;
		constexpr auto NUM_BYTES = bits_to_bytes(NUM_BITS);
		uint8_t const* pval = get_context().buffer().template advance_bits<IE, NUM_BITS>();

		auto const val = [](uint8_t const* in)
		{
			if constexpr (IE::traits::offset == 0 && (IE::traits::bits % 8) == 0)
			{
				return get_bytes<NUM_BYTES, value_t>(in);
			}
			else
			{
				size_t res = get_bytes<NUM_BYTES>(in);
				if constexpr (IE::traits::offset)
				{
					constexpr size_t MASK = (size_t(1) << (NUM_BYTES * 8 - IE::traits::offset)) - 1;
					CODEC_TRACE("V=%zXh(%zu) M=%zXh", res, NUM_BYTES, MASK);
					res &= MASK;
				}
				constexpr uint8_t RS = NUM_BYTES * 8 - NUM_BITS;
				if constexpr (RS) { res >>= RS; }
				return value_t(res);
			}
		}(pval);

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
		CODEC_TRACE("VAL=%zXh [%s]: %s", std::size_t(val), name<IE>(), get_context().buffer().toString());
	}

	//IE_OCTET_STRING
	template <class IE> void operator() (IE& ie, IE_OCTET_STRING)
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

}	//end: namespace med
