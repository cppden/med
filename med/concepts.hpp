/**
@file
concepts used in the library

@copyright Denis Priyomov 2016-2022
Distributed under the MIT License
(See accompanying file LICENSE or visit https://github.com/cppden/med)
*/

#pragma once

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
	// typename T::field_type;

	// { v.count() } -> std::same_as<std::size_t>;
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

template<class, class, class = void> struct is_setter : std::false_type {};
template<class IE, class FUNC>
struct is_setter<IE, FUNC,
	std::void_t<decltype(std::declval<FUNC>()(std::declval<IE&>(), std::false_type{}))>
> : std::true_type {};

template <class IE, class FUNC>
concept ASetter = is_setter<IE, FUNC>::value;
//TODO: how to make if work w/o enable_if?
// template <class IE, class FUNC>
// concept ASetter = requires(IE& ie, FUNC&& setter)
// {
// 	//requires AField<F>;
// 	{ setter(ie, std::false_type{}) } -> std::same_as<void>;
// };

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

template <class T>
concept AHasDefaultValue = requires(T v)
{
	{ T::traits::default_value + 0 } -> Arithmetic;
};

template <class T>
constexpr T make_identity(T v) { return v; }

template<typename T, typename = void> struct is_predefined : std::false_type {};
template <class T> struct is_predefined<T, std::enable_if_t<T::is_const>> : std::true_type {};

template <class T>
concept APredefined = is_predefined<T>::value;
//TODO: wtf?
// template <class T>
// concept APredefined = requires(T v)
// {
// 	{ make_identity(T::is_const) } -> std::same_as<bool>;
// };

//TODO: how?
template <class, class Enable = void>
struct has_meta_info : std::false_type { };
template <class T>
struct has_meta_info<T, std::enable_if_t<!meta::list_is_empty_v<typename T::meta_info>>> : std::true_type { };

template <class T>
concept AHasMetaInfo = has_meta_info<T>::value;

}	//end: namespace med
