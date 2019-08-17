#pragma once
/*
@file
tiny metaprogramming type list

@copyright Denis Priyomov 2019
Distributed under the MIT License
(See accompanying file LICENSE or visit https://github.com/cppden/ctstring)
*/
#include <type_traits>

namespace med::meta {

template <class... T> struct typelist {};

/* --- list size --- */
template <class L> struct list_size
{
	static_assert(std::is_same_v<L, list_size<L>>, "not a list");
};
template <template <class...> class L, class... T> struct list_size<L<T...>>
{
	static constexpr std::size_t value = sizeof...(T);
};
template <class L> constexpr std::size_t list_size_v = list_size<L>::value;

template <class L> constexpr bool is_list_empty_v = (list_size_v<L> == 0);

/* --- get/pop front type of list --- */
template <class L> struct list_first
{
	static_assert(std::is_same_v<L, list_first<L>>, "empty or not a list");
};
template <template <class...> class L, class T1, class... T> struct list_first<L<T1, T...>>
{
	using type = T1;
	using rest = L<T...>;
};
template <class L> using list_first_t = typename list_first<L>::type;
template <class L> using list_rest_t = typename list_first<L>::rest;

/* --- push front/back a type in list --- */
template <class L, class... T> struct list_push
{
	static_assert(std::is_same_v<L, list_push<L>>, "not a list");
};
template <template <class...> class L, class... U, class... T> struct list_push<L<U...>, T...>
{
	using front = L<T..., U...>;
	using back = L<U..., T...>;
};
template <class L, class... T> using list_push_front_t = typename list_push<L, T...>::front;
template <class L, class... T> using list_push_back_t = typename list_push<L, T...>::back;

template <class L1 = typelist<>, class L2 = typelist<>, class L3 = typelist<>> struct list_append;
template <
	template<class...> class L1, class... T1,
	template<class...> class L2, class... T2,
	template<class...> class L3, class... T3>
struct list_append<L1<T1...>, L2<T2...>, L3<T3...>>
{
	using type = L1<T1..., T2..., T3...>;
};
template <class... L> using list_append_t = typename list_append<L...>::type;


template <typename T>
struct wrap
{
	using type = T;
};

template <class T>
using unwrap_t = typename T::type;

} //end: namespace med::meta

