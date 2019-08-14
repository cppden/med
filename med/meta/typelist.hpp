#pragma once
/*
@file
tiny metaprogramming type list

@copyright Denis Priyomov 2019
Distributed under the MIT License
(See accompanying file LICENSE or visit https://github.com/cppden/ctstring)
*/

namespace med::meta {

template <class... T>
struct typelist {};

template <typename T>
struct wrap
{
	using type = T;
};

template <class T>
using unwrap_t = typename T::type;

} //end: namespace med::meta

