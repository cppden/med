/**
@file
error and warning codes definition

@copyright Denis Priyomov 2016-2017
Distributed under the MIT License
(See accompanying file LICENSE or visit https://github.com/cppden/med)
*/

#pragma once

#include <cstdint>
#undef OVERFLOW

namespace med {

enum class error : uint8_t
{
	SUCCESS,
	OVERFLOW,   //insufficient input or output buffer
	INCORRECT_VALUE,
	INCORRECT_TAG,
	MISSING_IE, //missing required (mandatory) IE
	EXTRA_IE,   //excessive number of IE
	OUT_OF_MEMORY,   //no space to allocate
};

}	//end: namespace med
