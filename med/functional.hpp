/**
@file
set of auxilary traits/functions

@copyright Denis Priyomov 2016-2017
Distributed under the MIT License
(See accompanying file LICENSE or visit https://github.com/cppden/med)
*/

#pragma once

#include <utility>
#include <type_traits>

#include "concepts.hpp"

namespace med {

template <class T>
concept AHasFieldType = requires(T v)
{
	typename T::field_type;
};

template <class T>
struct get_field_type
{
	using type = T;
};
template <AHasFieldType T>
struct get_field_type<T>
{
	using type = typename get_field_type<typename T::field_type>::type;
};
template <class T> using get_field_type_t = typename get_field_type<T>::type;

}	//end: namespace med
