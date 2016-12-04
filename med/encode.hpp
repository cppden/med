/*!
@file
TODO: define.

@copyright Denis Priyomov 2016
Distributed under the MIT License
(See accompanying file LICENSE or copy at https://opensource.org/licenses/MIT)
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

namespace detail {

template <class T, typename Enable = void>
struct null_encoder
{
	template <class FUNC, class IE>
	static constexpr bool encode(FUNC& func, IE& ie)
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

	template <int DELTA>
	bool operator() (placeholder::_length<DELTA> const&)
	{
		m_delta = DELTA;
		m_snapshot = m_encoder(GET_STATE{}); //save position in encoded buffer to update with correct length
		return m_encoder(ADVANCE_STATE{int(length_type::traits::bits)});
	}

	~length_encoder()
	{
		//update the length with final value
		if (m_snapshot)
		{
			state_type const end = m_encoder(GET_STATE{});

			length_type true_len;
			true_len.set(end - m_start - m_delta);
			CODEC_TRACE("LENGTH stop: len=%zu(%+d)", true_len.get(), m_delta);

			m_encoder(SET_STATE{}, m_snapshot);
			encode(m_encoder, true_len);
			m_encoder(SET_STATE{}, end);
		}
	}

	template <class ...T>
	auto operator() (T&&... args)  { return m_encoder(std::forward<T>(args)...); }

	FUNC&            m_encoder;
	int              m_delta;
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
		CODEC_TRACE("%s with length...:", name<IE>());
		auto const pad = add_padding<IE>(func);
		if (ie.encode(length_encoder<FUNC, typename IE::length_type>{ func }))
		{
			return static_cast<bool>(pad);
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

}	//end: namespace detail

//IE_NULL
template <class WRAPPER, class FUNC, class IE>
constexpr bool encode(FUNC& func, IE& ie, IE_NULL const&)
{
	return detail::null_encoder<FUNC>::encode(func, ie);
};

template <class WRAPPER, class FUNC, class IE>
inline bool encode(FUNC& func, IE& ie, CONTAINER const&)
{
	return detail::container_encoder<FUNC>::encode(func, ie);
}


template <class WRAPPER, class FUNC, class IE>
inline bool encode(FUNC& func, IE& ie, PRIMITIVE const&)
{
	return detail::encode_primitive(func, ie, typename WRAPPER::ie_type{});
}

template <class WRAPPER, class FUNC, class IE>
inline bool encode(FUNC& func, IE& ie, IE_LV const&)
{
	typename WRAPPER::length_type len_ie{};
	std::size_t len_value = get_length(ref_field(ie));
	if (length_to_value(len_ie, len_value))
	{
		CODEC_TRACE("L=%zx[%s]:", len_ie.get(), name<IE>());
		return encode(func, len_ie)
			&& encode(func, ref_field(ie));
	}
	else
	{
		func(error::INCORRECT_VALUE, name<typename WRAPPER::length_type>(), len_value);
	}
	return false;
}

template <class WRAPPER, class FUNC, class IE>
inline bool encode(FUNC& func, IE& ie, IE_TV const&)
{
	typename WRAPPER::tag_type const tag_ie{};
	CODEC_TRACE("T%zx<%s>{%s}", tag_ie.get(), name<WRAPPER>(), name<IE>());
	return encode(func, tag_ie)
		&& encode<typename WRAPPER::field_type>(func, ref_field(ie), typename WRAPPER::field_type::ie_type{});
}

template <class FUNC, class IE>
constexpr int invoke_encode(FUNC&, IE&, int count) { return count; }

template <class FUNC, class IE, std::size_t INDEX, std::size_t... Is>
inline int invoke_encode(FUNC& func, IE& ie, int count)
{
	if (ie.template ref_field<INDEX>().is_set())
	{
		CODEC_TRACE("[%s]@%zu", name<IE>(), INDEX);
		if (!sl::encode<IE>(func, ie.template ref_field<INDEX>(), typename IE::ie_type{})) return -1;
		return invoke_encode<FUNC, IE, Is...>(func, ie, INDEX+1);
	}
	return INDEX;
}

template<class FUNC, class IE, std::size_t... Is>
inline int repeat_encode_impl(FUNC& func, IE& ie, std::index_sequence<Is...>)
{
	return invoke_encode<FUNC, IE, Is...>(func, ie, 0);
}

}	//end: namespace sl

template <class FUNC, class IE>
std::enable_if_t<has_ie_type<IE>::value, bool>
inline encode(FUNC&& func, IE& ie)
{
	return sl::encode<IE>(func, ie, typename IE::ie_type{});
}

template <class FUNC, class IE>
std::enable_if_t<!has_ie_type<IE>::value, bool>
inline encode(FUNC& func, IE& ie)
{
	return func(ie);
}

template <std::size_t N, class FUNC, class IE>
inline int encode(FUNC& func, IE& ie)
{
	//CODEC_TRACE("%s *%zu", name<IE>(), N);
	return sl::repeat_encode_impl(func, ie, std::make_index_sequence<N>{});
}

}	//end: namespace med
