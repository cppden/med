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
#include "meta/typelist.hpp"


namespace med {

//structure layer
namespace sl {

//Tag
template <class TAG_TYPE, class ENCODER>
constexpr void encode_tag(ENCODER& encoder)
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
constexpr void encode_len(ENCODER& encoder, std::size_t len)
{
	if constexpr (not is_peek_v<LEN_TYPE>) //do nothing if it's a peek preview
	{
		LEN_TYPE ie;
		length_to_value(ie, len);
		CODEC_TRACE("%s=%zX[%s]", __FUNCTION__, std::size_t(ie.get_encoded()), name<LEN_TYPE>());
		encoder(ie, IE_LEN{});
	}
}

template <class EXPOSED, class T, typename Enable = void>
struct container_encoder
{
	template <class ENCODER, class IE>
	static constexpr void encode(ENCODER& encoder, IE const& ie)
	{
		CODEC_TRACE("%s w/o length exposed=%s...:", name<IE>(), name<EXPOSED>());
		if constexpr (std::is_void_v<EXPOSED>)
		{
			ie.encode(encoder);
		}
		else
		{
			ie.template encode<meta::list_rest_t<typename IE::ies_types>>(encoder);
		}
	}
};

//special case for printer
template <class EXPOSED, class T>
struct container_encoder<EXPOSED, T, std::void_t<typename T::container_encoder>>
{
	template <class ENCODER, class IE>
	static void encode(ENCODER& encoder, IE const& ie)
	{
		return typename T::container_encoder{}(encoder, ie);
	}
};

template <class META_INFO, class EXPOSED, class ENCODER, class IE>
constexpr void ie_encode(ENCODER& encoder, IE const& ie)
{
	if constexpr (not is_peek_v<IE>) //do nothing if it's a peek preview
	{
		if constexpr (not meta::list_is_empty_v<META_INFO>)
		{
			using mi = meta::list_first_t<META_INFO>;
			using mi_rest = meta::list_rest_t<META_INFO>;
			CODEC_TRACE("%s[%s-%s]: %s", __FUNCTION__, name<IE>(), name<EXPOSED>(), class_name<mi>());

			if constexpr (mi::kind == mik::TAG)
			{
				using tag_t = typename mi::info_type;
				encode_tag<tag_t>(encoder);
			}
			else if constexpr (mi::kind == mik::LEN)
			{
				using length_t = typename mi::info_type;
				if constexpr (std::is_same_v<typename IE::ie_type, CONTAINER>)
				{
					if constexpr (IE::template has<length_t>())
					{
						CODEC_TRACE("LV=!? [%s] rest=%s multi=%d", name<IE>(), class_name<mi_rest>(), is_multi_field_v<IE>);
					}
					else
					{
						CODEC_TRACE("LV=? [%s] rest=%s multi=%d", name<IE>(), class_name<mi_rest>(), is_multi_field_v<IE>);
					}
				}
				else
				{
					CODEC_TRACE("LV=? [%s] rest=%s multi=%d", name<IE>(), class_name<mi_rest>(), is_multi_field_v<IE>);
				}
				auto len = sl::ie_length<EXPOSED, mi_rest>(ie, encoder);
				using dependency_type = get_dependency_t<length_t>;
				if constexpr (!std::is_void_v<dependency_type>)
				{
					auto const delta = length_t::dependency(ie.template get<dependency_type>());
					len -= delta;
					CODEC_TRACE("adjusted by %d L=%zxh [%s] dependent on %s", -delta, len, name<IE>(), name<dependency_type	>());
				}

				CODEC_TRACE("LV=%zxh [%s]", len, name<IE>());
				encode_len<length_t>(encoder, len);

				using pad_traits = typename get_padding<length_t>::type;
				if constexpr (!std::is_void_v<pad_traits>)
				{
					CODEC_TRACE("padded len_type=%s...:", name<length_t>());
					using pad_t = typename ENCODER::template padder_type<pad_traits, ENCODER>;
					pad_t pad{encoder};
					ie_encode<mi_rest, EXPOSED>(encoder, ie);
					pad.add_padding();
					return;
				}
			}

			ie_encode<mi_rest, EXPOSED>(encoder, ie);
		}
		else
		{
			using ie_type = typename IE::ie_type;
			CODEC_TRACE("%s[%.30s]: %s", __FUNCTION__, class_name<IE>(), class_name<ie_type>());
			if constexpr (std::is_base_of_v<CONTAINER, ie_type>)
			{
				container_encoder<EXPOSED, ENCODER>::encode(encoder, ie);
			}
			else
			{
				if constexpr (!std::is_same_v<null_allocator, std::remove_const_t<typename ENCODER::allocator_type>>)
				{
					put_snapshot(encoder, ie);
				}
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
		sl::ie_encode<mi, void>(encoder, ie);
	}
	else //special cases like passing a placeholder
	{
		encoder(ie);
	}
}

}	//end: namespace med
