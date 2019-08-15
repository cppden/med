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
#include "meta/typelist.hpp"


namespace med {

//structure layer
namespace sl {

struct default_handler
{
	template <class DECODER, class IE, class HEADER>
	constexpr void operator()(DECODER&, IE const&, HEADER const& header) const
	{
		MED_THROW_EXCEPTION(unknown_tag, name<IE>(), get_tag(header))
	}
};

//template <class WRAPPER, class DECODER, class IE, class UNEXP>
//constexpr void decode_ie(DECODER& decoder, IE& ie, IE_NULL, UNEXP&)
//{
//	decoder(ie, typename WRAPPER::ie_type{});
//}

//Tag
//PEEK_SAVE - to preserve state in case of peek tag or not
template <class TAG_TYPE, bool PEEK_SAVE, class DECODER>
inline std::size_t decode_tag(DECODER& decoder)
{
	TAG_TYPE ie;
	std::size_t value{};
	if constexpr (PEEK_SAVE && is_peek_v<TAG_TYPE>)
	{
		CODEC_TRACE("PEEK TAG %s", name<TAG_TYPE>());
		if (decoder(PUSH_STATE{}, ie))
		{
			value = decoder(ie, IE_TAG{});
			decoder(POP_STATE{});
		}
	}
	else
	{
		value = decoder(ie, IE_TAG{});
	}
	CODEC_TRACE("TAG=%zxh [%s]", value, name<TAG_TYPE>());
	return value;
}

template <class WRAPPER, class DECODER, class IE, class UNEXP>
constexpr void decode_ie(DECODER& decoder, IE& ie, PRIMITIVE, UNEXP&)
{
	if constexpr (is_peek_v<IE>)
	{
		CODEC_TRACE("PEEK PRIMITIVE %s[%s(%s)]", __FUNCTION__, name<WRAPPER>(), name<IE>());
		if (decoder(PUSH_STATE{}, ie))
		{
			decoder(ie, typename WRAPPER::ie_type{});
			decoder(POP_STATE{});
		}
	}
	else
	{
		using tag_type = typename meta::unwrap_t<decltype(DECODER::template get_tag_type<WRAPPER>())>;
		if constexpr (not std::is_void_v<tag_type>)
		{
			auto const tag = decode_tag<tag_type, true>(decoder);
			if (not tag_type::match(tag))
			{
				//NOTE: this can only be called for mandatory field thus it's fail case (not unexpected)
				MED_THROW_EXCEPTION(unknown_tag, name<WRAPPER>(), tag)
			}
			//CODEC_TRACE("T=%zxh V[%s]", std::size_t(tag), name<WRAPPER>());
		}
		CODEC_TRACE("PRIMITIVE %s[%s(%s)]", __FUNCTION__, name<WRAPPER>(), name<IE>());
		decoder(ie, typename WRAPPER::ie_type{});
	}
}

//Tag-Value
template <class WRAPPER, class DECODER, class IE, class UNEXP>
inline void decode_ie(DECODER& decoder, IE& ie, IE_TV, UNEXP& unexp)
{
	CODEC_TRACE("TV[%s]", name<WRAPPER>());
	using tag_type = typename meta::unwrap_t<decltype(DECODER::template get_tag_type<IE>())>;
	auto const tag = decode_tag<tag_type, true>(decoder);
	if (tag_type::match(tag))
	{
		CODEC_TRACE("T=%zxh V[%s]", std::size_t(tag), name<WRAPPER>());
		return decode(decoder, ref_field(ie), unexp);
	}
	//NOTE: this can only be called for mandatory field thus it's fail case (not unexpected)
	MED_THROW_EXCEPTION(unknown_tag, name<WRAPPER>(), tag)
}

//Length
template <class LEN_TYPE, class DECODER>
inline std::size_t decode_len(DECODER& decoder)
{
	LEN_TYPE ie;
	if constexpr (is_peek_v<LEN_TYPE>)
	{
		CODEC_TRACE("PEEK LEN %s", name<LEN_TYPE>());
		if (decoder(PUSH_STATE{}, ie))
		{
			decoder(ie, IE_LEN{});
			decoder(POP_STATE{});
		}
	}
	else
	{
		decoder(ie, IE_LEN{});
	}
	std::size_t value = ie.get_encoded();
	CODEC_TRACE("LEN[%s]=%zxh", name<LEN_TYPE>(), value);
	value_to_length(ie, value);
	return value;
}
//Length-Value
template <class WRAPPER, class DECODER, class IE, class UNEXP>
inline void decode_ie(DECODER& decoder, IE& ie, IE_LV, UNEXP& unexp)
{
	CODEC_TRACE("LV[%s]", name<WRAPPER>());
	using len_type = typename WRAPPER::length_type;
	auto const len_value = decode_len<len_type>(decoder);
	if (auto end = decoder(PUSH_SIZE{len_value}))
	{
		decode(decoder, ref_field(ie), unexp);
		//TODO: ??? as warning not error
		if (0 != end.size())
		{
			CODEC_TRACE("%s: end-size=%zu", name<IE>(), end.size());
			MED_THROW_EXCEPTION(overflow, name<WRAPPER>(), len_value)
		}
	}
	else
	{
		//TODO: something more informative: tried to set length beyond data end
		MED_THROW_EXCEPTION(overflow, name<WRAPPER>(), len_value)
	}
}

template <bool BY_IE, class DECODER, class IE, class UNEXP>
struct length_decoder;

template <class DECODER, class IE, class UNEXP>
struct len_dec_impl
{
	using state_type = typename DECODER::state_type;
	using size_state = typename DECODER::size_state;
	using allocator_type = typename DECODER::allocator_type;

