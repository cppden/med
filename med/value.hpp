/*!
@file
TODO: define.

@copyright Denis Priyomov 2016
Distributed under the MIT License
(See accompanying file LICENSE or copy at https://opensource.org/licenses/MIT)
*/

#pragma once

#include <type_traits>

#include "ie.hpp"
#include "value_traits.hpp"


namespace med {

/**
 * plain field with length counted in bits
 */

template <std::size_t BITS>
struct value : IE<IE_VALUE>
{
	using traits     = value_traits<BITS>;
	using value_type = typename traits::value_type;
	using base_t = value;

	value_type get() const                      { return m_value; }
	void set(value_type v)                      { m_value = v; m_set = true; }
	void reset()                                { m_set = false; }
	bool is_set() const                         { return m_set; }
	explicit operator bool() const              { return is_set(); }

private:
	value_type m_value{};
	bool       m_set{false};
};

/**
 * plain fixed field with length counted in bits
 * gives error if decoded value doesn't match the fixed one
 */
template <std::size_t VALUE, std::size_t BITS = sizeof(std::size_t)*8>
struct cvalue : IE<const IE_VALUE>
{
	using traits     = value_traits<BITS>;
	using value_type = typename traits::value_type;
	using base_t = cvalue;

	static constexpr value_type get()          { return VALUE; }
	static constexpr bool set(value_type v)    { return VALUE == v; }
	static constexpr bool is_set()             { return true; }
	static constexpr bool match(value_type v)  { return VALUE == v; }
};

/**
 * plain initialized field with length counted in bits
 * decoded even if doesn't match initial value
 */
template <std::size_t VALUE, std::size_t BITS = sizeof(std::size_t)*8>
struct ivalue : IE<IE_VALUE>
{
	using traits     = value_traits<BITS>;
	using value_type = typename traits::value_type;
	using base_t = ivalue;

	static constexpr value_type get()          { return VALUE; }
	static constexpr void set(value_type v)    { }
	static constexpr bool is_set()             { return true; }
};

//convert cvalue to a value
template <class T, typename Enable = void>
struct make_value
{
	using type = value<T::traits::bits>;
};


template <class T>
using make_value_t = typename make_value<T>::type;

//convert read-write value to a fixed one
template <class T, std::size_t VALUE>
using make_cvalue = cvalue<VALUE, T::traits::bits>;

template <class T, std::size_t VALUE>
using make_ivalue = ivalue<VALUE, T::traits::bits>;

template <class IE, typename VALUE>
inline auto set_value(IE& ie, VALUE v) -> std::enable_if_t<std::is_same<bool, decltype(ie.set(v))>::value, bool>
{
	return ie.set(v);
}

template <class IE, typename VALUE>
inline auto set_value(IE& ie, VALUE v) -> std::enable_if_t<std::is_same<void, decltype(ie.set(v))>::value, bool>
{
	ie.set(v);
	return true;
}

} //namespace med