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
	static char const* psz = nullptr;
	static char sz[128];
	if (!psz)
	{
		psz = typeid(T).name();
		if (char* sane = abi::__cxa_demangle(psz, nullptr, nullptr, nullptr))
		{
			auto* plain = std::strrchr(sane, ':');
			if (plain) plain++;
			else plain = sane;
			std::strncpy(sz, plain, sizeof(sz)-1);
			sz[sizeof(sz) - 1] = '\0';
			std::free(sane);
			psz = sz;
		}
	}

	return psz;
}

template <class T>
concept AHasName = requires(T v)
{
	{ T::name() } -> std::same_as<char const*>;
};

template <class IE, class...>
constexpr char const* name()
{
	if constexpr (AHasName<IE>)
	{
		return IE::name();
	}
	else if constexpr (AHasFieldType<IE>)
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
