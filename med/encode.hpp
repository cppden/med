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

template <
	class META_INFO = meta::typelist<>,
	class EXPOSED = void,
	class EXP_LEN = void
>
struct encode_type_context
{
	using meta_info_type = META_INFO;
	using exposed_type = EXPOSED;
	using explicit_length_type = EXP_LEN;
};

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
				encode_tag<typename mi::info_type>(encoder);
			}
			else if constexpr (mi::kind == mik::LEN)
			{
				using len_t = typename mi::info_type;
				auto len = sl::ie_length<EXPOSED, mi_rest>(ie, encoder);
				CODEC_TRACE("LV[%s]=%zX%c rest=%s", name<len_t>(), len, AMultiField<IE>?'*':' ', class_name<mi_rest>());
				using dependency_t = get_dependency_t<len_t>;
				if constexpr (!std::is_void_v<dependency_t>)
				{
					auto const delta = len_t::dependency(ie.template get<dependency_t>());
					len -= delta;
					CODEC_TRACE("adjusted by %d L=%zxh [%s] dependent on %s", -delta, len, name<IE>(), name<dependency_t>());
				}

				if constexpr (APresentIn<len_t, IE>)
				{
					static_assert(not is_peek_v<len_t>);
					//TODO: a way to avoid cast?
					auto& ie_len = const_cast<IE&>(ie).template ref<len_t>();
					length_to_value(ie_len, len);
					CODEC_TRACE("explicit LV[%s]=%zX", name<len_t>(), std::size_t(ie_len.get_encoded()));
				}
				else if constexpr (not is_peek_v<len_t>) //do nothing if it's a peek preview
				{
					len_t ie_len;
					length_to_value(ie_len, len);
					encoder(ie_len, IE_LEN{});
				}

				using pad_traits = typename get_padding<len_t>::type;
				if constexpr (!std::is_void_v<pad_traits>)
				{
					CODEC_TRACE("padded len_type=%s...:", name<len_t>());
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
			CODEC_TRACE("%s[%.30s]: %s", __FUNCTION__, name<IE>(), name<ie_type>());
			if constexpr (std::is_base_of_v<CONTAINER, ie_type>)
			{
				//special case for printer
				if constexpr (requires { typename ENCODER::container_encoder; })
				{
					typename ENCODER::container_encoder{}(encoder, ie);
				}
				else
				{
					if constexpr (std::is_void_v<EXPOSED>)
					{
						CODEC_TRACE(">>> %s", name<IE>());
						ie.encode(encoder);
						CODEC_TRACE("<<< %s", name<IE>());
					}
					else //NOTE! EXPOSED should be the 1st IE
					{
						CODEC_TRACE(">>> %s exposed=%s", name<IE>(), name<EXPOSED>());
						ie.template encode<meta::list_rest_t<typename IE::ies_types>>(encoder);
						CODEC_TRACE("<<< %s exposed=%s", name<IE>(), name<EXPOSED>());
					}
				}
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

template <class ENCODER, AHasIeType IE>
constexpr void encode(ENCODER&& encoder, IE const& ie)
{
	using mi = meta::produce_info_t<ENCODER, IE>;
	//CODEC_TRACE("mi=%s", class_name<mi>());
	sl::ie_encode<mi, void>(encoder, ie);
}

}	//end: namespace med
