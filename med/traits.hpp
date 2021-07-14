#pragma once

#include "meta/typelist.hpp"

namespace med {

enum class mik //kind of meta-information
{
	TAG,
	LEN,
};

//meta-information holder
template <mik KIND, class INFO>
struct mi : INFO
{
	using info_type = INFO;
	static constexpr mik kind = KIND;
};

template <class... T>
struct add_meta_info
{
	using meta_info = meta::typelist<T...>;
};

template <class, class Enable = void>
struct has_meta_info : std::false_type { };
template <class T>
struct has_meta_info<T, std::enable_if_t<!meta::list_is_empty_v<typename T::meta_info>>> : std::true_type { };
template <class T>
constexpr bool has_meta_info_v = has_meta_info<T>::value;

template <class, class Enable = void>
struct get_meta_info
{
	using type = meta::typelist<>;
};
template <class T>
struct get_meta_info<T, std::void_t<typename T::meta_info>>
{
	using type = typename T::meta_info;
};
template <class T>
struct get_meta_info<T, std::enable_if_t<int(T::kind) >= 0>>
{
	using type = meta::typelist<T>;
};
template <class T> using get_meta_info_t = typename get_meta_info<T>::type;

//template <class... T>
//struct make_meta_info
//{
//	using type = meta::list_append_t<get_meta_info_t<T>...>;
//};
//template <class... T> using make_meta_info_t = typename make_meta_info<T...>::type;

template <class T, class Enable = void>
struct get_meta_tag
{
	using type = void;
};
template <class L>
struct get_meta_tag<L, std::enable_if_t<meta::list_is_empty_v<L>>>
{
	using type = void;
};
template <class L>
struct get_meta_tag<L, std::enable_if_t<!meta::list_is_empty_v<L>>>
{
	using type = conditional_t<meta::list_first_t<L>::kind == mik::TAG, meta::list_first_t<L>, void>;
};
template <class T> using get_meta_tag_t = typename get_meta_tag<T>::type;

} //end: namespace med
