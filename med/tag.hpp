/**
@file
tag related definitions

@copyright Denis Priyomov 2016-2017
Distributed under the MIT License
(See accompanying file LICENSE or visit https://github.com/cppden/med)
*/

#pragma once

#include <type_traits>

#include "ie_type.hpp"
#include "meta/typelist.hpp"
#include "concepts.hpp"

namespace med {

template <class FUNC>
struct tag_getter
{
	//TODO: skip if not tag?
	template <class T>
	using apply = meta::list_first_t<meta::produce_info_t<FUNC, T>>;
};

template <class T>
constexpr auto get_tag(T const& header)
{
	if constexpr (AHasGetTag<T>)
	{
		return header.get_tag();
	}
	else
	{
		return header.get_encoded();
	}
}

}	//end: namespace med
