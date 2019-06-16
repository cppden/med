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
#include "exception.hpp"


namespace med {

//traits representing a fixed value of particular size (in bits/bytes/int)
//which is predefined during encode and decode
template <std::size_t VAL, class T, class EXT_TRAITS = base_traits, class Enable = void>
struct fixed
{
	static_assert(std::is_void<T>(), "MALFORMED FIXED");
};

template <std::size_t VAL, std::size_t BITS, class EXT_TRAITS>
struct fixed<VAL, bits<BITS>, EXT_TRAITS, void>
	: value_traits<EXT_TRAITS, BITS>
{
	static constexpr bool is_const = true;
	static constexpr typename value_traits<EXT_TRAITS, BITS>::value_type value = VAL;
};

template <std::size_t VAL, std::size_t BYTES, class EXT_TRAITS>
struct fixed<VAL, bytes<BYTES>, EXT_TRAITS, void>
	: value_traits<EXT_TRAITS, BYTES*8>
{
	static constexpr bool is_const = true;
	static constexpr typename value_traits<EXT_TRAITS, BYTES*8>::value_type value = VAL;
};

template <std::size_t VAL, typename T, class EXT_TRAITS>
struct fixed< VAL, T, EXT_TRAITS, std::enable_if_t<std::is_integral_v<T>> >
	: value_base_traits<T, EXT_TRAITS>
{
	static constexpr bool is_const = true;
	static constexpr typename value_base_traits<T, EXT_TRAITS>::value_type value = VAL;
};

//traits representing a fixed value with default
template <std::size_t VAL, class T, class EXT_TRAITS = base_traits, class Enable = void>
struct defaults
{
	static_assert(std::is_void<T>(), "MALFORMED DEFAULTS");
};

template <std::size_t VAL, std::size_t BITS, class EXT_TRAITS>
struct defaults<VAL, bits<BITS>, EXT_TRAITS, void>
	: value_traits<EXT_TRAITS, BITS>
{
	static constexpr typename value_traits<EXT_TRAITS, BITS>::value_type default_value = VAL;
};

template <std::size_t VAL, std::size_t BYTES, class EXT_TRAITS>
struct defaults<VAL, bytes<BYTES>, EXT_TRAITS, void>
	: value_traits<EXT_TRAITS, BYTES*8>
{
	static constexpr typename value_traits<EXT_TRAITS, BYTES*8>::value_type default_value = VAL;
};

template <std::size_t VAL, typename T, class EXT_TRAITS>
struct defaults< VAL, T, EXT_TRAITS, std::enable_if_t<std::is_integral_v<T>> >
	: value_base_traits<T, EXT_TRAITS>
{
	static constexpr typename value_base_traits<T, EXT_TRAITS>::value_type default_value = VAL;
};

//traits representing initialized value which is encoded with the fixed value
//but can have any value when decoded
template <std::size_t VAL, class T, class EXT_TRAITS = base_traits>
struct init : fixed<VAL, T, EXT_TRAITS>
{
	static constexpr bool is_const = false;
};


/**
 * plain numeric value (TODO: rename to numeric_value?)
 */
template <class TRAITS>
struct integer : IE<IE_VALUE>
{
	using traits     = TRAITS;
	using value_type = typename traits::value_type;
	using base_t     = integer;

	value_type get() const                      { return get_encoded(); }
	auto set(value_type v)                      { return set_encoded(v); }

	//NOTE: do not override!
	static constexpr bool is_const = false;
	value_type get_encoded() const              { return m_value; }
	void set_encoded(value_type v)              { m_value = v; m_set = true; }
	void clear()                                { m_set = false; }
	bool is_set() const                         { return m_set; }
	explicit operator bool() const              { return is_set(); }
	template <class... ARGS>
	void copy(base_t const& from, ARGS&&...)
	{
		m_value = from.m_value;
		m_set = from.m_set;
	}

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
	static constexpr bool is_const = true;
	explicit operator bool() const                      { return is_set(); }
	static constexpr value_type get_encoded()           { return traits::value; }
	static constexpr bool set_encoded(value_type v)     { return traits::value == v; }
	static constexpr bool is_set()                      { return true; }
	static constexpr bool match(value_type v)           { return traits::value == v; }
	template <class... ARGS>
	static constexpr void copy(base_t const&, ARGS&&...){ }
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
	explicit operator bool() const                      { return is_set(); }
	//NOTE: do not override!
	static constexpr bool is_const = true;
	static constexpr value_type get_encoded()           { return traits::value; }
	static constexpr void set_encoded(value_type)       { }
	static constexpr bool is_set()                      { return true; }
	template <class... ARGS>
	static constexpr void copy(base_t const&, ARGS&&...){ }
};


/**
 * integer with a default value
 */
template <class TRAITS>
struct def_integer : integer<TRAITS>
{
	using traits     = TRAITS;
	using value_type = typename traits::value_type;
	using base_t     = def_integer;

	//NOTE: do not override!
	value_type get_encoded() const
	{
		return this->is_set() ? integer<TRAITS>::get_encoded() : traits::default_value;
	}
};


/**
 * generic value - a facade for integers above
 */
template <class T, class EXT_TRAITS = base_traits, class Enable = void>
struct value;


template <std::size_t BITS, class EXT_TRAITS>
struct value<bits<BITS>, EXT_TRAITS, void>
	: integer<value_traits<EXT_TRAITS, BITS>> {};

template <std::size_t BYTES, class EXT_TRAITS>
struct value<bytes<BYTES>, EXT_TRAITS, void>
	: integer<value_traits<EXT_TRAITS, BYTES*8>> {};

template <typename VAL, class EXT_TRAITS>
struct value< VAL, EXT_TRAITS, std::enable_if_t<std::is_integral_v<VAL>> >
	: integer<value_base_traits<VAL, EXT_TRAITS>> {};

//meta-function to select proper int
template <class T, typename Enable = void>
struct integer_selector
{
	static_assert(std::is_void<T>(), "INVALID TRAITS");
};

template <class T>
struct integer_selector<T,
	std::enable_if_t<
		std::is_same_v<typename T::value_type, std::remove_const_t<decltype(T::value)>>
		&& T::is_const
	>
>
{
	using type = const_integer<T>;
};

template <class T>
struct integer_selector<T,
	std::enable_if_t<
		std::is_same_v<typename T::value_type, std::remove_const_t<decltype(T::value)>>
		&& !T::is_const
	>
>
{
	using type = init_integer<T>;
};

template <class T>
struct integer_selector<T,
	std::enable_if_t<
		std::is_same_v<typename T::value_type, std::remove_const_t<decltype(T::default_value)>>
	>
>
{
	using type = def_integer<T>;
};

template <class T, class EXT_TRAITS>
struct value< T, EXT_TRAITS, std::enable_if_t<not std::is_void_v<typename T::value_type>> >
	: integer_selector<T>::type {};

//TODO: support in octet codecs? rename integer to more generic name?
template <typename VAL, class EXT_TRAITS>
struct value< VAL, EXT_TRAITS, std::enable_if_t<std::is_floating_point_v<VAL>> >
	: integer<value_base_traits<VAL, EXT_TRAITS>> {};


} //namespace med
