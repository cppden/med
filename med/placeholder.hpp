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
	//the following defs are required to avoid special handling in container's encoder
	using field_type = void;

	struct field_t
	{
		static constexpr void clear()       { }
		static constexpr bool is_set()      { return true; }
		template <class... ARGS>
		static constexpr MED_RESULT copy(field_t const&, ARGS&&...) { MED_RETURN_SUCCESS; }
	};
	static constexpr field_t ref_field()    { return field_t{}; }
	static constexpr bool is_set()			{ return true; }
};

}	//end: namespace placeholder

}	//end: namespace med
