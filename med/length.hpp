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
#include "state.hpp"
#include "value_traits.hpp"
#include "name.hpp"
#include "meta/typelist.hpp"


namespace med {

template <class LENGTH>
struct length_t
{
	using length_type = LENGTH;
};

namespace detail {

template <class, class Enable = void>
struct has_length_type : std::false_type { };
template <class T>
struct has_length_type<T, std::void_t<typename T::length_type>> : std::true_type { };

template <class, typename Enable = void>
struct has_get_length : std::false_type { };
template <class T>
struct has_get_length<T, std::void_t<decltype(std::declval<T const>().get_length())>> : std::true_type { };

template <class, typename Enable = void>
struct has_size : std::false_type { };
template <class T>
struct has_size<T, std::void_t<decltype(std::declval<T const>().size())>> : std::true_type { };

template <class, typename Enable = void>
struct has_set_length : std::false_type { };
template <class T>
struct has_set_length<T, std::void_t<decltype(std::declval<T>().set_length(0))>> : std::true_type { };

} //end: namespace detail


template <class T> constexpr bool is_length_v = detail::has_length_type<T>::value;

template <class IE, class ENCODER>
constexpr std::size_t field_length(IE const& ie, ENCODER& encoder);

namespace sl {

template <class META_INFO = meta::typelist<>, class IE, class ENCODER>
constexpr std::size_t ie_length(IE const& ie, ENCODER& encoder)
{
	std::size_t len = 0;

	if constexpr (not is_peek_v<IE>)
	{
		if constexpr (not meta::list_is_empty_v<META_INFO>)
		{
			using mi = meta::list_first_t<META_INFO>;
			CODEC_TRACE("%s[%s]: %s", __FUNCTION__, name<IE>(), class_name<mi>());

			if constexpr (mi::kind == mik::TAG)
			{
				len = ie_length(mi{}, encoder);
			}
			else if constexpr (mi::kind == mik::LEN)
			{
				//TODO: remove length_type, handle directly like TAG
				//TODO: involve codec to get length type + may need to set its value like for BER
				using type = typename mi::length_type;
				len = ie_length(type{}, encoder);
			}

			len += ie_length<meta::list_rest_t<META_INFO>>(ie, encoder);
		}
		else //data itself
		{
			using ie_type = typename IE::ie_type;
			//CODEC_TRACE("%s[%.30s]: %s", __FUNCTION__, class_name<IE>(), class_name<ie_type>());
			if constexpr (std::is_base_of_v<CONTAINER, ie_type>)
			{
				len = ie.calc_length(encoder);
				CODEC_TRACE("%s[%s] : len(SEQ) = %zu", __FUNCTION__, name<IE>(), len);
			}
			else if constexpr (is_multi_field_v<IE>)
			{
				for (auto& field : ie)
				{
					if (field.is_set())
					{
						len += field_length(field, encoder);
					}
				}
			}
			else
			{
				len = encoder(GET_LENGTH{}, ie);
				CODEC_TRACE("%s[%s] : len(VAL) = %zu", __FUNCTION__, name<IE>(), len);
			}
		}
	}
	return len;
}

} //end: namespace sl

template <class IE, class ENCODER>
constexpr std::size_t field_length(IE const& ie, ENCODER& encoder)
{
	using mi = meta::produce_info_t<ENCODER, IE>;
	return sl::ie_length<mi>(ie, encoder);
}

namespace detail {

template<class T>
static auto test_value_to_length(int, std::size_t val = 0) ->
	std::enable_if_t<
		std::is_same_v<bool, decltype(T::value_to_length(val))>, std::true_type
	>;

template<class>
static auto test_value_to_length(long) -> std::false_type;

template <class T>
using has_length_converters = decltype(detail::test_value_to_length<T>(0));

}	//end: namespace detail


template <class FIELD>
constexpr void length_to_value(FIELD& field, std::size_t len)
{
	//convert length to raw value if needed
	if constexpr (detail::has_length_converters<FIELD>::value)
	{
		if (not FIELD::length_to_value(len))
		{
			MED_THROW_EXCEPTION(invalid_value, name<FIELD>(), len)
		}
	}

	CODEC_TRACE("L=%zXh [%s]:", len, name<FIELD>());

	//set the length IE with the value
	if constexpr (detail::has_set_length<FIELD>::value)
	{
		if constexpr (std::is_same_v<bool, decltype(field.set_length(len))>)
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
	else if constexpr (std::is_same_v<bool, decltype(field.set_encoded(len))>)
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
}

template <class FIELD>
constexpr void value_to_length(FIELD& field, std::size_t& len)
{
	//use proper length accessor
	if constexpr (detail::has_get_length<FIELD>::value)
	{
		len = field.get_length();
	}
	else
	{
		len = field.get_encoded();
	}

	//convert raw value to length if needed
	if constexpr (detail::has_length_converters<FIELD>::value)
	{
		if (not FIELD::value_to_length(len))
		{
			MED_THROW_EXCEPTION(invalid_value, name<FIELD>(), len)
		}
		CODEC_TRACE("LEN=%zu [%s]", len, name<FIELD>());
	}
}

}	//end: namespace med
