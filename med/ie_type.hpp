/**
@file
elementary IE types

@copyright Denis Priyomov 2016-2017
Distributed under the MIT License
(See accompanying file LICENSE or visit https://github.com/cppden/med)
*/

#pragma once

#include "value_traits.hpp"
#include "concepts.hpp"
#include "exception.hpp"

namespace med {


//meta-selectors for IEs
struct PRIMITIVE {}; //represents value/data which is handled by physical layer of codec

//selectors for IEs
struct IE_NULL         : PRIMITIVE {};
struct IE_VALUE        : PRIMITIVE {};
struct IE_OCTET_STRING : PRIMITIVE {};
struct IE_BIT_STRING   : PRIMITIVE {};

struct CONTAINER {}; //represents a container of IEs (holds internally)
//selectors for IEs
struct IE_CHOICE : CONTAINER {};
struct IE_SEQUENCE : CONTAINER {};
struct IE_SET : CONTAINER {};

template <class IE>
concept AContainer = std::is_base_of_v<CONTAINER, typename IE::ie_type>;

//structure layer selectors
struct IE_TAG {}; //tag
struct IE_LEN {}; //length

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

//check if type used in meta-information is also used inside container
//this means we shouldn't encode this meta-data implicitly
template <class META_INFO, class CONT>
constexpr bool explicit_meta_in()
{
	if constexpr (!meta::list_is_empty_v<META_INFO> && AContainer<CONT>)
	{
		using ft = get_info_t<meta::list_first_t<META_INFO>>;
		return CONT::template has<ft>();
	}
	return false;
}

template <
	class IE_TYPE,
	class META_INFO = meta::typelist<>,
	class EXP_TAG = void,
	class EXP_LEN = void,
	class DEPENDENT = void,
	class DEPENDENCY = void
>
struct type_context
{
	using ie_type = IE_TYPE;
	using meta_info_type = META_INFO;
	using explicit_tag_type = EXP_TAG;
	using explicit_length_type = EXP_LEN;
	using dependent_type = DEPENDENT;
	using dependency_type = DEPENDENCY;
};


}	//end: namespace med
