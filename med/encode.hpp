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
#include "meta/typelist.hpp"


namespace med {

//structure layer
namespace sl {

//Tag
template <class TAG_TYPE, class ENCODER>
inline void encode_tag(ENCODER& encoder)
{
	if constexpr (not is_peek_v<TAG_TYPE>) //do nothing if it's a peek preview
	{
		TAG_TYPE const ie{};
		CODEC_TRACE("%s=%zX[%s]", __FUNCTION__, std::size_t(ie.get_encoded()), name<TAG_TYPE>());
		encoder(ie, IE_TAG{});
	}
}

//Length
template <class LEN_TYPE, class ENCODER>
inline void encode_len(ENCODER& encoder, std::size_t len)
{
	if constexpr (not is_peek_v<LEN_TYPE>) //do nothing if it's a peek preview
	{
		LEN_TYPE ie;
		length_to_value(ie, len);
		CODEC_TRACE("%s=%zX[%s]", __FUNCTION__, std::size_t(ie.get_encoded()), name<LEN_TYPE>());
		encoder(ie, IE_LEN{});
	}
}

template <bool BY_IE, class LEN, class ENCODER>
struct length_encoder;

template <class LEN, class ENCODER>
struct len_enc_impl
{
	using state_type = typename ENCODER::state_type;
	using length_type = LEN;
	template <class... PA>
	using padder_type = typename ENCODER::template padder_type<PA...>;

	explicit len_enc_impl(ENCODER& encoder) noexcept
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
	explicit operator bool() const                    { return static_cast<bool>(m_lenpos); }

	template <class T>
	static constexpr std::size_t size_of()            { return ENCODER::template size_of<T>(); }
	template <class T>
	static constexpr auto produce_meta_info()         { return ENCODER::template produce_meta_info<T>(); }

	//forward regular types to encoder
	template <class... Args> //NOTE: decltype is needed to expose actually defined operators
	auto operator() (Args&&... args) -> decltype(std::declval<ENCODER>()(std::forward<Args>(args)...))
	{
		//CODEC_TRACE("%s", __PRETTY_FUNCTION__);
		return m_encoder(std::forward<Args>(args)...);
	}

protected:
	ENCODER&         m_encoder;
	state_type const m_start;
	state_type       m_lenpos { };
};


//by placeholder
template <class LEN, class ENCODER>
struct length_encoder<false, LEN, ENCODER> : len_enc_impl<LEN, ENCODER>
{
	using length_type = LEN;
	using len_enc_impl<LEN, ENCODER>::len_enc_impl;
	using len_enc_impl<LEN, ENCODER>::operator();

	//length position by placeholder
	template <int DELTA>
	auto operator() (placeholder::_length<DELTA> const&)
	{
		CODEC_TRACE("len_enc[%s] %+d by placeholder", name<length_type>(), DELTA);
		m_delta = DELTA;
		//save position in encoded buffer to update with correct length
		this->m_lenpos = this->m_encoder(GET_STATE{});
		return this->m_encoder(ADVANCE_STATE{int(+ENCODER::template size_of<length_type>())});
	}

	//update the length with final value
	void set_length_ie()
	{
		if (this->m_lenpos)
		{
			auto const end = this->m_encoder(GET_STATE{});
			std::size_t len = end - this->m_start - m_delta;
			CODEC_TRACE("LENGTH stop: len=%zu(%+d)", len, m_delta);
			this->m_encoder(SET_STATE{}, this->m_lenpos);
			encode_len<length_type>(this->m_encoder, len);
			this->m_encoder(SET_STATE{}, end);
		}
	}

private:
	int m_delta {0};
};

//by exact length type
template <class LEN, class ENCODER>
struct length_encoder<true, LEN, ENCODER> : len_enc_impl<LEN, ENCODER>
{
	using length_type = LEN;
	using len_enc_impl<LEN, ENCODER>::len_enc_impl;
	using len_enc_impl<LEN, ENCODER>::operator();


	//length position by exact type match
	template <class T, class ...ARGS>
	auto operator () (T const& len_ie, ARGS&&...) ->
		std::enable_if_t<std::is_same_v<get_field_type_t<T>, length_type>>
	{
		CODEC_TRACE("len_enc[%s] by IE", name<length_type>());
		this->m_lenpos = this->m_encoder(GET_STATE{}); //save position in encoded buffer to update with correct length
		//TODO: trigger setters if any?
		m_len_ie.copy(len_ie);
		return this->m_encoder(ADVANCE_STATE{+ENCODER::template size_of<length_type>()});
	}

