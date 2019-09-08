#pragma once
/*
@file
tiny metaprogramming type list

@copyright Denis Priyomov 2019
Distributed under the MIT License
(See accompanying file LICENSE or visit https://github.com/cppden/ctstring)
*/
#include <type_traits>

namespace med {

namespace detail {

template<bool C> struct if_t;
template<> struct if_t<true>
{
	template<typename THEN, typename ELSE> using type = THEN;
};
template<> struct if_t<false>
{
	template<typename THEN, typename ELSE> using type = ELSE;
};

} //end: namespace detail

template<bool C, typename THEN, typename ELSE>
using conditional_t = typename detail::if_t<C>::template type<THEN, ELSE>;

namespace meta {

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

template <class L> constexpr bool list_is_empty_v = (list_size_v<L> == 0);

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


// sizeof...(Ts) if T not present
template <class T, class... Ts> struct index_of : std::integral_constant<std::size_t, 0> {};
template <class T, class T_1ST, typename... T_REST>
struct index_of<T, T_1ST, T_REST...> : std::integral_constant<std::size_t,
		std::is_same_v<T, T_1ST> ? 0 : index_of<T, T_REST...>::value + 1> {};
template <class T, class... Ts> constexpr std::size_t index_of_v = index_of<T, Ts...>::value;


template <class T, class L> struct list_index_of
{
	static_assert(std::is_same_v<L, list_size<L>>, "not a list");
};
template <class T, template <class...> class L, class... Ts> struct list_index_of<T, L<Ts...>> : index_of<T, Ts...> {};
template <class T, class L> constexpr std::size_t list_index_of_v = list_index_of<T, L>::value;

template <class L> struct list_aligned_union
{
	static_assert(std::is_same_v<L, list_size<L>>, "not a list");
};
template <template <class...> class L, class... Ts> struct list_aligned_union<L<Ts...>> : std::aligned_union<0, Ts...> {};
template <class T> using list_aligned_union_t = typename list_aligned_union<T>::type;

template <typename T = void>
struct wrap
{
	using type = T;
};

template <class T>
using unwrap_t = typename T::type;

} //end: namespace meta
} //end: namespace med

