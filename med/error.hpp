/*!
@file
TODO: define.

@copyright Denis Priyomov 2016
Distributed under the MIT License
(See accompanying file LICENSE or copy at https://opensource.org/licenses/MIT)
*/

#pragma once

namespace med {

enum class error : uint8_t
{
	SUCCESS = 0,
	OVERFLOW, //insufficient input or output buffer
	INCORRECT_VALUE,
	INCORRECT_TAG,
	MISSING_IE, //missing required (mandatory) IE
	EXTRA_IE,   //excessive number of IE
};

enum class warning : uint8_t
{
	NONE = 0,
	OVERFLOW, //insufficient snapshot storage
};

}	//end: namespace med
