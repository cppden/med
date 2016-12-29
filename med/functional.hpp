/*!
@file
TODO: define.

@copyright Denis Priyomov 2016
Distributed under the MIT License
(See accompanying file LICENSE or copy at https://opensource.org/licenses/MIT)
*/

#pragma once

#include <type_traits>
#include "void.hpp"

namespace med {

template <typename T>
struct remove_cref : std::remove_reference<std::remove_const_t<T>> {};
template <typename T>
using remove_cref_t = typename remove_cref<T>::type;


template <class, class Enable = void>
struct has_ie_type : std::false_type { };

template <class T>
struct has_ie_type<T, void_t<typename T::ie_type>> : std::true_type { };

template <class, class Enable = void>
struct has_field_type : std::false_type { };
template <class T>
struct has_field_type<T, void_t<typename T::field_type>> : std::true_type { };


template <class T, typename Enable = void>
struct get_field_type
{
	using type = T;
};
template <class T>
struct get_field_type<T, std::enable_if_t<has_field_type<T>::value>>
{
	using type = typename get_field_type<typename T::field_type>::type;
};
template <class T>
using get_field_type_t = typename get_field_type<T>::type;


struct optional_t {};

template <class T>
constexpr bool is_optional_v = std::is_base_of<optional_t, T>::value;


template<typename T, typename Enable = void>
struct is_set_function : std::false_type { };
template<typename T>
struct is_set_function<T,
	std::enable_if_t<
		std::is_same<bool, decltype(std::declval<T>()(std::false_type{}))>::value>
	> : std::true_type
{
};
template <class T>
constexpr bool is_set_function_v = is_set_function<T>::value;


template <class, class Enable = void>
struct has_optional_type : std::false_type { };
template <class T>
struct has_optional_type<T, void_t<typename T::optional_type>> : std::true_type { };
template <class T>
constexpr bool has_optional_type_v = has_optional_type<T>::value;


namespace detail {

template<class T>
static auto test_setter(int, std::false_type val = std::false_type{}) ->
	std::enable_if_t<
		std::is_same< void, decltype( std::declval<T>()(val) ) >::value, std::true_type
	>;

template<class>
static auto test_setter(long) -> std::false_type;

}	//end: namespace detail

template <class T>
struct is_setter : decltype(detail::test_setter<T>(0)) { };
template <class T>
constexpr bool is_setter_v = is_setter<T>::value;

template <class, class Enable = void >
struct has_setter_type : std::false_type { };
template <class T>
struct has_setter_type<T, void_t<typename T::setter_type>> : std::true_type { };
template <class T>
constexpr bool has_setter_type_v = has_setter_type<T>::value;

}	//end: namespace med
