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
#include "meta/typelist.hpp"


namespace med {

template <class T, class Enable = void>
struct is_tag : std::false_type {};
template <class T>
struct is_tag<T, std::enable_if_t<std::is_same_v<bool, decltype(((T const*)0)->match(0))>>> : std::true_type {};

template <class T>
constexpr bool is_tag_v = is_tag<T>::value;

//define tag_type with assertion on type
template <class TAG_TYPE>
struct tag_t
{
	using tag_type = TAG_TYPE;
	static_assert(is_tag_v<TAG_TYPE>, "TAG IS REQUIRED");
};

template <class FUNC>
struct tag_getter
{
	template <class T>
	using apply = meta::unwrap_t<decltype(FUNC::template get_tag_type<T>())>;
};

//for choice
template <class T_VALUE, class TYPE>
struct option
{
	static_assert(is_tag_v<T_VALUE>, "TAG IS REQUIRED");
	static_assert(std::is_class_v<TYPE>, "CASE IS REQUIRED TO BE A CLASS");
	using option_value_type = T_VALUE;
	using option_type = TYPE;
};

struct option_getter
{
	template <class T>
	using apply = typename T::option_value_type;
};


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
