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

struct IE_TV : AGGREGATE {};
struct IE_LV : AGGREGATE {};

template <class IE_TYPE>
struct IE
{
	using ie_type = IE_TYPE;
};

template <class IE>
constexpr bool is_const_ie_v = std::is_const<typename IE::ie_type>::value;

//always-set empty IE as a message w/o body
struct empty : IE<IE_NULL>
{
	static constexpr void clear()           { }
	static constexpr void set()             { }
	static constexpr bool get()             { return true; }
	static constexpr bool is_set()          { return true; }
	template <class... ARGS>
	static constexpr MED_RESULT copy(empty const&, ARGS&&...) { MED_RETURN_SUCCESS; }
};

//selectable IE as empty case in choice
struct selectable : IE<IE_NULL>
{
	void set()                              { m_set = true; }
	bool get() const                        { return is_set(); }
	void clear()                            { m_set = false; }
	bool is_set() const                     { return m_set; }
	explicit operator bool() const          { return is_set(); }
	template <class... ARGS>
	MED_RESULT copy(selectable const& from, ARGS&&...) { m_set = from.m_set; MED_RETURN_SUCCESS; }

private:
	bool    m_set{false};
};

template <class T>
constexpr bool is_empty_v = std::is_base_of<IE<IE_NULL>, T>::value;

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

}	//end: namespace med
