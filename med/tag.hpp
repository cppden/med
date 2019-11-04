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

template <class T, class = void> struct is_tag : std::false_type {};
template <class T> struct is_tag<T, std::enable_if_t< std::is_same_v<bool, decltype(T::match(0))> >> : std::true_type {};
template <class T> constexpr bool is_tag_v = is_tag<T>::value;

template <class FUNC>
struct tag_getter
{
	//TODO: skip if not tag?
	template <class T>
	using apply = meta::list_first_t<meta::produce_info_t<FUNC, T>>;
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

}	//end: namespace med
