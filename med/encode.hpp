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
	TAG_TYPE const ie{};
	CODEC_TRACE("%s=%zX[%s]", __FUNCTION__, std::size_t(ie.get_encoded()), name<TAG_TYPE>());
	encoder(ie, IE_TAG{});
}

template <class TYPE_CTX, class ENCODER, class IE>
constexpr void ie_encode(ENCODER& encoder, IE const& ie)
{
	using META_INFO = typename TYPE_CTX::meta_info_type;
	using EXP_TAG = typename TYPE_CTX::explicit_tag_type;
	using EXP_LEN = typename TYPE_CTX::explicit_length_type;

	if constexpr (not meta::list_is_empty_v<META_INFO>)
	{
		using mi = meta::list_first_t<META_INFO>;
		using info_t = get_info_t<mi>;
		using exp_tag_t = conditional_t<mi::kind == mik::TAG && APresentIn<info_t, IE>, info_t, EXP_TAG>;
		using exp_len_t = conditional_t<mi::kind == mik::LEN && APresentIn<info_t, IE>, info_t, EXP_LEN>;
		using ctx = type_context<typename TYPE_CTX::ie_type, meta::list_rest_t<META_INFO>, exp_tag_t, exp_len_t>;
		CODEC_TRACE("%s[%s]<%s:%s>: %s", __FUNCTION__, name<IE>(), name<exp_tag_t>(), name<exp_len_t>(), class_name<mi>());

		if constexpr (mi::kind == mik::TAG)
		{
			if constexpr (!APresentIn<info_t, IE>)
			{
				encode_tag<info_t>(encoder);
			}
			else
			{
				CODEC_TRACE("skip explicit T[%s]", name<info_t>());
			}
		}
		else if constexpr (mi::kind == mik::LEN)
		{
			using len_t = info_t;
			auto len = sl::ie_length<ctx>(ie, encoder);
			CODEC_TRACE("LV[%s]=%zX%c", name<len_t>(), len, AMultiField<IE>?'*':' ');
			using dependency_t = get_dependency_t<len_t>;
			if constexpr (!std::is_void_v<dependency_t>)
			{
				auto const delta = len_t::dependency(ie.template get<dependency_t>());
				len -= delta;
				CODEC_TRACE("adjusted by %d L=%zXh [%s] dependent on %s", -delta, len, name<IE>(), name<dependency_t>());
			}

			if constexpr (APresentIn<info_t, IE>)
			{
				//TODO: a way to avoid cast?
				auto& ie_len = const_cast<IE&>(ie).template ref<len_t>();
				length_to_value(ie_len, len);
				CODEC_TRACE("explicit LV[%s]=%zX", name<len_t>(), std::size_t(ie_len.get_encoded()));
			}
			else
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
				ie_encode<ctx>(encoder, ie);
				pad.add_padding();
				return;
			}
		}

		ie_encode<ctx>(encoder, ie);
	}
	else
	{
		using ie_type = typename IE::ie_type;
		CODEC_TRACE("%s[%.30s]<%s:%s>: %s in %s", __FUNCTION__, name<IE>(), name<EXP_TAG>(), name<EXP_LEN>(), name<ie_type>(), name<typename TYPE_CTX::ie_type>());
		if constexpr (AContainer<IE>)
		{
			//special case for printer
			if constexpr (requires { typename ENCODER::container_encoder; })
			{
				typename ENCODER::container_encoder{}(encoder, ie);
			}
			else
			{
				//TODO: inconsistent between choice and others
				//NOTE! EXP_TAG should be the 1st IE
				if constexpr (std::is_same_v<IE_CHOICE, typename TYPE_CTX::ie_type> && !std::is_void_v<EXP_TAG>)
				{
					CODEC_TRACE(">>> %s<%s:%s>", name<IE>(), name<EXP_TAG>(), name<EXP_LEN>());
					ie.template encode<meta::list_rest_t<typename IE::ies_types>>(encoder);
					CODEC_TRACE("<<< %s<%s:%s>", name<IE>(), name<EXP_TAG>(), name<EXP_LEN>());
				}
				else
				{
					CODEC_TRACE(">>> %s", name<IE>());
					ie.encode(encoder);
					CODEC_TRACE("<<< %s", name<IE>());
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

}	//end: namespace sl

template <class ENCODER, AHasIeType IE>
constexpr void encode(ENCODER&& encoder, IE const& ie)
{
	using META_INFO = meta::produce_info_t<ENCODER, IE>;
	CODEC_TRACE("mi=%s by %s for %s", class_name<META_INFO>(), class_name<ENCODER>(), class_name<IE>());
	sl::ie_encode<type_context<typename IE::ie_type, META_INFO>>(encoder, ie);
}

}	//end: namespace med
