/**
@file
decoding entry point

@copyright Denis Priyomov 2016
Distributed under the MIT License
(See accompanying file LICENSE or copy at https://opensource.org/licenses/MIT)
*/

#pragma once

#include <utility>

#include "ie.hpp"
#include "error.hpp"
#include "optional.hpp"
#include "state.hpp"
#include "padding.hpp"
#include "debug.hpp"
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
	constexpr bool operator()(FUNC& func, IE const&, HEADER const& header) const
	{
		func(error::INCORRECT_TAG, name<IE>(), get_tag(header));
		return false;
	}
};


template <class FUNC, class IE, class IE_TYPE>
std::enable_if_t<is_read_only_v<IE>, bool>
inline decode_primitive(FUNC& func, IE& ie, IE_TYPE const& ie_type)
{
	if (func(PUSH_STATE{}))
	{
		if (func(ie, ie_type))
		{
			func(POP_STATE{});
			return true;
		}
	}
	return false;
};

template <class FUNC, class IE, class IE_TYPE>
std::enable_if_t<is_skip_v<IE>, bool>
inline decode_primitive(FUNC& func, IE& ie, IE_TYPE const& ie_type)
{
	//TODO: need to support fixed octet_string here?
	return func.advance(IE::traits::bits);
};

template <class FUNC, class IE, class IE_TYPE>
std::enable_if_t<!is_read_only_v<IE> && !is_skip_v<IE>, bool>
inline decode_primitive(FUNC& func, IE& ie, IE_TYPE const& ie_type)
{
	return func(ie, ie_type);
};

template <class TAG, class FUNC>
inline auto decode_tag(FUNC& func)
{
	make_value_t<TAG> tag;
	decode_primitive(func, tag, IE_VALUE{});
	return tag;
}


template <class WRAPPER, class FUNC, class IE, class UNEXP>
constexpr bool decode_ie(FUNC&, IE& ie, IE_NULL const&, UNEXP&)
{
	CODEC_TRACE("NULL %s", name<IE>());
	ie.set();
	return true; //do nothing
};

template <class WRAPPER, class FUNC, class IE, class UNEXP>
inline bool decode_ie(FUNC& func, IE& ie, PRIMITIVE const&, UNEXP&)
{
	CODEC_TRACE("PRIMITIVE %s", name<IE>());
	return decode_primitive(func, ie, typename WRAPPER::ie_type{});
}

//Tag-Value
template <class WRAPPER, class FUNC, class IE, class UNEXP>
std::enable_if_t<!is_optional_v<IE>, bool>
inline decode_ie(FUNC& func, IE& ie, IE_TV const&, UNEXP& unexp)
{
	CODEC_TRACE("TV %s", name<IE>());
	if (auto const tag = decode_tag<typename WRAPPER::tag_type>(func))
	{
		if (WRAPPER::tag_type::match(tag.get_encoded()))
		{
			CODEC_TRACE("TV[%zu : %s]", tag.get(), name<typename WRAPPER::field_type>());
			return decode(func, ref_field(ie), unexp);
		}
		//NOTE: this can only be called for mandatory field thus it's fail case (not unexpected)
		func(error::INCORRECT_TAG, name<typename WRAPPER::field_type>(), tag.get_encoded());
	}
	return false;
}

//Length-Value
template <class WRAPPER, class FUNC, class IE, class UNEXP>
inline bool decode_ie(FUNC& func, IE& ie, IE_LV const&, UNEXP& unexp)
{
	typename WRAPPER::length_type len_ie;
	CODEC_TRACE("LV[%s]", name<IE>());
	if (decode(func, len_ie, unexp))
	{
		std::size_t len_value = len_ie.get_encoded();
		if (value_to_length(func, len_ie, len_value))
		{
			//CODEC_TRACE("LV[%zu] : %s", len, name<IE>());
			auto end = func(PUSH_SIZE{len_value});
			if (decode(func, ref_field(ie), unexp))
			{
				if (0 == end.size()) return true;
				//TODO: ??? as warning not error
				func(error::OVERFLOW, name<typename WRAPPER::field_type>(), end.size() * FUNC::granularity);
			}
		}
	}
	return false;
}


