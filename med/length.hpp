/**
@file
length type definition and traits

@copyright Denis Priyomov 2016-2017
Distributed under the MIT License
(See accompanying file LICENSE or visit https://github.com/cppden/med)
*/

#pragma once

#include "exception.hpp"
#include "ie_type.hpp"
#include "field.hpp"
#include "state.hpp"
#include "value_traits.hpp"
#include "name.hpp"
#include "meta/typelist.hpp"
#include "padding.hpp"
#include "concepts.hpp"
#include "accessor.hpp"


namespace med {

template <class LENGTH>
struct length_t
{
	using length_type = LENGTH;
};

namespace detail {

template <class T>
concept AHasGetLength = requires(T const& v)
{
	{ v.get_length() } -> std::integral;
};

template <class>
struct get_dependency { using type = void; };
template <class T> requires requires(T v){ typename T::dependency_type; }
struct get_dependency<T>
{
	using type = typename T::dependency_type;
};

} //end: namespace detail

template <class T>
using get_dependency_t = typename detail::get_dependency<T>::type;


namespace sl {

template <class TYPE_CTX, class IE, class ENCODER>
constexpr std::size_t ie_length(IE const& ie, ENCODER& encoder) noexcept
{
	using META_INFO = typename TYPE_CTX::meta_info_type;
	using EXP_TAG = typename TYPE_CTX::explicit_tag_type;
	using EXP_LEN = typename TYPE_CTX::explicit_length_type;

	std::size_t len = 0;

	if constexpr (not meta::list_is_empty_v<META_INFO>)
	{
		using mi = meta::list_first_t<META_INFO>;
		using info_t = get_info_t<mi>;
		using exp_tag_t = conditional_t<mi::kind == mik::TAG && APresentIn<info_t, IE>, info_t, EXP_TAG>;
		using exp_len_t = conditional_t<mi::kind == mik::LEN && APresentIn<info_t, IE>, info_t, EXP_LEN>;

		//TODO: pass calculated length to length_t when sizeof(len) depends on value like in ASN.1 BER
		CODEC_TRACE("%s[%s]<%s:%s>: %s", __FUNCTION__, name<IE>(), name<exp_tag_t>(), name<exp_len_t>(), name<info_t>());
		len += ie_length<type_context<typename TYPE_CTX::ie_type, meta::list_rest_t<META_INFO>, exp_tag_t, exp_len_t>>(ie, encoder);
		if constexpr (mi::kind == mik::LEN)
		{
			//TODO: involve codec to get length type + may need to set its value like for BER
			using pad_traits = typename get_padding<info_t>::type;
			if constexpr (!std::is_void_v<pad_traits>)
			{
				using pad_t = typename ENCODER::template padder_type<pad_traits, ENCODER>;
#ifdef CODEC_TRACE_ENABLE
				auto const add_len = pad_t::calc_padding_size(len);
				CODEC_TRACE("padded len_type=%s: %zu + %zu = %zu", name<info_t>(), size_t(len), add_len, len + add_len);
				len += add_len;
#else
				len += pad_t::calc_padding_size(len);
#endif
			}
		}
		//calc length of LEN or TAG itself
		len += ie_length<type_context<typename TYPE_CTX::ie_type, meta::typelist<>, EXP_TAG, EXP_LEN>>(info_t{}, encoder);
	}
	else //data itself
	{
		CODEC_TRACE("%s[%s]<%s:%s> - DATA", __FUNCTION__, name<IE>(), name<EXP_TAG>(), name<EXP_LEN>());
		if constexpr (AContainer<IE>)
		{
			using ctx = type_context<typename TYPE_CTX::ie_type, meta::typelist<>, EXP_TAG, EXP_LEN>;
			CODEC_TRACE("%s[%.30s]%s<%s:%s>: %s", __FUNCTION__, name<IE>(), AMultiField<IE>?"*":"", name<EXP_TAG>(), name<EXP_LEN>(), name<typename IE::ie_type>());
			len += ie.template calc_length<ctx>(encoder);
			CODEC_TRACE("%s[%s] : len(SEQ) = %zu", __FUNCTION__, name<IE>(), len);
		}
		//NOTE: can't unroll multi-field here because for ASN.1 the OID and SEQENCE-OF
		//are based on multi-field but need different length calculation thus it's passed
		//directly to encoder
		else if constexpr (ASameAs<get_field_type_t<IE>, EXP_TAG, EXP_LEN>)
		{
			CODEC_TRACE("%s[%s] : skip explicit", __FUNCTION__, name<IE>());
		}
		else
		{
			//CODEC_TRACE("<%s:%s> : %s", class_name<EXP_TAG>(), class_name<EXP_LEN>(), class_name<IE>());
			len += encoder(GET_LENGTH{}, ie);
			CODEC_TRACE("%s[%s] : len(VAL) = %zu", __FUNCTION__, name<IE>(), len);
		}
	}
	CODEC_TRACE("%s[%s]<%s:%s> -> %zu", __FUNCTION__, name<IE>(), name<EXP_TAG>(), name<EXP_LEN>(), len);
	return len;
}

} //end: namespace sl

template <class IE, class ENCODER>
constexpr std::size_t field_length(IE const& ie, ENCODER& encoder) noexcept
{
	using mi = meta::produce_info_t<ENCODER, IE>;
	//CODEC_TRACE("%s[%s]", __FUNCTION__, name<IE>());
	return sl::ie_length<type_context<typename IE::ie_type, mi>>(ie, encoder);
}


template <AField FIELD>
constexpr void length_to_value(FIELD& field, std::size_t len)
{
	//set the length IE with the value
	if constexpr (AHasSetLength<FIELD>)
	{
		if constexpr (std::is_same_v<bool, decltype(field.set_length(0))>)
		{
			if (not field.set_length(len))
			{
				MED_THROW_EXCEPTION(invalid_value, name<FIELD>(), len)
			}
		}
		else
		{
			field.set_length(len);
		}
	}
	else if constexpr (std::is_same_v<bool, decltype(field.set_encoded(0))>)
	{
		if (not field.set_encoded(len))
		{
			MED_THROW_EXCEPTION(invalid_value, name<FIELD>(), len)
		}
	}
	else
	{
		field.set_encoded(len);
	}
	CODEC_TRACE("L=%zXh(%zX) [%s]:", len, std::size_t(field.get_encoded()), name<FIELD>());
}

template <class FIELD>
constexpr std::size_t value_to_length(FIELD& field)
{
	//use proper length accessor
	if constexpr (detail::AHasGetLength<FIELD>)
	{
		std::size_t const len = field.get_length();
		CODEC_TRACE("LV[%s]=%zu", name<FIELD>(), len);
		return len;
	}
	else
	{
		std::size_t const len = field.get_encoded();
		CODEC_TRACE("LV[%s]=%zu", name<FIELD>(), len);
		return len;
	}
}

}	//end: namespace med
