/**
@file
IE to represent generic integral value

@copyright Denis Priyomov 2016-2017
Distributed under the MIT License
(See accompanying file LICENSE or visit https://github.com/cppden/med)
*/

#pragma once

#include <type_traits>

#include "ie_type.hpp"
#include "value_traits.hpp"


namespace med {

template <uint8_t NUM>
struct bits
{
	static_assert(NUM > 0 && NUM <= 64, "INVALID NUMBER OF BITS");
	static constexpr uint8_t size = NUM;
};

template <uint8_t NUM>
struct bytes
{
	static_assert(NUM > 0 && NUM <= 8, "INVALID NUMBER OF BYTES");
	static constexpr uint8_t size = NUM;
};

template <std::size_t VAL, class T = std::size_t, class Enable = void>
struct fixed
{
	static_assert(std::is_void<T>(), "MALFORMED FIXED");
};

template <std::size_t VAL, uint8_t BITS>
struct fixed<VAL, bits<BITS>, void>
	: value_traits<BITS>
{
	static constexpr bool is_const = true;
	static constexpr typename value_traits<BITS>::value_type value = VAL;
};

template <std::size_t VAL, uint8_t BYTES>
struct fixed<VAL, bytes<BYTES>, void>
	: value_traits<BYTES*8>
{
	static constexpr bool is_const = true;
	static constexpr typename value_traits<BYTES*8>::value_type value = VAL;
};

template <std::size_t VAL, typename T>
struct fixed< VAL, T, std::enable_if_t<std::is_integral<T>::value> >
	: integral_traits<T>
{
	static constexpr bool is_const = true;
	static constexpr typename integral_traits<T>::value_type value = VAL;
};

template <std::size_t VAL, class T>
struct init : fixed<VAL, T>
{
	static constexpr bool is_const = false;
};


/**
 * plain integral value
 */
template <class TRAITS>
struct integer : IE<IE_VALUE>
{
	using traits     = TRAITS;
	using value_type = typename traits::value_type;
	using base_t     = integer;

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
 * plain fixed integral value
 * gives error if decoded value doesn't match the fixed one
 */
template <class TRAITS>
struct const_integer : IE<const IE_VALUE>
{
	using traits     = TRAITS;
	using value_type = typename traits::value_type;
	using base_t = const_integer;
	using writable = integer<traits>;

	static constexpr void clear()                       { }
	static constexpr value_type get()                   { return get_encoded(); }


	//NOTE: do not override!
	static constexpr value_type get_encoded()           { return traits::value; }
	static constexpr bool set_encoded(value_type v)     { return traits::value == v; }
	static constexpr bool is_set()                      { return true; }
	static constexpr bool match(value_type v)           { return traits::value == v; }
};


/**
 * plain initialized/set field
 * decoded even if doesn't match initial value
 */
template <class TRAITS>
struct init_integer : IE<IE_VALUE>
{
	using traits     = TRAITS;
	using value_type = typename traits::value_type;
	using base_t     = init_integer;

	static constexpr void clear()                       { }
	//NOTE: do not override!
	static constexpr value_type get_encoded()           { return traits::value; }
	static constexpr void set_encoded(value_type)       { }
	static constexpr bool is_set()                      { return true; }
};

template <class T, class Enable = void>
struct value;

template <uint8_t BITS>
struct value<bits<BITS>, void>
	: integer<value_traits<BITS>> {};

template <uint8_t BYTES>
struct value<bytes<BYTES>, void>
	: integer<value_traits<BYTES*8>> {};

template <typename VAL>
struct value< VAL, std::enable_if_t<std::is_integral<VAL>::value> >
	: integer<integral_traits<VAL>> {};

template <class, typename Enable = void>
struct is_fixed_traits : std::integral_constant<int, 0> { };

template <class T>
struct is_fixed_traits<T,
	std::enable_if_t<
		std::is_same<typename T::value_type, std::remove_const_t<decltype(T::value)>>::value
		&& T::is_const
	>
> : std::integral_constant<int, 1> { };

template <class T>
struct is_fixed_traits<T,
	std::enable_if_t<
		std::is_same<typename T::value_type, std::remove_const_t<decltype(T::value)>>::value
		&& !T::is_const
	>
> : std::integral_constant<int, -1> { };

template <class T>
struct value< T, std::enable_if_t<!std::is_void<typename T::value_type>::value && (0 == is_fixed_traits<T>::value)> >
	: integer<T> {};

template <class T>
struct value< T, std::enable_if_t<(1 == is_fixed_traits<T>::value)> >
	: const_integer<T> {};

template <class T>
struct value< T, std::enable_if_t<(-1 == is_fixed_traits<T>::value)> >
	: init_integer<T> {};


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
