/**
@file
helper traits to extract IE name

@copyright Denis Priyomov 2016-2017
Distributed under the MIT License
(See accompanying file LICENSE or visit https://github.com/cppden/med)
*/

#pragma once

#include <cstdlib>
#include <cstring>
#include <cxxabi.h>

#include "functional.hpp"

namespace med {

template <class T>
const char* class_name()
{
	char const* psz = typeid(T).name();

	int status;
	if (char* sane = abi::__cxa_demangle(psz, 0, 0, &status))
	{
		constexpr std::size_t LEN = 4*1024;
		constexpr std::size_t NUM = 8;
		static char szbuf[NUM][LEN];
		static uint8_t index = 0;
		index = (index + 1) % NUM;
		char* sz = szbuf[index];
		std::strncpy(sz, sane, LEN-1);
		sz[LEN-1] = '\0';
		std::free(sane);
		psz = sz;
	}

	return psz;
}

template <class, class Enable = void>
struct has_name : std::false_type { };

template <class T>
struct has_name<T, std::void_t<decltype(T::name())>> : std::true_type { };

template <class T>
constexpr bool has_name_v = has_name<T>::value;

template <class IE, class...>
constexpr char const* name()
{
	if constexpr (has_name_v<IE>)
	{
		return IE::name();
	}
	else if constexpr (has_field_type_v<IE>)
	{
		//gradually peel-off indirections looking for ::name() in each step
		return name<typename IE::field_type>();
	}
	else
	{
		return class_name<IE>();
	}
}

} //end: namespace med
