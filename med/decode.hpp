/**
@file
decoding entry point

@copyright Denis Priyomov 2016-2017
Distributed under the MIT License
(See accompanying file LICENSE or visit https://github.com/cppden/med)
*/

#pragma once

#include <utility>

#include "exception.hpp"
#include "optional.hpp"
#include "state.hpp"
#include "padding.hpp"
#include "length.hpp"
#include "name.hpp"
#include "placeholder.hpp"
#include "value.hpp"


namespace med {

//structure layer
namespace sl {

struct default_handler
{
	template <class FUNC, class IE, class HEADER>
	constexpr void operator()(FUNC&, IE const&, HEADER const& header) const
	{
		MED_THROW_EXCEPTION(unknown_tag, name<IE>(), get_tag(header));
	}
};

template <class WRAPPER, class FUNC, class IE, class UNEXP>
constexpr void decode_ie(FUNC& func, IE& ie, IE_NULL const&, UNEXP&)
{
	func(ie, typename WRAPPER::ie_type{});
	//ie.set();
}

template <class WRAPPER, class FUNC, class IE, class UNEXP>
constexpr void decode_ie(FUNC& func, IE& ie, PRIMITIVE const&, UNEXP&)
{
	CODEC_TRACE("PRIMITIVE %s", name<IE>());
	if constexpr (is_peek_v<IE>)
	{
		if (func(PUSH_STATE{}, ie))
		{
			func(ie, typename WRAPPER::ie_type{});
			func(POP_STATE{});
		}
	}
	else if constexpr (is_skip_v<IE>)
	{
		//TODO: need to support fixed octet_string here?
		return func.template advance<IE::traits::bits>();
	}
	else
	{
		return func(ie, typename WRAPPER::ie_type{});
	}
}

//Tag-Value
template <class WRAPPER, class FUNC, class IE, class UNEXP>
inline void decode_ie(FUNC& func, IE& ie, IE_TV const&, UNEXP& unexp)
{
	CODEC_TRACE("TV %s", name<IE>());
	//convert const to writable
	using TAG = typename WRAPPER::tag_type::writable;
	TAG tag;
	func(tag, typename TAG::ie_type{});
	if (WRAPPER::tag_type::match(tag.get_encoded()))
	{
		CODEC_TRACE("TV[%zu : %s]", std::size_t(tag.get()), name<WRAPPER>());
		return decode(func, ref_field(ie), unexp);
	}
	//NOTE: this can only be called for mandatory field thus it's fail case (not unexpected)
	MED_THROW_EXCEPTION(unknown_tag, name<WRAPPER>(), tag.get_encoded());
}

//Length-Value
template <class WRAPPER, class FUNC, class IE, class UNEXP>
inline void decode_ie(FUNC& func, IE& ie, IE_LV const&, UNEXP& unexp)
{
	typename WRAPPER::length_type len_ie;
	CODEC_TRACE("LV[%s]", name<WRAPPER>());
	decode(func, len_ie, unexp);
	std::size_t len_value = len_ie.get_encoded();
	CODEC_TRACE("raw len=%zu", len_value);
	value_to_length(len_ie, len_value);
	if (auto end = func(PUSH_SIZE{len_value}))
	{
		decode(func, ref_field(ie), unexp);
		//TODO: ??? as warning not error
		if (0 != end.size())
		{
			CODEC_TRACE("%s: end-size=%zu", name<IE>(), end.size());
			MED_THROW_EXCEPTION(overflow, name<WRAPPER>(), len_value);
		}
	}
	else
	{
		//TODO: something more informative: tried to set length beyond data end
		MED_THROW_EXCEPTION(overflow, name<WRAPPER>(), len_value);
	}
}

template <bool BY_IE, class FUNC, class IE, class UNEXP>
struct length_decoder;

template <class FUNC, class IE, class UNEXP>
struct len_dec_impl
{
	using state_type = typename FUNC::state_type;
	using size_state = typename FUNC::size_state;
	using allocator_type = typename FUNC::allocator_type;
	static constexpr std::size_t granularity = FUNC::granularity;
	static constexpr auto codec_kind = get_codec_kind_v<FUNC>;

	using length_type = typename IE::length_type;

	len_dec_impl(FUNC& decoder, IE& ie, UNEXP& unexp) noexcept
		: m_decoder{ decoder }
		, m_ie{ ie }
		, m_start{ m_decoder(GET_STATE{}) }
		, m_unexp{ unexp }
	{
		CODEC_TRACE("start %s with len=%s...:", name<IE>(), name<length_type>());
	}

	void restore_end()                                { m_size_state.restore_end(); }

#ifdef CODEC_TRACE_ENABLE
	~len_dec_impl() noexcept
	{
		CODEC_TRACE("finish %s with len=%s...:", name<IE>(), name<length_type>());
	}
#endif

	//check if placeholder was visited
	explicit operator bool() const                     { return static_cast<bool>(m_size_state); }
	auto size() const                                  { return m_size_state.size(); }

	template <class ...T>
	auto operator() (T&&... args)                      { return m_decoder(std::forward<T>(args)...); }

