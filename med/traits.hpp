#pragma once

#include "concepts.hpp"

namespace med {

enum class mik //kind of meta-information
{
	TAG,
	LEN,
};

//meta-information holder
template <mik KIND, class INFO>
struct mi
{
	using info_type = INFO;
	static constexpr mik kind = KIND;
};

template <class... T>
struct add_meta_info
{
	using meta_info = meta::typelist<T...>;
};

template <class T>
struct get_meta_info
{
	using type = meta::typelist<>;
};
template <class T> requires requires(T v) { typename T::meta_info; }
struct get_meta_info<T>
{
	using type = typename T::meta_info;
};
template <class T> concept AKind = std::same_as<mik const, decltype(T::kind)>;
template <AKind T>
struct get_meta_info<T>
{
	using type = meta::typelist<T>;
};
template <class T> using get_meta_info_t = typename get_meta_info<T>::type;

template <class L>
struct get_meta_tag
{
	using type = void;
};

template <class L> requires (!AEmptyTypeList<L>)
struct get_meta_tag<L>
{
	using type = conditional_t<meta::list_first_t<L>::kind == mik::TAG, typename meta::list_first_t<L>::info_type, void>;
};
template <class T> using get_meta_tag_t = typename get_meta_tag<T>::type;

} //end: namespace med
