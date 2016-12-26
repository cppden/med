/*!
@file
TODO: define.

@copyright Denis Priyomov 2016
Distributed under the MIT License
(See accompanying file LICENSE or copy at https://opensource.org/licenses/MIT)
*/

#pragma once

#include <cstddef>
#include <cstdint>

namespace med {

//Save current buffer state referred by id.
//It's similar to PUSH_STATE but saved in optional storage provided by user
//and associated with identifier for later retrieval.
struct SNAPSHOT
{
	using id_type = char const*;

	id_type      id;
	std::size_t  size;
};

template <class IE>
constexpr SNAPSHOT::id_type snapshot_id = IE::name();


}	//end: namespace med