	//update the length with final value
	void set_length_ie()
	{
		if (this->m_lenpos)
		{
			auto const end = this->m_encoder(GET_STATE{});
			std::size_t const len_value = end - this->m_start - length_type::traits::bits/8;
			this->m_encoder(SET_STATE{}, this->m_lenpos);
			length_to_value(m_len_ie, len_value);
			this->m_encoder(m_len_ie, IE_LEN{});
			this->m_encoder(SET_STATE{}, end);
		}
	}

private:
	length_type m_len_ie;
};


template <class T, typename Enable = void>
struct container_encoder
{
	template <class ENCODER, class IE>
	static constexpr void encode(ENCODER& encoder, IE const& ie)
	{
		if constexpr (is_length_v<IE>)
		{
			using length_type = typename IE::length_type;
			using pad_traits = typename get_padding<length_type>::type;

			length_encoder<IE::template has<length_type>(), length_type, ENCODER> le{ encoder };

			if constexpr (std::is_void_v<pad_traits>)
			{
				CODEC_TRACE("start %s with len_type=%s...:", name<IE>(), name<length_type>());
				ie.encode(le);
				le.set_length_ie();
			}
			else
			{
				CODEC_TRACE("start %s with padded len_type=%s...:", name<IE>(), name<length_type>());
				using pad_t = typename ENCODER::template padder_type<pad_traits, ENCODER>;
				pad_t pad{encoder};
				ie.encode(le);
				//special case for empty elements w/o length placeholder
				pad.enable(static_cast<bool>(le));
				le.set_length_ie();
				pad.add();
			}
		}
		else
		{
			CODEC_TRACE("%s...:", name<IE>());
			ie.encode(encoder);
		}
	}
};

//special case for printer
template <class T>
struct container_encoder<T, std::void_t<typename T::container_encoder>>
{
	template <class ENCODER, class IE>
	static void encode(ENCODER& encoder, IE const& ie)
	{
		return typename T::container_encoder{}(encoder, ie);
	}
};

template <class META_INFO, class ENCODER, class IE>
inline void ie_encode(ENCODER& encoder, IE const& ie)
{
	if constexpr (not is_peek_v<IE>) //do nothing if it's a peek preview
	{
		if constexpr (not meta::list_is_empty_v<META_INFO>)
		{
			using mi = meta::list_first_t<META_INFO>;
			using mi_rest = meta::list_rest_t<META_INFO>;
			CODEC_TRACE("%s[%s]: %s", __FUNCTION__, name<IE>(), class_name<mi>());

			if constexpr (mi::kind == mik::TAG)
			{
				encode_tag<mi>(encoder);
			}
			else if constexpr (mi::kind == mik::LEN)
			{
				//CODEC_TRACE("LV=? [%s] rest=%s multi=%d", name<IE>(), class_name<mi_rest>(), is_multi_field_v<IE>);
				auto const len = sl::ie_length<mi_rest>(ie, encoder);
				//CODEC_TRACE("LV=%zxh [%s]", len, name<IE>());
				encode_len<typename mi::length_type>(encoder, len);
			}

			ie_encode<mi_rest>(encoder, ie);
		}
		else
		{
			using ie_type = typename IE::ie_type;
			CODEC_TRACE("%s[%.30s]: %s", __FUNCTION__, class_name<IE>(), class_name<ie_type>());
			if constexpr (std::is_base_of_v<CONTAINER, ie_type>)
			{
				call_if<is_callable_with_v<ENCODER, ENTRY_CONTAINER>>::call(encoder, ENTRY_CONTAINER{}, ie);
				container_encoder<ENCODER>::encode(encoder, ie);
				call_if<is_callable_with_v<ENCODER, EXIT_CONTAINER>>::call(encoder, EXIT_CONTAINER{}, ie);
			}
			else
			{
				snapshot(encoder, ie);
				encoder(ie, ie_type{});
			}
		}
	}
}

}	//end: namespace sl

template <class ENCODER, class IE>
constexpr void encode(ENCODER&& encoder, IE const& ie)
{
	if constexpr (has_ie_type_v<IE>)
	{
		using mi = meta::produce_info_t<ENCODER, IE>;
		CODEC_TRACE("mi=%s", class_name<mi>());
		sl::ie_encode<mi>(encoder, ie);
	}
	else //special cases like passing a placeholder
	{
		encoder(ie);
	}
}

}	//end: namespace med
