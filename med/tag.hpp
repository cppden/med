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
struct is_tag<T, std::enable_if_t<std::is_const<typename T::ie_type>::value>> : std::true_type {};
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
struct has_tag_type<T, std::enable_if_t<!std::is_same<void, typename T::tag_type>::value>> : std::true_type { };
template <class T>
constexpr bool has_tag_type_v = has_tag_type<T>::value;

//for using in choice
template <class TAG_TYPE, class CASE = void>
struct tag : tag_t<TAG_TYPE>
{
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


template <class T>
constexpr auto get_tag(T const& header) -> decltype(std::declval<T>().get_tag())
{
	return header.get_tag();
}

template <class T>
constexpr auto get_tag(T const& header, std::enable_if_t<!has_get_tag<T>::value>* = nullptr)
{
	return header.get_encoded();
}

template <class T, class TV>
std::enable_if_t<has_get_tag<T>::value, bool>
inline set_tag(T& header, TV&& tag)
{
	if (header.is_set() && header.get_tag() == tag)
	{
		return false;
	}
	header.set_tag(tag);
	return true;
}

template <class T, class TV>
std::enable_if_t<!has_get_tag<T>::value, bool>
inline set_tag(T& header, TV&& tag)
{
	if (header.is_set() && header.get_encoded() == tag)
	{
		return false;
	}
	header.set_encoded(tag);
	return true;
}

}	//end: namespace med
