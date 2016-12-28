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


template <class COUNTER>
struct counter_t
{
	using counter_type = COUNTER;
};

template <class, class Enable = void>
struct has_counter_type : std::false_type { };

template <class T>
struct has_counter_type<T, void_t<typename T::counter_type>> : std::true_type { };

template <class T>
using is_counter = has_counter_type<T>;

template <class T>
constexpr bool is_counter_v = is_counter<T>::value;



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

template <class FIELD>
std::enable_if_t<has_multi_field<FIELD>::value, std::size_t>
inline field_count(FIELD const& ie)
{
	//return detail::get_field_count(ie, std::make_index_sequence<FIELD::max>{});
	std::size_t count = 0;
	for (auto it = ie.begin(), ite = ie.end(); it != ite; ++it)
	{
		++count;
	}
	return count;
}

template <class FIELD>
std::enable_if_t<!has_multi_field<FIELD>::value, std::size_t>
inline field_count(FIELD const& field)
{
	return field.is_set() ? 1 : 0;
}

// user-provided functor to etract field count
template <class T>
constexpr bool is_count_getter_v = is_count_getter<T>::value;


template <class, class Enable = void >
struct has_count_getter : std::false_type { };

template <class T>
struct has_count_getter<T, void_t<typename T::count_getter>> : std::true_type { };

template <class T>
constexpr bool has_count_getter_v = has_count_getter<T>::value;

} //namespace med
