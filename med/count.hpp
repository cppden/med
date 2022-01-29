/**
@file
counter related primitives

@copyright Denis Priyomov 2016-2017
Distributed under the MIT License
(See accompanying file LICENSE or visit https://github.com/cppden/med)
*/

#pragma once

#include <utility>
#include <type_traits>

#include "config.hpp"
#include "functional.hpp"
#include "name.hpp"
#include "exception.hpp"

namespace med {


template <class COUNTER>
struct counter_t
{
	using counter_type = COUNTER;
};

template <class, class Enable = void>
struct has_counter_type : std::false_type { };

template <class T>
struct has_counter_type<T, std::void_t<typename T::counter_type>> : std::true_type { };

template <class T>
using is_counter = has_counter_type<T>;

template <class T>
constexpr bool is_counter_v = is_counter<T>::value;

namespace detail {

template <class FUNC, class IE>
constexpr void check_n_arity(FUNC&, IE const&, std::size_t count)
{
	if (count >= IE::min)
	{
		if (count > IE::max)
		{
			MED_THROW_EXCEPTION(extra_ie, name<IE>(), IE::max, count)
		}
	}
	else
	{
		MED_THROW_EXCEPTION(missing_ie, name<IE>(), IE::min, count)
	}
}

} //end: namespace detail

//multi-field
template <class FUNC, class IE>
constexpr void check_arity(FUNC& func, IE const& ie, std::size_t count)
{
	if constexpr (is_optional_v<IE>)
	{
		if (count) { detail::check_n_arity(func, ie, count); }
	}
	else
	{
		detail::check_n_arity(func, ie, count);
	}
}

template <class FUNC, class IE>
constexpr void check_arity(FUNC& func, IE const& ie)
{
	check_arity(func, ie, ie.count());
}

// user-provided functor to etract field count
template<typename T, class Enable = void>
struct is_count_getter : std::false_type { };
template<typename T>
struct is_count_getter<T, std::enable_if_t<
		std::is_unsigned_v<decltype( std::declval<T>()(std::false_type{}) )>
	>
> : std::true_type { };
template <class T>
constexpr bool is_count_getter_v = is_count_getter<T>::value;


template <class, class Enable = void >
struct has_count_getter : std::false_type { };
template <class T>
struct has_count_getter<T, std::void_t<typename T::count_getter>> : std::true_type { };
template <class T>
constexpr bool has_count_getter_v = has_count_getter<T>::value;

//T::count checker
template <class, class Enable = void >
struct has_count : std::false_type { };
template <class T>
struct has_count<T, std::enable_if_t<std::is_same_v<std::size_t, decltype(std::declval<T const>().count())>>> : std::true_type { };
template <class T>
constexpr bool has_count_v = has_count<T>::value;

template <class FIELD>
constexpr std::size_t field_count(FIELD const& field)
{
	if constexpr (has_count_v<FIELD>)
	{
		return field.count();
	}
	else
	{
		return field.is_set() ? 1 : 0;
	}
}

} //namespace med
