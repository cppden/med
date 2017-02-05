/**
@file
encoding entry point

@copyright Denis Priyomov 2016-2017
Distributed under the MIT License
(See accompanying file LICENSE or visit https://github.com/cppden/med)
*/

#pragma once

#include <utility>

#include "ie.hpp"
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
	static constexpr bool encode(FUNC&, IE&)
	{
		return true; //do nothing
	}
};

template <class T>
struct null_encoder<T, void_t<typename T::null_encoder>>
{
	template <class FUNC, class IE>
	static bool encode(FUNC& func, IE& ie)
	{
		return typename T::null_encoder{}(func, ie);
	}
};


template <class FUNC, class IE, class IE_TYPE>
std::enable_if_t<is_read_only_v<IE>, bool>
constexpr encode_primitive(FUNC&, IE&, IE_TYPE const&)
{
	return true; //do nothing
};

template <class FUNC, class IE, class IE_TYPE>
std::enable_if_t<!is_read_only_v<IE>, bool>
inline encode_primitive(FUNC& func, IE& ie, IE_TYPE const& ie_type)
{
	snapshot(func, ie);
	return func(ie, ie_type);
};

template <class FUNC, class LEN>
struct length_encoder
{
	using state_type = typename FUNC::state_type;
	using length_type = LEN;
	enum : std::size_t { granularity = FUNC::granularity };

	explicit length_encoder(FUNC& encoder) noexcept
		: m_encoder{ encoder }
		, m_start{ m_encoder(GET_STATE{}) }
	{
	}

	~length_encoder()
	{
		//update the length with final value
		if (m_snapshot)
		{
			state_type const end = m_encoder(GET_STATE{});

			std::size_t len_value = end - m_start - m_delta;
			CODEC_TRACE("LENGTH stop: len=%zu(%+d)", len_value, m_delta);

			length_type len_ie;
			if (length_to_value(m_encoder, len_ie, len_value))
			{
				CODEC_TRACE("L=%zx[%s]:", len_ie.get(), name<length_type>());
				m_encoder(SET_STATE{}, m_snapshot);
				encode(m_encoder, len_ie);
				m_encoder(SET_STATE{}, end);
			}
		}
	}

	template <int DELTA>
	bool operator() (placeholder::_length<DELTA> const&)
	{
		m_delta = DELTA;
		m_snapshot = m_encoder(GET_STATE{}); //save position in encoded buffer to update with correct length
		return m_encoder(ADVANCE_STATE{int(length_type::traits::bits)});
	}

	//check if placeholder was visited
	explicit operator bool() const                     { return static_cast<bool>(m_snapshot); }

	template <class ...T>
	auto operator() (T&&... args)  { return m_encoder(std::forward<T>(args)...); }

	FUNC&            m_encoder;
	int              m_delta {0};
	state_type const m_start;
	state_type       m_snapshot{ };
};

template <class T, typename Enable = void>
struct container_encoder
{
	template <class FUNC, class IE>
	std::enable_if_t<!has_length_type<IE>::value, bool>
	static encode(FUNC& func, IE& ie)
	{
		CODEC_TRACE("%s...:", name<IE>());
		return ie.encode(func);
	}

	template <class FUNC, class IE>
	std::enable_if_t<has_length_type<IE>::value, bool>
	static encode(FUNC& func, IE& ie)
	{
		auto const pad = add_padding<IE>(func);
		{
			CODEC_TRACE("start %s with length...:", name<IE>());
			length_encoder<FUNC, typename IE::length_type> le{ func };
			if (ie.encode(le))
			{
				CODEC_TRACE("finish %s with length...:", name<IE>());
				//special case for empty elements w/o length placeholder
				padding_enable(pad, static_cast<bool>(le));
				return static_cast<bool>(pad);
			}
		}
		return false;
	}
};

template <class T>
struct container_encoder<T, void_t<typename T::container_encoder>>
{
	template <class FUNC, class IE>
	static bool encode(FUNC& func, IE& ie)
	{
		return typename T::container_encoder{}(func, ie);
	}
};


//IE_NULL
template <class WRAPPER, class FUNC, class IE>
constexpr bool encode_ie(FUNC& func, IE& ie, IE_NULL const&)
{
	return null_encoder<FUNC>::encode(func, ie);
};

template <class WRAPPER, class FUNC, class IE>
inline bool encode_ie(FUNC& func, IE& ie, CONTAINER const&)
{
	return container_encoder<FUNC>::encode(func, ie);
}


template <class WRAPPER, class FUNC, class IE>
inline bool encode_ie(FUNC& func, IE& ie, PRIMITIVE const&)
{
	return encode_primitive(func, ie, typename WRAPPER::ie_type{});
}

template <class WRAPPER, class FUNC, class IE>
inline bool encode_ie(FUNC& func, IE& ie, IE_LV const&)
{
	typename WRAPPER::length_type len_ie{};
	std::size_t len_value = get_length(ref_field(ie));
	if (length_to_value(func, len_ie, len_value))
	{
		CODEC_TRACE("L=%zx[%s]:", len_ie.get(), name<IE>());
		return encode(func, len_ie)
			&& encode(func, ref_field(ie));
	}
	return false;
}

template <class WRAPPER, class FUNC, class IE>
inline bool encode_ie(FUNC& func, IE& ie, IE_TV const&)
{
	typename WRAPPER::tag_type const tag_ie{};
	CODEC_TRACE("T%zx<%s>{%s}", tag_ie.get(), name<WRAPPER>(), name<IE>());
	return encode(func, tag_ie)
		&& encode_ie<typename WRAPPER::field_type>(func, ref_field(ie), typename WRAPPER::field_type::ie_type{});
}

}	//end: namespace sl

template <class FUNC, class IE>
std::enable_if_t<has_ie_type_v<IE>, bool>
inline encode(FUNC&& func, IE& ie)
{
	return sl::encode_ie<IE>(func, ie, typename IE::ie_type{});
}

template <class FUNC, class IE>
std::enable_if_t<!has_ie_type_v<IE>, bool>
inline encode(FUNC&& func, IE& ie)
{
	return func(ie);
}


}	//end: namespace med
