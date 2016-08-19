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
//Check if the buffer state is valid (not EoF)
struct CHECK_STATE {};
//Return current state of buffer.
struct GET_STATE {};
//Reset state of buffer to the given one passed as argument.
struct SET_STATE {};
//Advance the buffer state to relative number of bits
struct ADVANCE_STATE
{
	int    bits;
};

//Set end of buffer (its size).
struct PUSH_SIZE
{
	std::size_t  size;
};

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


//Pad buffer with specfied number of bits using filler value.
struct ADD_PADDING
{
	std::size_t  bits;
	uint8_t      filler;
};

}	//end: namespace med
