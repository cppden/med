/**
@file
tag related definitions

@copyright Denis Priyomov 2016-2017
Distributed under the MIT License
(See accompanying file LICENSE or visit https://github.com/cppden/med)
*/

#pragma once

#include <type_traits>

#include "ie_type.hpp"


namespace med {

template <class T, class Enable = void>
struct is_tag : std::false_type {};
template <class T>
struct is_tag<T, std::enable_if_t<std::is_same_v<bool, decltype(((T const*)0)->match(0))>>> : std::true_type {};

template <class T>
constexpr bool is_tag_v = is_tag<T>::value;

template <class TAG>
struct tag_t
{
	using tag_type = TAG;
	static_assert(is_tag_v<TAG>, "TAG IS REQUIRED");
};

template <class, class = void >
struct has_tag_type : std::false_type { };
template <class T>
struct has_tag_type<T, std::void_t<typename T::tag_type>> : std::true_type { };
template <class T>
constexpr bool has_tag_type_v = has_tag_type<T>::value;

//for choice
template <class TAG_TYPE, class CASE>
struct tag : tag_t<TAG_TYPE>
{
	static_assert(std::is_class_v<CASE>, "CASE IS REQUIRED TO BE A CLASS");
	using case_type = CASE;
};

template <class, class Enable = void>
struct has_get_tag : std::false_type { };

template <class T>
struct has_get_tag<T, std::void_t<decltype(std::declval<T>().get_tag())>> : std::true_type { };


template <class IE, typename Enable = void>
struct tag_type;

template <class IE>
struct tag_type<IE, std::enable_if_t<has_tag_type<IE>::value>>
{
	using type = typename IE::tag_type;
};

template <class IE>
struct tag_type<IE, std::enable_if_t<!has_tag_type<IE>::value && has_tag_type<typename IE::field_type>::value>>
{
	using type = typename IE::field_type::tag_type;
};

template <class T>
using tag_type_t = typename tag_type<T>::type;

//used to check that tags in choice/set are unique
//TODO: filter out non-fixed tags since the current way is faster to compile
//      but doesn't allow to have multiple non-fixed tags
template <class T, typename Enable = void>
struct tag_value_get
{
	static constexpr auto value = nullptr;
};
template <class T>
struct tag_value_get<T, std::enable_if_t<not std::is_void_v<decltype(tag_type<T>::type::get())>>>
{
	static constexpr auto value = tag_type<T>::type::get();
};

template <class T>
constexpr auto get_tag(T const& header)
{
	if constexpr (has_get_tag<T>::value)
	{
		return header.get_tag();
	}
	else
	{
		return header.get_encoded();
	}
}

template <class T, class TV>
inline bool set_tag(T& header, TV&& tag)
{
	if constexpr (has_get_tag<T>::value)
	{
		if (header.is_set() && header.get_tag() == tag)
		{
			return false;
		}
		header.set_tag(tag);
	}
	else
	{
		if (header.is_set() && header.get_encoded() == tag)
		{
			return false;
		}
		header.set_encoded(tag);
	}
	return true;
}


}	//end: namespace med
