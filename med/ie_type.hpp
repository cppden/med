/**
@file
elementary IE types

@copyright Denis Priyomov 2016-2017
Distributed under the MIT License
(See accompanying file LICENSE or visit https://github.com/cppden/med)
*/

#pragma once

#include "value_traits.hpp"
#include "functional.hpp"
#include "exception.hpp"

namespace med {


//meta-selectors for IEs
struct PRIMITIVE {}; //represents value/data which is handled by physical layer of codec
struct CONTAINER {}; //represents a container of IEs (holds internally)

//selectors for IEs
struct IE_NULL         : PRIMITIVE {};
struct IE_VALUE        : PRIMITIVE {};
struct IE_OCTET_STRING : PRIMITIVE {};
struct IE_BIT_STRING   : PRIMITIVE {};

//structure layer selectors
struct IE_TV {}; //tag-value
struct IE_LV {}; //length-value
struct IE_TAG {}; //tag
struct IE_LEN {}; //length

//for structured codecs
struct ENTRY_CONTAINER{};
struct HEADER_CONTAINER{};
struct EXIT_CONTAINER{};
struct NEXT_CONTAINER_ELEMENT{}; //NOTE: not issued for the 1st element

template <class IE_TYPE>
struct IE
{
	using ie_type = IE_TYPE;
};

//always-set empty IE as a message w/o body
template <class EXT_TRAITS = base_traits>
struct empty : IE<IE_NULL>
{
	using traits     = value_base_traits<void, bits<0>, EXT_TRAITS>;
	using value_type = void;

	static constexpr void clear()           { }
	static constexpr void set()             { }
	static constexpr bool get()             { return true; }
	static constexpr bool is_set()          { return true; }
	template <class... ARGS>
	static constexpr void copy(empty const&, ARGS&&...) { }
};

template <class T>
constexpr bool is_empty_v = std::is_same_v<IE_NULL, typename T::ie_type>;

//read during decode w/o changing buffer state
//skipped during encode and length calculation
struct peek_t {};

template <class T>
struct peek : T, peek_t {};

template <class T>
constexpr bool is_peek_v = std::is_base_of<peek_t, T>::value;

}	//end: namespace med