template <class WRAPPER, class FUNC, class IE, class UNEXP>
std::enable_if_t<!has_length_type<IE>::value, bool>
inline decode_ie(FUNC& func, IE& ie, CONTAINER const&, UNEXP& unexp)
{
	CODEC_TRACE("%s w/o length...:", name<IE>());
	return ie.decode(func, unexp);
}

template <class FUNC, class IE, class UNEXP>
struct length_decoder
{
	using state_type = typename FUNC::state_type;
	using size_state = typename FUNC::size_state;
	using allocator_type = typename FUNC::allocator_type;
	enum : std::size_t { granularity = FUNC::granularity };

	using length_type = typename IE::length_type;

	length_decoder(FUNC& decoder, IE& ie, UNEXP& unexp) noexcept
		: m_decoder{ decoder }
		, m_ie{ ie }
		, m_start{ m_decoder(GET_STATE{}) }
		, m_unexp{ unexp }
	{
		CODEC_TRACE("start %s with length...:", name<IE>());
	}

#ifdef CODEC_TRACE_ENABLE
	~length_decoder()
	{
		CODEC_TRACE("finish %s with length...:", name<IE>());
	}
#endif

	template <int DELTA>
	bool operator() (placeholder::_length<DELTA>&)
	{
		length_type len_ie;
		if (decode(m_decoder, len_ie, m_unexp))
		{
			//reduced size of the input buffer for current length and elements from the start of IE
			std::size_t len_value = len_ie.get_encoded();
			if (value_to_length(m_decoder, len_ie, len_value))
			{
				typename length_type::value_type const size = (len_value + DELTA) - (m_decoder(GET_STATE{}) - m_start);
				CODEC_TRACE("size(%zu)=length(%zu) + %d - %d", size, len_value, DELTA, (m_decoder(GET_STATE{}) - m_start));
				m_size_state = m_decoder(PUSH_SIZE{size});
				if (m_size_state) { return true; }
				m_decoder(error::OVERFLOW, name<get_field_type_t<IE>>(), m_size_state.size() * FUNC::granularity, size * FUNC::granularity);
			}
		}
		return false;

	}

	//check if placeholder was visited
	explicit operator bool() const                     { return static_cast<bool>(m_size_state); }
	auto size() const                                  { return m_size_state.size(); }

	template <class ...T>
	auto operator() (T&&... args)                      { return m_decoder(std::forward<T>(args)...); }

	allocator_type& get_allocator()                    { return m_decoder.get_allocator(); }

	FUNC&            m_decoder;
	IE&              m_ie;
	state_type const m_start;
	size_state       m_size_state; //to switch end of buffer for this IE
	UNEXP&           m_unexp;
};

template <class WRAPPER, class FUNC, class IE, class UNEXP>
std::enable_if_t<has_length_type<IE>::value, bool>
inline decode_ie(FUNC& func, IE& ie, CONTAINER const&, UNEXP& unexp)
{
	auto const pad = add_padding<IE>(func);
	{
		length_decoder<FUNC, IE, UNEXP> ld{ func, ie, unexp };
		if (!ie.decode(ld, unexp)) return false;
		//special case for empty elements w/o length placeholder
		padding_enable(pad, static_cast<bool>(ld));
		CODEC_TRACE("%sable padding bits=%zu for len=%zu", ld?"en":"dis", padding_size(pad), ld.size());
		if (std::size_t const left = ld.size() * FUNC::granularity - padding_size(pad))
		{
			func(error::OVERFLOW, name<IE>(), left);
			return false;
		}
	}
	return static_cast<bool>(pad);
}

}	//end: namespace sl

template <class FUNC, class IE, class UNEXP = sl::default_handler>
std::enable_if_t<has_ie_type<IE>::value, bool>
inline decode(FUNC&& func, IE& ie, UNEXP&& unexp = sl::default_handler{})
{
	return sl::decode_ie<IE>(func, ie, typename IE::ie_type{}, unexp);
}

template <class FUNC, class IE, class UNEXP>
std::enable_if_t<!has_ie_type<IE>::value, bool>
inline decode(FUNC& func, IE& ie, UNEXP&)
{
	return func(ie);
}

}	//end: namespace med
