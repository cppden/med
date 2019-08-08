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
#include "exception.hpp"
#include "optional.hpp"
#include "snapshot.hpp"
#include "padding.hpp"
#include "length.hpp"
#include "name.hpp"
#include "placeholder.hpp"


namespace med {

//structure layer
namespace sl {

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
	template <class... Args> //NOTE: decltype is needed to expose actually defined operators
	auto operator() (Args&&... args) -> decltype(std::declval<FUNC>()(std::forward<Args>(args)...))
	{
		//CODEC_TRACE("%s", __PRETTY_FUNCTION__);
		return m_encoder(std::forward<Args>(args)...);
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
	void set_length_ie()
	{
		if (this->m_lenpos)
		{
			auto const end = this->m_encoder(GET_STATE{});
			std::size_t len_value = end - this->m_start - m_delta;
			CODEC_TRACE("LENGTH stop: len=%zu(%+d)", len_value, m_delta);
			length_type len_ie;
			length_to_value(len_ie, len_value);
			this->m_encoder(SET_STATE{}, this->m_lenpos);
			encode(this->m_encoder, len_ie);
			this->m_encoder(SET_STATE{}, end);
		}
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
		std::enable_if_t<std::is_same_v<typename T::field_type, length_type>, void>
	{
		CODEC_TRACE("len_enc[%s] by IE", name<length_type>());
		this->m_lenpos = this->m_encoder(GET_STATE{}); //save position in encoded buffer to update with correct length
		//TODO: trigger setters if any?
		m_len_ie.copy(len_ie.ref_field());
		return this->m_encoder(ADVANCE_STATE{int(length_type::traits::bits)});
	}

	//update the length with final value
	void set_length_ie()
	{
		if (this->m_lenpos)
		{
			auto const end = this->m_encoder(GET_STATE{});
			std::size_t const len_value = end - this->m_start - 1;
			length_to_value(m_len_ie, len_value);
			this->m_encoder(SET_STATE{}, this->m_lenpos);
			encode(this->m_encoder, m_len_ie);
			this->m_encoder(SET_STATE{}, end);
		}
	}

private:
	length_type m_len_ie;
};


template <class T, typename Enable = void>
struct container_encoder
{
	template <class FUNC, class IE>
	static constexpr void encode(FUNC& func, IE const& ie)
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
				ie.encode(le);
				//special case for empty elements w/o length placeholder
				pad.enable(static_cast<bool>(le));
				if constexpr (pad_t::pad_traits::inclusive)
				{
					pad.add();
					le.set_length_ie();
				}
				else
				{
					le.set_length_ie();
					pad.add();
				}
			}
			else
			{
				CODEC_TRACE("start %s with len_type=%s...:", name<IE>(), name<length_type>());
				length_encoder<IE::template has<length_type>(), FUNC, length_type> le{ func };
				ie.encode(le);
				le.set_length_ie();
			}
		}
		else
		{
			CODEC_TRACE("%s...:", name<IE>());
			ie.encode(func);
		}
	}
};

template <class T>
struct container_encoder<T, std::void_t<typename T::container_encoder>>
{
	template <class FUNC, class IE>
	static void encode(FUNC& func, IE const& ie)
	{
		return typename T::container_encoder{}(func, ie);
	}
};

template <class WRAPPER, class FUNC, class IE>
inline void encode_ie(FUNC& func, IE const& ie, CONTAINER)
{
	call_if<is_callable_with_v<FUNC, ENTRY_CONTAINER>>::call(func, ENTRY_CONTAINER{}, ie);
	container_encoder<FUNC>::encode(func, ie);
	call_if<is_callable_with_v<FUNC, EXIT_CONTAINER>>::call(func, EXIT_CONTAINER{}, ie);
}

template <class WRAPPER, class FUNC, class IE>
inline void encode_ie(FUNC& func, IE const& ie, PRIMITIVE)
{
	if constexpr (not is_peek_v<IE>) //do nothing if it's a peek preview
	{
		snapshot(func, ie);
		func(ie, typename WRAPPER::ie_type{});
	}
}

template <class WRAPPER, class FUNC, class IE>
inline void encode_ie(FUNC& func, IE const& ie, IE_LV)
{
	typename WRAPPER::length_type len_ie{};
	std::size_t len_value = field_length(ref_field(ie));
	length_to_value(len_ie, len_value);
	CODEC_TRACE("L=%zx<%s>{%s}", std::size_t(len_ie.get()), name<WRAPPER>(), name<IE>());
	encode(func, len_ie);
	encode_ie<typename WRAPPER::field_type>(func, ref_field(ie), typename WRAPPER::field_type::ie_type{});
}

template <class WRAPPER, class FUNC, class IE>
inline void encode_ie(FUNC& func, IE const& ie, IE_TV)
{
	typename WRAPPER::tag_type const tag_ie{};
	CODEC_TRACE("T=%zx<%s>{%s}", std::size_t(tag_ie.get()), name<WRAPPER>(), name<IE>());
	encode(func, tag_ie);
	encode_ie<typename WRAPPER::field_type>(func, ref_field(ie), typename WRAPPER::field_type::ie_type{});
}

}	//end: namespace sl

template <class FUNC, class IE>
constexpr void encode(FUNC&& func, IE const& ie)
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
