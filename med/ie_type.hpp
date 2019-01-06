/**
@file
elementary IE types

@copyright Denis Priyomov 2016-2017
Distributed under the MIT License
(See accompanying file LICENSE or visit https://github.com/cppden/med)
*/

#pragma once

#include "functional.hpp"
#include "exception.hpp"

namespace med {

//meta-selectors for IEs
struct PRIMITIVE {}; //represents value/data which is handled by physical layer of codec
struct CONTAINER {}; //represents a container of IEs (holds internally)
struct AGGREGATE {}; //represents a aggregation of IEs (holds externally)

//selectors for IEs
struct IE_NULL {};
struct IE_VALUE        : PRIMITIVE {};
struct IE_OCTET_STRING : PRIMITIVE {};
struct IE_BIT_STRING   : PRIMITIVE {};

struct IE_TV : AGGREGATE {};
struct IE_LV : AGGREGATE {};

template <class IE_TYPE>
struct IE
{
	using ie_type = IE_TYPE;
};

//always-set empty IE as a message w/o body
struct empty : IE<IE_NULL>
{
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

//skip IE during both decode and encode
//can be used to define more efficient complex header for set
struct skip_t {};

template <class T>
struct skip : T, skip_t {};

template <class T>
constexpr bool is_skip_v = std::is_base_of<skip_t, T>::value;

//TODO: implicitly guess? length_enc/dec forwarding func operator prevents signature detection...
//template<class, class Enable = void>
//struct container_aware : std::false_type {};
//template<class FUNC>
//struct container_aware<FUNC, std::void_t<decltype(std::declval<FUNC>()(true, CONTAINER{}))>> : std::true_type {};
enum class codec_e
{
	PLAIN, //don't care of any data structure
	STRUCTURED, //noify on containers structure (e.g. JSON-codec)
};

//for structured codecs
struct ENTRY_CONTAINER{};
struct HEADER_CONTAINER{};
struct EXIT_CONTAINER{};
struct NEXT_CONTAINER_ELEMENT{}; //NOTE: not issued for the 1st element

template <class T, class Enable = void>
struct get_codec_kind
{
	static constexpr auto value = codec_e::PLAIN;
};
template <class T>
struct get_codec_kind<T, std::void_t<decltype(std::remove_reference_t<T>::codec_kind)>>
{
	static constexpr auto value = T::codec_kind;
};
template <class T>
constexpr codec_e get_codec_kind_v = get_codec_kind<T>::value;

}	//end: namespace med
