#pragma once

/**
@file
copy a field.

@copyright Denis Priyomov 2016-2018
Distributed under the MIT License
(See accompanying file LICENSE or visit https://github.com/cppden/med)
*/

#include <type_traits>

namespace med::util {

namespace detail {

template <typename... BOOLS>
constexpr bool all(BOOLS... bools)
{
	return (... && bools);
}

template <typename T1>
constexpr bool unique_values(T1&&)
{
	return true;
}

template <typename T1, typename... T>
constexpr bool unique_values(T1&& val, T&&... vals)
{
	return detail::all(val != vals...) && unique_values(std::forward<T>(vals)...);
}

} //end: namespace detail

template <typename... T>
constexpr bool are_unique(T&&... vals)
{
	return detail::unique_values(std::forward<T>(vals)...);
}

} //namespace med::util
