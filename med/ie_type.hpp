/**
@file
elementary IE types

@copyright Denis Priyomov 2016
Distributed under the MIT License
(See accompanying file LICENSE or copy at https://opensource.org/licenses/MIT)
*/

#pragma once

#include "functional.hpp"

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

//always-set empty IE as a message w/o body
struct empty : IE<IE_NULL>
{
	constexpr void clear()                  { }
	constexpr void set()                    { }
	constexpr bool get()                    { return true; }
	constexpr bool is_set()                 { return true; }
};

//selectable IE as empty case in choice
struct selectable : IE<IE_NULL>
{
	void set()                              { m_set = true; }
	bool get() const                        { return is_set(); }
	void clear()                            { m_set = false; }
	bool is_set() const                     { return m_set; }
	explicit operator bool() const          { return is_set(); }

private:
	bool    m_set{false};
};

template <class T>
constexpr bool is_empty_v = std::is_base_of<IE<IE_NULL>, T>::value;

//read during decode w/o changing buffer state
//skipped during encode and length calculation
struct read_only_t {};

template <class T>
struct read_only : T, read_only_t {};

template <class T>
constexpr bool is_read_only_v = std::is_base_of<read_only_t, T>::value;

//skip IE during both decode and encode
//can be used to define more efficient complex header for set
struct skip_t {};

template <class T>
struct skip : T, skip_t {};

template <class T>
constexpr bool is_skip_v = std::is_base_of<skip_t, T>::value;

}	//end: namespace med
