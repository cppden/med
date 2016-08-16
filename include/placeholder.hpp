/*!
@file
TODO: define.

@copyright Denis Priyomov 2016
Distributed under the MIT License
(See accompanying file LICENSE or copy at https://opensource.org/licenses/MIT)
*/

#pragma once

namespace med {

namespace placeholder {

template <int DELTA = 0>
struct _length
{
	//the following defs are required to avoid special handling in container's encoder
	using field_type = void;

	struct field_t
	{
		static bool constexpr is_set()      { return true; }
	};
	static constexpr field_t ref_field()    { return field_t{}; }
};

struct _tag
{
	//the following defs are required to avoid special handling in container's encoder
	using field_type = void;

	struct field_t
	{
		static bool constexpr is_set()      { return true; }
	};
	static constexpr field_t ref_field()    { return field_t{}; }
};

}	//end: namespace placeholder

}	//end: namespace med
