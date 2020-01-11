#pragma once
/*
@file
tiny metaprogramming type list

@copyright Denis Priyomov 2019
Distributed under the MIT License
(See accompanying file LICENSE or visit https://github.com/cppden/ctstring)
*/
#include <type_traits>
#include <utility>

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

template <class... T> struct typelist{};

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
	using type = list_first; //to allow instantiation of derived templates showing propper error
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


/* --- append lists of types into one --- */
template <class... Ts> struct list_append;
template <class T> struct list_append<T> { using type = T; };
template <
	template<class...> class L1, class... T1,
	template<class...> class L2, class... T2,
	class... Ts>
struct list_append<L1<T1...>, L2<T2...>, Ts...> : list_append<L1<T1..., T2...>, Ts...> {};
template <class... L> using list_append_t = typename list_append<L...>::type;

/* --- interleave two lists of types --- */
template <class L1, class L2> struct interleave;

template <
	template <class...> class L1, class... T1,
	template <class...> class L2, class... T2>
struct interleave<L1<T1...>, L2<T2...>>
{
	using type = list_append_t<L1<T1, T2>...>;
};
//interleave list of types with a type
template <
	template <class...> class L1, class... T1,
	class T2>
struct interleave<L1<T1...>, T2>
{
	using type = list_append_t<L1<T1, T2>...>;
};

template <class L1, class L2>
using interleave_t = typename interleave<L1, L2>::type;

/* --- transform list of types by meta-functor --- */
template<class L, template<class...> class MF> struct transform;
template<
	template<class...> class L,
	template<class...> class MF, class... Ts>
struct transform<L<Ts...>, MF> {
	using type = L<typename MF<Ts>::type...>;
};

template<class L, template<class...> class MF>
using transform_t = typename transform<L, MF>::type;

//transform with indexes (don't like it but no need to generalize yet)
namespace detail {

template <std::size_t I, std::size_t N>
struct int_index
{
	using type = std::integral_constant<std::size_t, I>;
};

template<class L, class I, template<class...> class MF, template<std::size_t, std::size_t> class F> struct transform_indexed;
template<
	template<class...> class L,
	template<class...> class MF, class... Ts,
	std::size_t... I,
	template<std::size_t, std::size_t> class F
>
struct transform_indexed<L<Ts...>, std::index_sequence<I...>, MF, F> {
	using type = L<
		typename MF<Ts, typename F<I, med::meta::list_size_v<L<Ts...>>>::type>::type...
	>;
};

} //end: namespace detail

template<
	class L,
	template<class...> class MF,
	template<std::size_t, std::size_t> class F = detail::int_index>
using transform_indexed_t = typename detail::transform_indexed<L, std::make_index_sequence<med::meta::list_size_v<L>>, MF, F>::type;

/* --- get index of type in list or sizeof...(Ts) if T not present --- */
namespace detail {

template <class T, class... Ts> struct index_of : std::integral_constant<std::size_t, 0> {};
template <class T, class T_1ST, typename... T_REST>
struct index_of<T, T_1ST, T_REST...> : std::integral_constant<std::size_t,
		std::is_same_v<T, T_1ST> ? 0 : index_of<T, T_REST...>::value + 1> {};

} //end: namespace detail

template <class T, class L> struct list_index_of
{
	static_assert(std::is_same_v<L, list_size<L>>, "not a list");
};
template <class T, template <class...> class L, class... Ts> struct list_index_of<T, L<Ts...>> : detail::index_of<T, Ts...> {};
template <class T, class L> constexpr std::size_t list_index_of_v = list_index_of<T, L>::value;


template <typename T = void>
struct wrap
{
	using type = T;
};

template <class T>
using unwrap_t = typename T::type;

template <class F, class T>
using produce_info_t = unwrap_t<decltype(std::remove_reference_t<F>::template produce_meta_info<T>())>;

} //end: namespace meta
} //end: namespace med

