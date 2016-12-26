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

//Memorize the current state of buffer if not at the end (one entry in buffer itself).
struct PUSH_STATE {};
//Restore the last saved state of buffer if any.
struct POP_STATE {};
//Reset state of buffer to the given one passed as argument.
struct SET_STATE {};

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
