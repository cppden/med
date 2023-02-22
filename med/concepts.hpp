/**
@file
concepts used in the library

@copyright Denis Priyomov 2016-2022
Distributed under the MIT License
(See accompanying file LICENSE or visit https://github.com/cppden/med)
*/

#pragma once

#include "meta/typelist.hpp"

#include <type_traits>
#include <concepts>

namespace med {

template<typename T>
concept APointer = std::is_pointer_v<T>;

template <class T>
concept AHasSize = requires(T const& v)
{
	{ v.size() } -> std::integral;
};

template <class T>
concept ADataContainer = AHasSize<T> && requires(T v)
{
	{ v.data() } -> APointer;
};

template <class T>
concept ALength = requires(T v)
{
	typename T::length_type;
};

template <class T>
concept AHasSetLength = requires(T& v)
{
	{ v.set_length(0) };
};

template <class T>
concept AHasIeType = requires(T v)
{
	typename T::ie_type;
};


template <class T>
concept AField = requires(T v)
{
	//TODO: why???
	//typename T::value_type;
	{ v.is_set() } -> std::same_as<bool>;
	{ v.clear() };
	//TODO: support strings or numeric-values
	//{ v.set_encoded(std::declval<typename T::value_type>()) } -> std::same_as<void>;
	//{ v.get_encoded() } -> std::same_as<typename T::value_type>;
};

template <class T>
concept AMultiField = requires(T v)
{
	typename T::field_value;
	{ v.count() } -> std::unsigned_integral;
	// { v.is_set() } -> std::same_as<bool>;
	// { v.empty() } -> std::same_as<bool>;
	// { v.clear() };
	// { v.pop_back() };
	// { v.push_back() } -> std::same_as<typename T::field_type>;
};

//marker for the optional IEs
struct optional_t {};

template <class T>
concept AOptional = std::is_base_of_v<optional_t, T>;
template <class T>
concept AMandatory = !std::is_base_of_v<optional_t, T>;

//checks if T looks like a functor to test condition of a field presense
template <typename T>
concept ACondition = requires(T v)
{
	{ v(std::false_type{}) } -> std::same_as<bool>;
};

template <class T>
concept AHasCondition = requires(T v)
{
	typename T::condition;
};

// user-provided functor to extract field count
template<typename T>
concept ACountGetter = requires(T v)
{
	{ v(std::false_type{}) } -> std::unsigned_integral;
};

template <class T>
concept ACounter = requires(T v)
{
	typename T::counter_type;
};

template <class T>
concept AHasCountGetter = requires(T v)
{
	typename T::count_getter;
};

template <class T>
concept AHasCount = requires(T const& v)
{
	{ v.count() } -> std::unsigned_integral;
};

template <class IE, class FUNC>
concept ASetter = AField<IE> && requires(IE& ie, FUNC setter)
{
	setter(ie, std::false_type{});
};

template <class T>
concept AHasSetterType = requires(T v)
{
	typename T::setter_type;
};

template <class T>
concept ATag = requires(T v)
{
	{ T::match(0) } -> std::same_as<bool>;
};

template <class T>
concept AHasGetTag = requires(T const& v)
{
	{ v.get_tag() } -> std::convertible_to<std::size_t>;
};

template <class T>
concept Arithmetic = std::floating_point<T> || std::integral<T>;

template <class T>
concept AValueTraits = requires (T v)
{
	typename T::value_type;
	{ T::bits + 0 } -> std::unsigned_integral;
};

template <class L>
concept AEmptyTypeList = meta::list_is_empty_v<L>;

template <class T> concept APredefinedValue = (T::is_defined == true);

template <class T>
concept AHasMetaInfo = !AEmptyTypeList<typename T::meta_info>;

template <class T>
concept AAllocator = requires (T v)
{
	{ v.allocate(std::size_t{}, std::size_t{}) } -> std::convertible_to<void*>;
};

template <class NEEDLE, class HAY>
concept APresentIn = HAY::template has<NEEDLE>();

template <class T, class... Ts>
concept ASameAs = (std::is_same_v<T, Ts> || ...);

}	//end: namespace med
