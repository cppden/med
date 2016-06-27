/*!
@file
TODO: define.

@copyright Denis Priyomov 2016
Distributed under the MIT License
(See accompanying file LICENSE or copy at https://opensource.org/licenses/MIT)
*/

#pragma once

#include <utility>
#include <type_traits>

#include "functional.hpp"

namespace med {

template <class IE>
std::enable_if_t<is_optional_v<IE>, bool>
constexpr check_arity(std::size_t count)    { return 0 == count || count >= IE::min; }

template <class IE>
std::enable_if_t<!is_optional_v<IE>, bool>
constexpr check_arity(std::size_t count)    { return count >= IE::min; }

template<typename T, class Enable = void>
struct is_count_getter : std::false_type { };

template<typename T>
struct is_count_getter<T, std::enable_if_t<
		std::is_unsigned< decltype( std::declval<T>()(std::false_type{}) ) >::value
	>
> : std::true_type { };


namespace detail {

template <class IE>
constexpr std::size_t invoke_count(IE&, std::size_t count) { return count; }

template <class IE, std::size_t INDEX, std::size_t... Is>
inline std::size_t invoke_count(IE& ie, std::size_t count)
{
	if (ie.template ref_field<INDEX>().is_set())
	{
		return invoke_count<IE, Is...>(ie, INDEX+1);
	}
	return INDEX;
}

template <class IE, std::size_t... Is>
inline std::size_t get_field_count(IE& ie, std::index_sequence<Is...>)
{
	return invoke_count<IE, Is...>(ie, 0);
}

}	//end: namespace detail

template <class FIELD>
std::enable_if_t<has_multi_field<FIELD>::value, std::size_t>
inline field_count(FIELD const& ie)
{
	return detail::get_field_count(ie, std::make_index_sequence<FIELD::max>{});
}

template <class FIELD>
std::enable_if_t<!has_multi_field<FIELD>::value, std::size_t>
inline field_count(FIELD const& field) { return field.is_set() ? 1 : 0; }


template <class T>
constexpr bool is_count_getter_v = is_count_getter<T>::value;


template <class, class Enable = void >
struct has_count_getter : std::false_type { };

template <class T>
struct has_count_getter<T, void_t<typename T::count_getter>> : std::true_type { };

template <class T>
constexpr bool has_count_getter_v = has_count_getter<T>::value;

} //namespace med
