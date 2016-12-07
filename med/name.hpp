/*!
@file
TODO: define.

@copyright Denis Priyomov 2016
Distributed under the MIT License
(See accompanying file LICENSE or copy at https://opensource.org/licenses/MIT)
*/

#pragma once

#include <cstring>
#include <cxxabi.h>

#include "ie.hpp"

namespace med {

namespace detail {

template <std::size_t SIZE>
struct char_buffer
{
	static char* allocate()
	{
		static char sz[SIZE];
		return sz;
	}
};

}	//end: namespace detail

template <class T>
const char* class_name()
{
	char const* psz = typeid(T).name();

	int status;
	if (char* sane = abi::__cxa_demangle(psz, 0, 0, &status))
	{
		enum {LEN = 256};
		char* sz = detail::char_buffer<LEN>::allocate();
		std::strncpy(sz, sane, LEN-1);
		free(sane);
		psz = sz;
	}

	return psz;
}

template <class, class Enable = void>
struct has_name : std::false_type { };

template <class T>
struct has_name<T, void_t<decltype(T::name())>> : std::true_type { };

template <class T>
constexpr bool has_name_v = has_name<T>::value;


template <class IE>
constexpr std::enable_if_t<has_name_v<IE>, char const*>
name()
{
	return IE::name();
}

template <class IE>
constexpr std::enable_if_t<!has_name_v<IE>, char const*>
name()
{
	return class_name<IE>();
}

}