	using length_type = typename IE::length_type;

	len_dec_impl(DECODER& decoder, IE& ie, UNEXP& unexp) noexcept
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

	//forward regular types to decoder
	template <class... Args> //NOTE: decltype is needed to expose actually defined operators
	auto operator() (Args&&... args) -> decltype(std::declval<DECODER>()(std::forward<Args>(args)...))
	{
		return m_decoder(std::forward<Args>(args)...);
	}

	template <class T>
	static constexpr auto get_tag_type()               { return DECODER::template get_tag_type<T>(); }

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
		CODEC_TRACE("len<%d>=%zu state-delta=%zu", DELTA, len_value, state_delta);
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
		if (calc_buf_len<DELTA>(len_value))
		{
			m_size_state = m_decoder(PUSH_SIZE{len_value});
			if (not m_size_state)
			{
				MED_THROW_EXCEPTION(overflow, name<IE>(), len_value/*, m_decoder.ctx.buffer()*/)
			}
		}
		else
		{
			MED_THROW_EXCEPTION(invalid_value, name<IE>(), len_value/*, m_decoder.ctx.buffer()*/)
		}
	}


	DECODER&         m_decoder;
	IE&              m_ie;
	state_type const m_start;
	size_state       m_size_state; //to switch end of buffer for this IE
	UNEXP&           m_unexp;
};

template <class DECODER, class IE, class UNEXP>
struct length_decoder<false, DECODER, IE, UNEXP> : len_dec_impl<DECODER, IE, UNEXP>
{
	using length_type = typename IE::length_type;
	using len_dec_impl<DECODER, IE, UNEXP>::len_dec_impl;
	using len_dec_impl<DECODER, IE, UNEXP>::operator();

	template <int DELTA>
	void operator() (placeholder::_length<DELTA>&)
	{
		CODEC_TRACE("len_dec[%s] %+d by placeholder", name<length_type>(), DELTA);
		length_type len_ie;
		return this->template decode_len_ie<DELTA>(len_ie);
	}
};

template <class DECODER, class IE, class UNEXP>
struct length_decoder<true, DECODER, IE, UNEXP> : len_dec_impl<DECODER, IE, UNEXP>
{
	using length_type = typename IE::length_type;
	using len_dec_impl<DECODER, IE, UNEXP>::len_dec_impl;
	using len_dec_impl<DECODER, IE, UNEXP>::operator();

	//length position by exact type match
	template <class T, class ...ARGS>
	auto operator () (T& len_ie, ARGS&&...) ->
		std::enable_if_t<std::is_same_v<typename T::field_type, length_type>, void>
	{
		CODEC_TRACE("len_dec[%s] by IE", name<length_type>());
		//don't count the size of length IE itself just like with regular LV
		constexpr auto len_ie_size = +DECODER::template size_of<length_type>();
		return this->template decode_len_ie<len_ie_size>(len_ie.ref_field());
	}
};

template <class WRAPPER, class DECODER, class IE, class UNEXP>
inline void decode_ie(DECODER& decoder, IE& ie, CONTAINER, UNEXP& unexp)
{
	call_if<is_callable_with_v<DECODER, ENTRY_CONTAINER>>::call(decoder, ENTRY_CONTAINER{}, ie);

	if constexpr (is_length_v<IE>)
	{
		if constexpr (has_padding<IE>::value)
		{
			using pad_t = padder<IE, DECODER>;
			pad_t pad{decoder};
			length_decoder<IE::template has<typename IE::length_type>(), DECODER, IE, UNEXP> ld{ decoder, ie, unexp };
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
			//if (std::size_t const left = ld.size() - padding_size(pad))
			//MED_THROW_EXCEPTION(overflow, name<IE>(), left)
		}
		else
		{
			length_decoder<IE::template has<typename IE::length_type>(), DECODER, IE, UNEXP> ld{ decoder, ie, unexp };
			ie.decode(ld, unexp);
			ld.restore_end();
		}
	}
	else
	{
		CODEC_TRACE("%s w/o length...:", name<IE>());
		ie.decode(decoder, unexp);
	}

	call_if<is_callable_with_v<DECODER, EXIT_CONTAINER>>::call(decoder, EXIT_CONTAINER{}, ie);
}

}	//end: namespace sl

template <class DECODER, class IE, class UNEXP = sl::default_handler>
constexpr void decode(DECODER&& decoder, IE& ie, UNEXP&& unexp = sl::default_handler{})
{
	if constexpr (has_ie_type<IE>::value)
	{
		return sl::decode_ie<IE>(decoder, ie, typename IE::ie_type{}, unexp);
	}
	else //special cases like passing a placeholder
	{
		return decoder(ie);
	}
}

}	//end: namespace med
