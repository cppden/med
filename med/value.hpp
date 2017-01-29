/**
@file
IE to represent generic integral value

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

template <std::size_t BITS, class TRAITS = value_traits<BITS>>
struct value : IE<IE_VALUE>
{
	using traits     = TRAITS;
	using value_type = typename traits::value_type;
	using base_t = value;

	value_type get() const                      { return get_encoded(); }
	void set(value_type v)                      { return set_encoded(v); }

	//NOTE: do not override!
	value_type get_encoded() const              { return m_value; }
	void set_encoded(value_type v)              { m_value = v; m_set = true; }
	void clear()                                { m_set = false; }
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
template <std::size_t VALUE, std::size_t BITS = sizeof(std::size_t)*8, class TRAITS = value_traits<BITS>>
struct cvalue : IE<const IE_VALUE>
{
	using traits     = TRAITS;
	using value_type = typename traits::value_type;
	using base_t = cvalue;
	using writable = value<BITS>;

	static constexpr void clear()                       { }
	static constexpr value_type get()                   { return get_encoded(); }


	//NOTE: do not override!
	static constexpr value_type get_encoded()           { return VALUE; }
	static constexpr bool set_encoded(value_type v)     { return VALUE == v; }
	static constexpr bool is_set()                      { return true; }
	static constexpr bool match(value_type v)           { return VALUE == v; }
};

/**
 * plain initialized field with length counted in bits
 * decoded even if doesn't match initial value
 */
template <std::size_t VALUE, std::size_t BITS = sizeof(std::size_t)*8, class TRAITS = value_traits<BITS>>
struct ivalue : IE<IE_VALUE>
{
	using traits     = TRAITS;
	using value_type = typename traits::value_type;
	using base_t = ivalue;

	static constexpr void clear()                       { }
	//NOTE: do not override!
	static constexpr value_type get_encoded()           { return VALUE; }
	static constexpr void set_encoded(value_type v)     { }
	static constexpr bool is_set()                      { return true; }
};

template <class IE, typename VALUE>
constexpr auto set_value(IE& ie, VALUE v) -> std::enable_if_t<std::is_same<bool, decltype(ie.set_encoded(v))>::value, bool>
{
	return ie.set_encoded(v);
}

template <class IE, typename VALUE>
constexpr auto set_value(IE& ie, VALUE v) -> std::enable_if_t<std::is_same<void, decltype(ie.set_encoded(v))>::value, bool>
{
	ie.set_encoded(v);
	return true;
}

} //namespace med
