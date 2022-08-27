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
	if constexpr (AOptional<IE>)
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


template <class FIELD>
constexpr std::size_t field_count(FIELD const& field)
{
	if constexpr (AHasCount<FIELD>)
	{
		return field.count();
	}
	else
	{
		return field.is_set() ? 1 : 0;
	}
}

} //namespace med
