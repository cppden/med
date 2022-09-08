#pragma once
/*
@file
foreach

@copyright Denis Priyomov 2019
Distributed under the MIT License
(See accompanying file LICENSE or visit https://github.com/cppden/ctstring)
*/
#include <functional>

#include "typelist.hpp"

namespace med::meta {

namespace detail {

template <class... Ts> struct foreach;

template <class T0, class... Ts>
struct foreach<T0, Ts...>
{
	template <class F, class... Args>
	static constexpr void exec(F&& f, Args&&... args)
	{
		f.template apply<T0>(std::forward<Args>(args)...);
		foreach<Ts...>::template exec(std::forward<F>(f), std::forward<Args>(args)...);
	}

	template <class PREV, class F, class... Args>
	static constexpr void exec(F&& f, Args&&... args)
	{
		f.template apply<PREV, T0>(std::forward<Args>(args)...);
		foreach<Ts...>::template exec<T0>(std::forward<F>(f), std::forward<Args>(args)...);
	}
};

template <>
struct foreach<>
{
	template <class F, class... Args>
	static constexpr void exec(F&&, Args&&...) {}

	template <class PREV, class F, class... Args>
	static constexpr void exec(F&&, Args&&...) {}
};

} //end: namespace detail

template <class L, class F, class... Ts>
constexpr auto foreach(F&& f, Ts&&... args)
{
	return []
	<template<class...> class List, class... Elems, class Func, class... Args>
	(List<Elems...>&&, Func&& f, Args&&... args)
	{
		return detail::foreach<Elems...>::exec(std::forward<Func>(f), std::forward<Args>(args)...);
	}
	(L{}, std::forward<F>(f), std::forward<Ts>(args)...);
}

template <class L, class F, class... Ts>
constexpr auto foreach_prev(F&& f, Ts&&... args)
{
	return []
	<template<class...> class List, class... Elems, class Func, class... Args>
	(List<Elems...>&&, Func&& f, Args&&... args)
	{
		return detail::foreach<Elems...>::template exec<void>(std::forward<Func>(f), std::forward<Args>(args)...);
	}
	(L{}, std::forward<F>(f), std::forward<Ts>(args)...);
}

/////////////////////////////////////////////

namespace detail {

template <class... Ts>
struct fold;

template <class T0, class... Ts>
struct fold<T0, Ts...>
{
	template <class F, class... Args>
	static constexpr auto exec(F&& f, Args&&... args)
	{
		auto const r1 = f.template apply<T0>(std::forward<Args>(args)...);
		auto const r2 = fold<Ts...>::template exec(std::forward<F>(f), std::forward<Args>(args)...);
		return f.op(r1, r2);
	}
};

template <>
struct fold<>
{
	template <class F, class... Args>
	static constexpr auto exec(F&& f, Args&&... args)
	{
		return f.template apply(std::forward<Args>(args)...);
	}
};

} //end: namespace detail

template <class L, class F, class... Ts>
constexpr auto fold(F&& f, Ts&&... args)
{
	return []
	<template<class...> class List, class... Elems, class Func, class... Args>
	(List<Elems...>&&, Func&& f, Args&&... args)
	{
		return detail::fold<Elems...>::exec(std::forward<Func>(f), std::forward<Args>(args)...);
	}
	(L{}, std::forward<F>(f), std::forward<Ts>(args)...);
}

/////////////////////////////////////////////

namespace detail {

template <class... Ts>
struct for_if;

template <class T0, class... Ts>
struct for_if<T0, Ts...>
{
	template <class F, class... Args>
	static constexpr auto exec(F&& f, Args&&... args)
	{
		if (f.template check<T0>(std::forward<Args>(args)...))
		{
			return f.template apply<T0>(std::forward<Args>(args)...);
		}
		else
		{
			return for_if<Ts...>::template exec(std::forward<F>(f), std::forward<Args>(args)...);
		}
	}

	template <class PREV, class F, class... Args>
	static constexpr auto exec(F&& f, Args&&... args)
	{
		if (f.template check<PREV, T0>(std::forward<Args>(args)...))
		{
			return f.template apply<PREV, T0>(std::forward<Args>(args)...);
		}
		else
		{
			return for_if<Ts...>::template exec<T0>(std::forward<F>(f), std::forward<Args>(args)...);
		}
	}
};

template <>
struct for_if<>
{
	template <class F, class... Args>
	static constexpr auto exec(F&& f, Args&&... args)
	{
		return f.template apply(std::forward<Args>(args)...);
	}

	template <class PREV, class F, class... Args>
	static constexpr auto exec(F&& f, Args&&... args)
	{
		return f.template apply<PREV>(std::forward<Args>(args)...);
	}
};

template <template<class...> class L, class... Elems, class F, class... Args>
constexpr auto for_if_impl(L<Elems...>&&, F&& f, Args&&... args)
{
	return for_if<Elems...>::exec(std::forward<F>(f), std::forward<Args>(args)...);
}

} //end: namespace detail

template <class L, class F, class... Args>
constexpr auto for_if(F&& f, Args&&... args)
{
	return detail::for_if_impl(L{}, std::forward<F>(f), std::forward<Args>(args)...);
}
template <class L, class F, class... Args>
constexpr auto for_if(bool cond, F&& f, Args&&... args)
{
	return cond
		? detail::for_if_impl(L{}, std::forward<F>(f), std::forward<Args>(args)...)
		: detail::for_if_impl(typelist{}, std::forward<F>(f), std::forward<Args>(args)...);
}

} //end: namespace med::meta
