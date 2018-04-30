/**
@file
single-instance field and its aggregates

@copyright Denis Priyomov 2016-2017
Distributed under the MIT License
(See accompanying file LICENSE or visit https://github.com/cppden/med)
*/

#pragma once

#include "ie_type.hpp"
#include "tag.hpp"
#include "length.hpp"


namespace med {

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
	std::is_same_v<bool, decltype(std::declval<T>().is_set())>
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

}	//end: namespace med
