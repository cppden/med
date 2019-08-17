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

//Length
template <class LEN_TYPE, class ENCODER>
inline void encode_len(ENCODER& encoder, std::size_t len)
{
	if constexpr (not is_peek_v<LEN_TYPE>) //do nothing if it's a peek preview
	{
		LEN_TYPE ie;
		length_to_value(ie, len);
		encoder(ie, IE_LEN{});
	}
}

template <bool BY_IE, class ENCODER, class LEN>
struct length_encoder;

template <class ENCODER, class LEN>
struct len_enc_impl
{
	using state_type = typename ENCODER::state_type;
	using length_type = LEN;

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
	static constexpr auto get_tag_type()              { return ENCODER::template get_tag_type<T>(); }

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
template <class ENCODER, class LEN>
struct length_encoder<false, ENCODER, LEN> : len_enc_impl<ENCODER, LEN>
{
	using length_type = LEN;
	using len_enc_impl<ENCODER, LEN>::len_enc_impl;
	using len_enc_impl<ENCODER, LEN>::operator();

	//length position by placeholder
	template <int DELTA>
	auto operator() (placeholder::_length<DELTA> const&)
	{
		CODEC_TRACE("len_enc[%s] %+d by placeholder", name<length_type>(), DELTA);
		m_delta = DELTA;
		//save position in encoded buffer to update with correct length
		this->m_lenpos = this->m_encoder(GET_STATE{});
		return this->m_encoder(ADVANCE_STATE{+ENCODER::template size_of<length_type>()});
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
template <class ENCODER, class LEN>
struct length_encoder<true, ENCODER, LEN> : len_enc_impl<ENCODER, LEN>
{
	using length_type = LEN;
	using len_enc_impl<ENCODER, LEN>::len_enc_impl;
	using len_enc_impl<ENCODER, LEN>::operator();


	//length position by exact type match
	template <class T, class ...ARGS>
	auto operator () (T const& len_ie, ARGS&&...) ->
		std::enable_if_t<std::is_same_v<get_field_type_t<T>, length_type>>
	{
		CODEC_TRACE("len_enc[%s] by IE", name<length_type>());
		this->m_lenpos = this->m_encoder(GET_STATE{}); //save position in encoded buffer to update with correct length
		//TODO: trigger setters if any?
		m_len_ie.copy(ref_field(len_ie));
		return this->m_encoder(ADVANCE_STATE{+ENCODER::template size_of<length_type>()});
	}

	//update the length with final value
	void set_length_ie()
	{
		if (this->m_lenpos)
		{
			auto const end = this->m_encoder(GET_STATE{});
			std::size_t const len_value = end - this->m_start - 1;
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

			if constexpr (has_padding<IE>::value)
			{
				CODEC_TRACE("start %s with padded len_type=%s...:", name<IE>(), name<length_type>());
				using pad_t = padder<IE, ENCODER>;
				pad_t pad{encoder};
				length_encoder<IE::template has<length_type>(), ENCODER, length_type> le{ encoder };
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
				length_encoder<IE::template has<length_type>(), ENCODER, length_type> le{ encoder };
				ie.encode(le);
				le.set_length_ie();
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

//Tag
template <class TAG_TYPE, class ENCODER>
inline void encode_tag(ENCODER& encoder)
{
	if constexpr (not is_peek_v<TAG_TYPE>) //do nothing if it's a peek preview
	{
		CODEC_TRACE("%s[%s]", __FUNCTION__, name<TAG_TYPE>());
		TAG_TYPE const ie{};
		encoder(ie, IE_TAG{});
	}
}

template <class META_INFO, class ENCODER, class IE>
inline void ie_encode(ENCODER& encoder, IE const& ie)
{
	if constexpr (not is_peek_v<IE>) //do nothing if it's a peek preview
	{
		if constexpr (not meta::is_list_empty_v<META_INFO>)
		{
			using mi = meta::list_first_t<META_INFO>;
			CODEC_TRACE("%s[%s]: %s", __FUNCTION__, name<IE>(), class_name<mi>());

			if constexpr (mi::kind == mik::TAG)
			{
				using tag_type = med::meta::unwrap_t<decltype(ENCODER::template get_tag_type<mi>())>;
				encode_tag<tag_type>(encoder);
			}
			else if constexpr (mi::kind == mik::LEN)
			{
				CODEC_TRACE("LV=? [%s]", name<IE>());
				auto const len = field_length(rref_field(ie), encoder);
				CODEC_TRACE("LV=%zxh [%s]", len, name<IE>());
				encode_len<typename mi::length_type>(encoder, len);
			}

			ie_encode<meta::list_rest_t<META_INFO>>(encoder, ie);
		}
		else
		{
			auto const& field = rref_field(ie);
			//using ie_type = typename std::remove_reference_t<decltype(rref_field(ie))>::ie_type;
			using field_type = remove_cref_t<decltype(field)>;
			using ie_type = typename field_type::ie_type;
			CODEC_TRACE("%s[%.30s]: %s", __FUNCTION__, class_name<field_type>(), class_name<ie_type>());
			if constexpr (std::is_base_of_v<CONTAINER, ie_type>)
			{
				call_if<is_callable_with_v<ENCODER, ENTRY_CONTAINER>>::call(encoder, ENTRY_CONTAINER{}, field);
				container_encoder<ENCODER>::encode(encoder, field);
				call_if<is_callable_with_v<ENCODER, EXIT_CONTAINER>>::call(encoder, EXIT_CONTAINER{}, field);
			}
			else
			{
				snapshot(encoder, field);
				encoder(field, ie_type{});
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
		sl::ie_encode<get_meta_info_t<IE>>(encoder, ie);
	}
	else //special cases like passing a placeholder
	{
		encoder(ie);
	}
}

}	//end: namespace med
