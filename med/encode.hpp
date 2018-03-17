/**
@file
encoding entry point

@copyright Denis Priyomov 2016-2017
Distributed under the MIT License
(See accompanying file LICENSE or visit https://github.com/cppden/med)
*/

#pragma once

#include <utility>

#include "config.hpp"
#include "error.hpp"
#include "optional.hpp"
#include "snapshot.hpp"
#include "padding.hpp"
#include "debug.hpp"
#include "length.hpp"
#include "name.hpp"
#include "placeholder.hpp"


namespace med {

//structure layer
namespace sl {

template <class T, typename Enable = void>
struct null_encoder
{
	template <class FUNC, class IE>
	static constexpr MED_RESULT encode(FUNC&, IE&)
	{
		MED_RETURN_SUCCESS; //do nothing
	}
};

template <class T>
struct null_encoder<T, std::void_t<typename T::null_encoder>>
{
	template <class FUNC, class IE>
	static MED_RESULT encode(FUNC& func, IE& ie)
	{
		return typename T::null_encoder{}(func, ie);
	}
};

template <class FUNC, class LEN>
struct length_encoder
{
	using state_type = typename FUNC::state_type;
	using length_type = LEN;
	static constexpr std::size_t granularity = FUNC::granularity;

	explicit length_encoder(FUNC& encoder) noexcept
		: m_encoder{ encoder }
		, m_start{ m_encoder(GET_STATE{}) }
	{
	}

	//~length_encoder()                  { set_length_ie(); }
	//update the length with final value
	void set_length_ie()
	{
		if (m_lenpos)
		{
			state_type const end = m_encoder(GET_STATE{});

			std::size_t len_value = end - m_start - m_delta;
			CODEC_TRACE("LENGTH stop: len=%zu(%+d)", len_value, m_delta);

			length_type len_ie;
#if (MED_EXCEPTIONS)
			length_to_value(m_encoder, len_ie, len_value);
			CODEC_TRACE("L=%zx[%s]:", std::size_t(len_ie.get()), name<length_type>());
			m_encoder(SET_STATE{}, m_lenpos);
			encode(m_encoder, len_ie);
			m_encoder(SET_STATE{}, end);
#else
			if (length_to_value(m_encoder, len_ie, len_value))
			{
				CODEC_TRACE("L=%zx[%s]:", std::size_t(len_ie.get()), name<length_type>());
				m_encoder(SET_STATE{}, m_lenpos);
				encode(m_encoder, len_ie);
				m_encoder(SET_STATE{}, end);
			}
#endif //MED_EXCEPTIONS
		}
	}

	template <int DELTA>
	MED_RESULT operator() (placeholder::_length<DELTA> const&)
	{
		m_delta = DELTA;
		m_lenpos = m_encoder(GET_STATE{}); //save position in encoded buffer to update with correct length
		return m_encoder(ADVANCE_STATE{int(length_type::traits::bits)});
	}

	//check if placeholder was visited
	explicit operator bool() const                       { return static_cast<bool>(m_lenpos); }

	template <class ...T>
	auto operator() (T&&... args)                        { return m_encoder(std::forward<T>(args)...); }

	FUNC&            m_encoder;
	int              m_delta {0};
	state_type const m_start;
	state_type       m_lenpos { };
};

template <class T, typename Enable = void>
struct container_encoder
{
	template <class FUNC, class IE>
	static constexpr MED_RESULT encode(FUNC& func, IE& ie)
	{
		if constexpr (has_length_type<IE>::value)
		{
			CODEC_TRACE("start %s with length...:", name<IE>());
			if constexpr (has_padding<IE>::value)
			{
				using pad_t = padder<IE, FUNC>;
				pad_t pad{func};
				length_encoder<FUNC, typename IE::length_type> le{ func };
				MED_CHECK_FAIL(ie.encode(le));
				CODEC_TRACE("finish %s with length...:", name<IE>());
				//special case for empty elements w/o length placeholder
				pad.enable(static_cast<bool>(le));
				if constexpr (pad_t::pad_traits::inclusive)
				{
					MED_CHECK_FAIL(pad.add());
					le.set_length_ie();
					MED_RETURN_SUCCESS;
				}
				else
				{
					le.set_length_ie();
					return pad.add();
				}
			}
			else
			{
				length_encoder<FUNC, typename IE::length_type> le{ func };
				MED_CHECK_FAIL(ie.encode(le));
				le.set_length_ie();
				CODEC_TRACE("finish %s with length...:", name<IE>());
				MED_RETURN_SUCCESS;
			}
		}
		else
		{
			CODEC_TRACE("%s...:", name<IE>());
			return ie.encode(func);
		}
	}
};

template <class T>
struct container_encoder<T, std::void_t<typename T::container_encoder>>
{
	template <class FUNC, class IE>
	static MED_RESULT encode(FUNC& func, IE& ie)
	{
		return typename T::container_encoder{}(func, ie);
	}
};


//IE_NULL
template <class WRAPPER, class FUNC, class IE>
constexpr MED_RESULT encode_ie(FUNC& func, IE& ie, IE_NULL const&)
{
	return null_encoder<FUNC>::encode(func, ie);
}

template <class WRAPPER, class FUNC, class IE>
inline MED_RESULT encode_ie(FUNC& func, IE& ie, CONTAINER const&)
{
	return container_encoder<FUNC>::encode(func, ie);
}

template <class WRAPPER, class FUNC, class IE>
inline MED_RESULT encode_ie(FUNC& func, IE& ie, PRIMITIVE const&)
{
	if constexpr (is_peek_v<IE>)
	{
		MED_RETURN_SUCCESS; //do nothing
	}
	else
	{
		snapshot(func, ie);
		return func(ie, typename WRAPPER::ie_type{});
	}
}

template <class WRAPPER, class FUNC, class IE>
inline MED_RESULT encode_ie(FUNC& func, IE& ie, IE_LV const&)
{
	typename WRAPPER::length_type len_ie{};
	std::size_t len_value = get_length(ref_field(ie));
	CODEC_TRACE("L=%zx[%s]:", std::size_t(len_ie.get()), name<IE>());
	return length_to_value(func, len_ie, len_value)
		MED_AND encode(func, len_ie)
		MED_AND encode(func, ref_field(ie));
}

template <class WRAPPER, class FUNC, class IE>
inline MED_RESULT encode_ie(FUNC& func, IE& ie, IE_TV const&)
{
	typename WRAPPER::tag_type const tag_ie{};
	CODEC_TRACE("T%zx<%s>{%s}", std::size_t(tag_ie.get()), name<WRAPPER>(), name<IE>());
	return encode(func, tag_ie)
		MED_AND encode_ie<typename WRAPPER::field_type>(func, ref_field(ie), typename WRAPPER::field_type::ie_type{});
}

}	//end: namespace sl

template <class FUNC, class IE>
constexpr MED_RESULT encode(FUNC&& func, IE& ie)
{
	if constexpr (has_ie_type_v<IE>)
	{
		return sl::encode_ie<IE>(func, ie, typename IE::ie_type{});
	}
	else
	{
		return func(ie);
	}
}

}	//end: namespace med
