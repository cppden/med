/**
@file
snapshot of buffer state for inplace updating

@copyright Denis Priyomov 2016-2017
Distributed under the MIT License
(See accompanying file LICENSE or visit https://github.com/cppden/med)
*/

#pragma once

#include "length.hpp"
#include "state.hpp"

namespace med {

//captures placement of IE in buffer during encoding to be updated in-place later in encoded data
//used as attribute for IE via multiple inheritance
//NOTE: requires IE to have name() defined!
struct with_snapshot {};

template <class FUNC, class IE>
constexpr void snapshot(FUNC& func, IE& ie)
{
	if constexpr (std::is_base_of<with_snapshot, IE>::value)
	{
		func(SNAPSHOT{snapshot_id<IE>, sl::ie_length<>(ie, func)});
	}
}

}
