/*!
@file
TODO: define.

@copyright Denis Priyomov 2016
Distributed under the MIT License
(See accompanying file LICENSE or copy at https://opensource.org/licenses/MIT)
*/

#pragma once

#include <type_traits>

#include "ie_type.hpp"


namespace med {

template <class TAG>
struct tag_t
{
	using tag_type = TAG;
};

template <class T, class Enable = void>
struct is_tag : std::false_type {};
template <class T>
struct is_tag<T, std::enable_if_t<std::is_const<typename T::ie_type>::value>> : std::true_type {};


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
struct has_get_tag<T, void_t<decltype(std::declval<T>().get_tag())>> : std::true_type { };


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
std::enable_if_t<has_get_tag<T>::value, std::size_t>
inline get_tag(T const& header)
{
	return header.get_tag();
}

template <class T>
std::enable_if_t<!has_get_tag<T>::value, std::size_t>
inline get_tag(T const& header)
{
	return header.get();
}

template <class T>
std::enable_if_t<has_get_tag<T>::value, bool>
inline set_tag(T& header, std::size_t tag)
{
	if (header.get_tag() == tag) { return false; }
	header.set_tag(tag);
	return true;
}

template <class T>
std::enable_if_t<!has_get_tag<T>::value, bool>
inline set_tag(T& header, std::size_t tag)
{
	if (header.get() == tag) { return false; }
	header.set(tag);
	return true;
}

}	//end: namespace med