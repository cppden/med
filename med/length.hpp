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
#include "value_traits.hpp"
#include "name.hpp"
#include "error.hpp"
#include "debug.hpp"


namespace med {

template <class LENGTH>
struct length_t
{
	using length_type = LENGTH;
};

namespace detail {

template <class WRAPPER>
struct length_getter;

template <class, class Enable = void>
struct has_length_type : std::false_type { };
template <class T>
struct has_length_type<T, std::void_t<typename T::length_type>> : std::true_type { };

template <class, typename Enable = void>
struct has_get_length : std::false_type { };
template <class T>
struct has_get_length<T, std::void_t<decltype(std::declval<T>().get_length())>> : std::true_type { };

template <class, typename Enable = void>
struct has_set_length : std::false_type { };
template <class T>
struct has_set_length<T, std::void_t<decltype(std::declval<T>().set_length(0))>> : std::true_type { };

} //end: namespace detail


template <class T>
constexpr bool is_length_v = detail::has_length_type<T>::value;


template <class FIELD>
constexpr std::size_t get_length(FIELD const& field)
{
	return detail::length_getter<FIELD>::get(field);
}
template <class WRAPPER, class FIELD, typename Enable = std::enable_if_t<!std::is_same<WRAPPER, FIELD>::value>>
constexpr std::size_t get_length(FIELD const& field)
{
	return detail::length_getter<WRAPPER>::get(field);
}

namespace detail {

template <class WRAPPER, class FIELD>
constexpr std::size_t calc_length(FIELD const&, IE_NULL const&) { return 0; }

template <class WRAPPER, class FIELD>
constexpr std::size_t calc_length(FIELD const& field, CONTAINER const&)
{
	CODEC_TRACE("calc_length(%s)...", name<FIELD>());
	return field.calc_length();
}

template <class WRAPPER, class FIELD>
constexpr std::size_t calc_length(FIELD const& field, PRIMITIVE const&)
{
	if constexpr (detail::has_get_length<FIELD>::value)
	{
		CODEC_TRACE("length(%s) = %zu", name<FIELD>(), field.get_length());
		return field.get_length();
	}
	else
	{
		//TODO: assuming length in bytes if IE is not customized
		CODEC_TRACE("length(%s) = %zu", name<FIELD>(), bits_to_bytes(FIELD::traits::bits));
		return bits_to_bytes(FIELD::traits::bits);
	}
}

template <class WRAPPER, class FIELD>
constexpr std::size_t calc_length(FIELD const& field, IE_TV const&)
{
	using tag_t = typename WRAPPER::tag_type;
	CODEC_TRACE("calc_length(%s) : TV", name<FIELD>());
	return get_length(tag_t{}) + get_length<FIELD>(ref_field(field));
}

template <class WRAPPER, class FIELD>
constexpr std::size_t calc_length(FIELD const& field, IE_LV const&)
{
	using len_t = typename WRAPPER::length_type;
	CODEC_TRACE("calc_length(%s) : LV", name<FIELD>());
	return get_length(len_t{}) + get_length<FIELD>(ref_field(field));
}

//---------------------------------

template<class T>
static auto test_value_to_length(int, std::size_t val = 0) ->
	std::enable_if_t<
		std::is_same_v<bool, decltype(T::value_to_length(val))>, std::true_type
	>;

template<class>
static auto test_value_to_length(long) -> std::false_type;

template <class WRAPPER>
struct length_getter
{
	template <class FIELD>
	static constexpr std::size_t get(FIELD const& field)
	{
		if constexpr (is_peek_v<WRAPPER>)
		{
			return 0;
		}
		else
		{
			return calc_length<WRAPPER>(field, typename WRAPPER::ie_type{});
		}
	}
};

template <class T>
using has_length_converters = decltype(detail::test_value_to_length<T>(0));

}	//end: namespace detail


template <class FUNC, class FIELD>
constexpr MED_RESULT length_to_value(FUNC& func, FIELD& field, std::size_t len)
{
	//convert length to raw value if needed
	if constexpr (detail::has_length_converters<FIELD>::value)
	{
		if (not FIELD::length_to_value(len))
		{
			return func(error::INVALID_VALUE, name<FIELD>(), len);
		}
	}

	CODEC_TRACE("L=%zx[%s]:", len, name<FIELD>());

	//set the length IE with the value
	if constexpr (detail::has_set_length<FIELD>::value)
	{
		if constexpr (std::is_same_v<bool, decltype(field.set_length(len))>)
		{
			if (not field.set_length(len))
			{
				return func(error::INVALID_VALUE, name<FIELD>(), len);
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
			return func(error::INVALID_VALUE, name<FIELD>(), len);
		}
	}
	else
	{
		field.set_encoded(len);
	}
	MED_RETURN_SUCCESS;
}

template <class FUNC, class FIELD>
constexpr MED_RESULT value_to_length(FUNC& func, FIELD& field, std::size_t& len)
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

	CODEC_TRACE("L=%zx[%s]:", len, name<FIELD>());

	//convert raw value to length if needed
	if constexpr (detail::has_length_converters<FIELD>::value)
	{
		if (not FIELD::value_to_length(len))
		{
			return func(error::INVALID_VALUE, name<FIELD>(), len);
		}
	}
	MED_RETURN_SUCCESS;
}

}	//end: namespace med
