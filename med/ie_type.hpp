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
template <class... EXT_TRAITS>
struct empty : IE<IE_NULL>
{
	using traits     = value_traits<bits<0>, EXT_TRAITS...>;
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

//marker for the optional IEs
struct optional_t {};
template <class T> constexpr bool is_optional_v = std::is_base_of<optional_t, T>::value;

//read during decode w/o changing buffer state
//skipped during encode and length calculation
struct peek_t {};
template <class T> constexpr bool is_peek_v = std::is_base_of<peek_t, T>::value;

//check if type used in meta-information is also used inside container
//this means we shouldn't encode this meta-data implicitly
template <class META_INFO, class CONT>
constexpr bool explicit_meta_in()
{
	if constexpr (!meta::list_is_empty_v<META_INFO> && std::is_base_of_v<CONTAINER, typename CONT::ie_type>)
	{
		using ft = typename meta::list_first_t<META_INFO>::info_type;
		return CONT::template has<ft>();
	}
	return false;
}

}	//end: namespace med