	allocator_type& get_allocator()                    { return m_decoder.get_allocator(); }

protected:
	//reduce size of the buffer for current length and elements from the start of IE
	template <int DELTA>
	bool calc_buf_len(std::size_t& len_value) const
	{
		if constexpr (DELTA < 0)
		{
			if (len_value < std::size_t(-DELTA))
			{
				CODEC_TRACE("len=%zu less than delta=%+d", len_value, DELTA);
				return false;
			}
		}
		len_value += DELTA;
		auto const state_delta = std::size_t(m_decoder(GET_STATE{}) - m_start);
		CODEC_TRACE("len=%zu state-delta=%zu", len_value, state_delta);
		if (state_delta > len_value)
		{
			CODEC_TRACE("len=%zu less than buffer has been advanced=%zu", len_value, state_delta);
			return false;
		}
		len_value -= state_delta;
		return true;
	}

	template <int DELTA>
	void decode_len_ie(length_type& len_ie)
	{
		decode(m_decoder, len_ie, m_unexp);
		std::size_t len_value = 0;
		value_to_length(len_ie, len_value);
		if (not calc_buf_len<DELTA>(len_value))
		{
			MED_THROW_EXCEPTION(invalid_value, name<IE>(), len_value/*, m_decoder.ctx.buffer()*/);
		}
		m_size_state = m_decoder(PUSH_SIZE{len_value});
		if (not m_size_state)
		{
			MED_THROW_EXCEPTION(overflow, name<IE>(), len_value/*, m_decoder.ctx.buffer()*/);
		}
	}


	FUNC&            m_decoder;
	IE&              m_ie;
	state_type const m_start;
	size_state       m_size_state; //to switch end of buffer for this IE
	UNEXP&           m_unexp;
};

template <class FUNC, class IE, class UNEXP>
struct length_decoder<false, FUNC, IE, UNEXP> : len_dec_impl<FUNC, IE, UNEXP>
{
	using length_type = typename IE::length_type;
	using len_dec_impl<FUNC, IE, UNEXP>::len_dec_impl;
	using len_dec_impl<FUNC, IE, UNEXP>::operator();

	template <int DELTA>
	void operator() (placeholder::_length<DELTA>&)
	{
		CODEC_TRACE("len_dec[%s] %+d by placeholder", name<length_type>(), DELTA);
		length_type len_ie;
		return this->template decode_len_ie<DELTA>(len_ie);
	}
};

template <class FUNC, class IE, class UNEXP>
struct length_decoder<true, FUNC, IE, UNEXP> : len_dec_impl<FUNC, IE, UNEXP>
{
	using length_type = typename IE::length_type;
	using len_dec_impl<FUNC, IE, UNEXP>::len_dec_impl;
	using len_dec_impl<FUNC, IE, UNEXP>::operator();

	//length position by exact type match
	template <class T, class ...ARGS>
	auto operator () (T& len_ie, ARGS&&...) ->
		std::enable_if_t<std::is_same_v<typename T::field_type, length_type>, void>
	{
		CODEC_TRACE("len_dec[%s] by IE", name<length_type>());
		//don't count the size of length IE itself just like with regular LV
		return this->template decode_len_ie<length_type::traits::bits/FUNC::granularity>(len_ie.ref_field());
	}
};

template <class WRAPPER, class FUNC, class IE, class UNEXP>
inline void decode_ie(FUNC& func, IE& ie, CONTAINER const&, UNEXP& unexp)
{
	if constexpr (codec_e::STRUCTURED == get_codec_kind_v<FUNC>)
	{
		func(ie, ENTRY_CONTAINER{});
	}

	if constexpr (is_length_v<IE>)
	{
		if constexpr (has_padding<IE>::value)
		{
			using pad_t = padder<IE, FUNC>;
			pad_t pad{func};
			length_decoder<IE::template has<typename IE::length_type>(), FUNC, IE, UNEXP> ld{ func, ie, unexp };
			ie.decode(ld, unexp);
			//special case for empty elements w/o length placeholder
			pad.enable(static_cast<bool>(ld));
			CODEC_TRACE("%sable padding bits=%zu for len=%zu", ld?"en":"dis", pad.size(), ld.size());
			if constexpr (pad_t::pad_traits::inclusive)
			{
				pad.add();
				ld.restore_end();
			}
			else
			{
				ld.restore_end();
				return pad.add();
			}
			//TODO: treat this case as warning? happens only in case of last IE with padding ended prematuraly
			//if (std::size_t const left = ld.size() * FUNC::granularity - padding_size(pad))
			//MED_THROW_EXCEPTION(overflow, name<IE>(), left);
		}
		else
		{
			length_decoder<IE::template has<typename IE::length_type>(), FUNC, IE, UNEXP> ld{ func, ie, unexp };
			ie.decode(ld, unexp);
			ld.restore_end();
		}

		//TODO: refactor to cover all branches the same way as in encoder
		if constexpr (codec_e::STRUCTURED == get_codec_kind_v<FUNC>)
		{
			func(ie, EXIT_CONTAINER{});
		}
	}
	else
	{
		CODEC_TRACE("%s w/o length...:", name<IE>());
		if constexpr (codec_e::STRUCTURED == get_codec_kind_v<FUNC>)
		{
			ie.decode(func, unexp);
			func(ie, EXIT_CONTAINER{});
		}
		else
		{
			ie.decode(func, unexp);
		}
	}
}

}	//end: namespace sl

template <class FUNC, class IE, class UNEXP = sl::default_handler>
constexpr void decode(FUNC&& func, IE& ie, UNEXP&& unexp = sl::default_handler{})
{
	if constexpr (has_ie_type<IE>::value)
	{
		return sl::decode_ie<IE>(func, ie, typename IE::ie_type{}, unexp);
	}
	else
	{
		return func(ie);
	}
}

}	//end: namespace med
