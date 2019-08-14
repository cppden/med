#pragma once

/**
@file
helper classes to detect and prevent duplicate tags in choice and set.

@copyright Denis Priyomov 2016-2019
Distributed under the MIT License
(See accompanying file LICENSE or visit https://github.com/cppden/med)
*/

#include <utility>
#include <type_traits>

namespace med::meta {

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

namespace detail {

template <class getter, class IE1, class IE2, typename E = void>
struct is_duplicate
{
	static constexpr auto value = false;
	using type = void;
};

template <class getter, class IE1, class IE2>
struct is_duplicate<getter, IE1, IE2,
		std::void_t<decltype(getter::template apply<IE1>::get()), decltype(getter::template apply<IE2>::get())>>
{
	static constexpr auto value = getter::template apply<IE1>::get() == getter::template apply<IE2>::get();
	using type = std::pair<IE1, IE2>;
};

struct fallback
{
	static constexpr bool value = true;
	using type = void;
};

template <class getter, class IE, class TL>
struct same;

template <class getter, class IE, template<class...> class L, class... IEs>
struct same<getter, IE, L<IEs...>> : std::disjunction<is_duplicate<getter, IE, IEs>..., fallback>
{};

template <class T> struct clash;
template <> struct clash<void>
{
	static constexpr void error() {}
};

} //end: namespace detail

template <class getter, class L>
struct unique;
template <class getter, class L>
using unique_t = typename unique<getter, L>::type;

template <class getter, template<class...> class L>
struct unique<getter, L<>>
{
	using type = void;
};


template <class getter, template<class...> class L, class T1, class... T>
struct unique<getter, L<T1, T...>>
{
	using clashed_tags = decltype(detail::clash<typename detail::same<getter, T1, L<T...>>::type>::error());
	using type = std::enable_if_t<std::is_void_v<clashed_tags>, unique_t<getter, L<T...>>>;
};

} //namespace med::meta
