/**
@file
placeholders for IE replacement to select proper scope

@copyright Denis Priyomov 2016-2017
Distributed under the MIT License
(See accompanying file LICENSE or visit https://github.com/cppden/med)
*/

#pragma once

#include "exception.hpp"

namespace med {

namespace placeholder {

template <int DELTA = 0>
struct _length
{
	using field_type = void;

	//the following defs are required to avoid special handling in container's encoder
	static constexpr void clear()       { }
	static constexpr bool is_set()      { return true; }
	template <class... Ts>
	static constexpr void copy(Ts&&...) { }
};

template<int L, int R>
static constexpr bool operator==(_length<L> const&, _length<R> const&) { return true; }

}	//end: namespace placeholder

}	//end: namespace med
