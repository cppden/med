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
//TODO: remove
#include "sl/octet_info.hpp"


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


//for choice
template <class TAG_TYPE, class CASE>
struct tag : tag_t<TAG_TYPE>
{
	static_assert(std::is_class_v<CASE>, "CASE IS REQUIRED TO BE A CLASS");
	using case_type = CASE;
};


//template <class IE, typename Enable = void>
//struct tag_type;

//template <class IE>
//struct tag_type<IE, std::enable_if_t<sl::detail::has_tag_type<IE>::value>>
//{
//	using type = typename IE::tag_type;
//};

//template <class IE>
//struct tag_type<IE, std::enable_if_t<!sl::detail::has_tag_type<IE>::value && sl::detail::has_tag_type<typename IE::field_type>::value>>
//{
//	using type = typename IE::field_type::tag_type;
//};


namespace detail {

template <class, class Enable = void>
struct has_get_tag : std::false_type { };
template <class T>
struct has_get_tag<T, std::void_t<decltype(std::declval<T>().get_tag())>> : std::true_type { };

} //end: namespace detail

template <class T>
constexpr auto get_tag(T const& header)
{
	if constexpr (detail::has_get_tag<T>::value)
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
	if constexpr (detail::has_get_tag<T>::value)
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
