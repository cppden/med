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
	static constexpr MED_RESULT encode(FUNC&, IE const&)
	{
		MED_RETURN_SUCCESS; //do nothing
	}
};

template <class T>
struct null_encoder<T, std::void_t<typename T::null_encoder>>
{
	template <class FUNC, class IE>
	static MED_RESULT encode(FUNC& func, IE const& ie)
	{
		return typename T::null_encoder{}(func, ie);
	}
};

template <bool BY_IE, class FUNC, class LEN>
struct length_encoder;

template <class FUNC, class LEN>
struct len_enc_impl
{
	using state_type = typename FUNC::state_type;
	using length_type = LEN;
	static constexpr std::size_t granularity = FUNC::granularity;

	explicit len_enc_impl(FUNC& encoder) noexcept
		: m_encoder{ encoder }
		, m_start{ m_encoder(GET_STATE{}) }
	{
	}

#ifdef CODEC_TRACE_ENABLE
	~len_enc_impl() noexcept
	{
		CODEC_TRACE("finish len_type=%s...:", name<length_type>());
	}
#endif

	//check if placeholder was visited
	explicit operator bool() const                       { return static_cast<bool>(m_lenpos); }

	//forward regular types to encoder
	template <class ...T>
	auto operator() (T&&... args)
	{
		//CODEC_TRACE("%s", __PRETTY_FUNCTION__);
		return m_encoder(std::forward<T>(args)...);
	}

protected:
	FUNC&            m_encoder;
	state_type const m_start;
	state_type       m_lenpos { };
};


//by placeholder
template <class FUNC, class LEN>
struct length_encoder<false, FUNC, LEN> : len_enc_impl<FUNC, LEN>
{
	using length_type = LEN;
	using len_enc_impl<FUNC, LEN>::len_enc_impl;
	using len_enc_impl<FUNC, LEN>::operator();

	//length position by placeholder
	template <int DELTA>
	auto operator() (placeholder::_length<DELTA> const&)
	{
		CODEC_TRACE("len_enc[%s] %+d by placeholder", name<length_type>(), DELTA);
		m_delta = DELTA;
		//save position in encoded buffer to update with correct length
		this->m_lenpos = this->m_encoder(GET_STATE{});
		return this->m_encoder(ADVANCE_STATE{int(length_type::traits::bits)});
	}

	//update the length with final value
	MED_RESULT set_length_ie()
	{
		if (this->m_lenpos)
		{
			auto const end = this->m_encoder(GET_STATE{});
			std::size_t len_value = end - this->m_start - m_delta;
			//CODEC_TRACE("LENGTH stop: len=%zu(%+d)", len_value, m_delta);
			length_type len_ie;
			MED_CHECK_FAIL(length_to_value(this->m_encoder, len_ie, len_value));
			this->m_encoder(SET_STATE{}, this->m_lenpos);
			MED_CHECK_FAIL(encode(this->m_encoder, len_ie));
			this->m_encoder(SET_STATE{}, end);
		}
		MED_RETURN_SUCCESS;
	}

private:
	int m_delta {0};
};

//by exact length type
template <class FUNC, class LEN>
struct length_encoder<true, FUNC, LEN> : len_enc_impl<FUNC, LEN>
{
	using length_type = LEN;
	using len_enc_impl<FUNC, LEN>::len_enc_impl;
	using len_enc_impl<FUNC, LEN>::operator();


	//length position by exact type match
	template <class T, class ...ARGS>
	auto operator () (T const& len_ie, ARGS&&...) ->
		std::enable_if_t<std::is_same_v<typename T::field_type, length_type>, MED_RESULT>
	{
		CODEC_TRACE("len_enc[%s] by IE", name<length_type>());
		this->m_lenpos = this->m_encoder(GET_STATE{}); //save position in encoded buffer to update with correct length
		//TODO: trigger setters if any?
		m_len_ie.copy(len_ie.ref_field());
		return this->m_encoder(ADVANCE_STATE{int(length_type::traits::bits)});
	}

