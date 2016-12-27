/*!
@file
TODO: define.

@copyright Denis Priyomov 2016
Distributed under the MIT License
(See accompanying file LICENSE or copy at https://opensource.org/licenses/MIT)
*/

#pragma once

#include <limits>
#include <iterator>

#include "ie_type.hpp"
#include "tag.hpp"
#include "length.hpp"

namespace med {

template <std::size_t MAX>
struct max : std::integral_constant<std::size_t, MAX> {};

template <std::size_t MIN>
struct min : std::integral_constant<std::size_t, MIN> {};

template <std::size_t N>
struct arity : std::integral_constant<std::size_t, N> {};

template <std::size_t N>
struct inplace : std::integral_constant<std::size_t, N> {};

constexpr std::size_t inf = std::numeric_limits<std::size_t>::max();


template <typename PTR>
class buffer;

template <typename T>
struct remove_cref : std::remove_reference<std::remove_const_t<T>> {};
template <typename T>
using remove_cref_t = typename remove_cref<T>::type;


template <class FIELD>
struct field_t : FIELD
{
	using field_type = FIELD;

	field_type& ref_field()             { return *this; }
	field_type const& ref_field() const { return *this; }
};

template <class TAG, class VAL>
struct tag_value_t : field_t<VAL>, tag_t<TAG>
{
	using ie_type = IE_TV;
};

template <class LEN, class VAL>
struct length_value_t : field_t<VAL>, LEN
{
	using ie_type = IE_LV;
};


template <class T, class Enable = void>
struct is_field : std::false_type {};
template <class T>
struct is_field<T, std::enable_if_t<
	std::is_same<bool, decltype(std::declval<T>().is_set())>::value
	>
> : std::true_type {};

template <class T>
constexpr bool is_field_v = is_field<T>::value;


template <class T, class Enable = void>
struct is_tagged_field : std::false_type {};
template <class T>
struct is_tagged_field<T, std::enable_if_t<is_field_v<T> && has_tag_type_v<T>>> : std::true_type {};
template <class T>
constexpr bool is_tagged_field_v = is_tagged_field<T>::value;

template <class, class Enable = void>
struct is_sequence_of : std::false_type { };
template <class T>
struct is_sequence_of<T,
	std::enable_if_t<
		std::is_same<
			typename T::value_type, remove_cref_t<decltype(*std::declval<T>().begin())>
		>::value
	>
> : std::true_type { };
template <class T>
constexpr bool is_sequence_of_v = is_sequence_of<T>::value;


}	//end: namespace med
