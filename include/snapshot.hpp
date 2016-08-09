/*!
@file
TODO: define.

@copyright Denis Priyomov 2016
Distributed under the MIT License
(See accompanying file LICENSE or copy at https://opensource.org/licenses/MIT)
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
std::enable_if_t<std::is_base_of<with_snapshot, IE>::value>
inline snapshot(FUNC& func, IE& ie)
{
	func(SNAPSHOT{snapshot_id<IE>, get_length(ie)});
}

template <class FUNC, class IE>
std::enable_if_t<!std::is_base_of<with_snapshot, IE>::value>
constexpr snapshot(FUNC&, IE&)
{
}

}