	//update the length with final value
	MED_RESULT set_length_ie()
	{
		if (this->m_lenpos)
		{
			auto const end = this->m_encoder(GET_STATE{});
			std::size_t const len_value = end - this->m_start - 1;
			MED_CHECK_FAIL(length_to_value(this->m_encoder, m_len_ie, len_value));
			this->m_encoder(SET_STATE{}, this->m_lenpos);
			MED_CHECK_FAIL(encode(this->m_encoder, m_len_ie));
			this->m_encoder(SET_STATE{}, end);
		}
		MED_RETURN_SUCCESS;
	}

private:
	length_type m_len_ie;
};


template <class T, typename Enable = void>
struct container_encoder
{
	template <class FUNC, class IE>
	static constexpr MED_RESULT encode(FUNC& func, IE const& ie)
	{
		if constexpr (is_length_v<IE>)
		{
			using length_type = typename IE::length_type;

			if constexpr (has_padding<IE>::value)
			{
				CODEC_TRACE("start %s with padded len_type=%s...:", name<IE>(), name<length_type>());
				using pad_t = padder<IE, FUNC>;
				pad_t pad{func};
				length_encoder<IE::template has<length_type>(), FUNC, length_type> le{ func };
				MED_CHECK_FAIL(ie.encode(le));
				//special case for empty elements w/o length placeholder
				pad.enable(static_cast<bool>(le));
				if constexpr (pad_t::pad_traits::inclusive)
				{
					MED_CHECK_FAIL(pad.add());
					return le.set_length_ie();
				}
				else
				{
					MED_CHECK_FAIL(le.set_length_ie());
					return pad.add();
				}
			}
			else
			{
				CODEC_TRACE("start %s with len_type=%s...:", name<IE>(), name<length_type>());
				length_encoder<IE::template has<length_type>(), FUNC, length_type> le{ func };
				MED_CHECK_FAIL(ie.encode(le));
				return le.set_length_ie();
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
	static MED_RESULT encode(FUNC& func, IE const& ie)
	{
		return typename T::container_encoder{}(func, ie);
	}
};


//IE_NULL
template <class WRAPPER, class FUNC, class IE>
constexpr MED_RESULT encode_ie(FUNC& func, IE const& ie, IE_NULL const&)
{
	return null_encoder<FUNC>::encode(func, ie);
}

template <class WRAPPER, class FUNC, class IE>
inline MED_RESULT encode_ie(FUNC& func, IE const& ie, CONTAINER const&)
{
	return container_encoder<FUNC>::encode(func, ie);
}

template <class WRAPPER, class FUNC, class IE>
inline MED_RESULT encode_ie(FUNC& func, IE const& ie, PRIMITIVE const&)
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
inline MED_RESULT encode_ie(FUNC& func, IE const& ie, IE_LV const&)
{
	typename WRAPPER::length_type len_ie{};
	std::size_t len_value = get_length(ref_field(ie));
	CODEC_TRACE("L=%zx[%s]:", std::size_t(len_ie.get()), name<IE>());
	return length_to_value(func, len_ie, len_value)
		MED_AND encode(func, len_ie)
		MED_AND encode(func, ref_field(ie));
}

template <class WRAPPER, class FUNC, class IE>
inline MED_RESULT encode_ie(FUNC& func, IE const& ie, IE_TV const&)
{
	typename WRAPPER::tag_type const tag_ie{};
	CODEC_TRACE("T%zx<%s>{%s}", std::size_t(tag_ie.get()), name<WRAPPER>(), name<IE>());
	return encode(func, tag_ie)
		MED_AND encode_ie<typename WRAPPER::field_type>(func, ref_field(ie), typename WRAPPER::field_type::ie_type{});
}

}	//end: namespace sl

template <class FUNC, class IE>
constexpr MED_RESULT encode(FUNC&& func, IE const& ie)
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
