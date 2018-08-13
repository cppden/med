#pragma once

/**
@file
copy a field.

@copyright Denis Priyomov 2016-2018
Distributed under the MIT License
(See accompanying file LICENSE or visit https://github.com/cppden/med)
*/

#include <utility>
#include <type_traits>

namespace med::util {

namespace detail {

template <class Enable, class... T>
struct all_ne;

template <class T1, class... T>
struct all_ne<std::enable_if_t<not std::is_same_v<std::nullptr_t, T1>>, T1, T...>
{
	template <class T0>
	static constexpr bool apply(T0 base, T1 val1, T... vals)
	{
		return base != static_cast<T0>(val1) && all_ne<void, T...>::apply(base, vals...);
	}
};

template <class... T>
struct all_ne<void, std::nullptr_t, T...>
{
	template <class T0>
	static constexpr bool apply(T0 base, std::nullptr_t, T... vals)
	{
		return all_ne<void, T...>::apply(base, vals...);
	}
};

template <>
struct all_ne<void>
{
	template <class T0>
	static constexpr bool apply(T0)
	{
		return true;
	}
};


template <class Enable, class... T>
struct unique_values;

template <class T1, class... T>
struct unique_values<std::enable_if_t<not std::is_same_v<std::nullptr_t, T1>>, T1, T...>
{
	static constexpr bool apply(T1 val, T... vals)
	{
		return all_ne<void, T...>::apply(val, vals...) && unique_values<void, T...>::apply(vals...);
	}
};

template <class T1, class... T>
struct unique_values<std::enable_if_t<std::is_same_v<std::nullptr_t, T1>>, T1, T...>
{
	static constexpr bool apply(std::nullptr_t, T... vals)
	{
		return unique_values<void, T...>::apply(vals...);
	}
};

template <class T1>
struct unique_values<std::enable_if_t<not std::is_same_v<std::nullptr_t, T1>>, T1>
{
	static constexpr bool apply(T1)
	{
		return true;
	}
};

template <>
struct unique_values<void>
{
	static constexpr bool apply()
	{
		return true;
	}
};


} //end: namespace detail

template <typename... T>
constexpr bool are_unique(T... vals)
{
	return detail::unique_values<void, T...>::apply(vals...);
}

} //namespace med::util
