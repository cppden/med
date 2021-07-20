/**
@file
IE to represent generic integral value

@copyright Denis Priyomov 2016-2017
Distributed under the MIT License
(See accompanying file LICENSE or visit https://github.com/cppden/med)
*/

#pragma once

#include <type_traits>

#include "units.hpp"
#include "ie_type.hpp"
#include "name.hpp"
#include "exception.hpp"


namespace med {

//traits representing a fixed value of particular size (in bits/bytes/int)
//which is predefined during encode and decode
template <std::size_t VAL, typename T, class... EXT_TRAITS>
struct fixed : value_traits<T, EXT_TRAITS...>
{
	static constexpr bool is_const = true;
	static constexpr typename value_traits<T, EXT_TRAITS...>::value_type value = VAL;
};

//traits representing a fixed value with default
template <std::size_t VAL, class T, class... EXT_TRAITS>
struct defaults : value_traits<T, EXT_TRAITS...>
{
	static constexpr typename value_traits<T, EXT_TRAITS...>::value_type default_value = VAL;
};

//traits representing initialized value which is encoded with the fixed value
//but can have any value when decoded
template <std::size_t VAL, class T, class... EXT_TRAITS>
struct init : fixed<VAL, T, EXT_TRAITS...>
{
	static constexpr bool is_const = false;
};

namespace detail {

/**
 * plain numeric value
 */
template <class TRAITS>
struct numeric_value : IE<IE_VALUE>
{
	using traits     = TRAITS;
	using value_type = typename traits::value_type;
	using base_t     = numeric_value;

	value_type get() const                          { return get_encoded(); }
	auto set(value_type v)                          { return set_encoded(v); }

	//NOTE: do not override!
	static constexpr bool is_const = false;
	value_type get_encoded() const                  { return m_value; }
	void set_encoded(value_type v)                  { m_value = v; m_set = true; }
	void clear()                                    { m_set = false; }
	bool is_set() const                             { return m_set; }
	explicit operator bool() const                  { return is_set(); }
	template <class... ARGS>
	void copy(base_t const& from, ARGS&&...)        { m_value = from.m_value; m_set = from.m_set; }

#if defined(__GNUC__) && (__GNUC__ < 9)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
#endif
	bool operator==(numeric_value const& rhs) const { return is_set() == rhs.is_set() && (!is_set() || get() == rhs.get()); }
#if defined(__GNUC__) && (__GNUC__ < 9)
#pragma GCC diagnostic pop
#endif

	bool operator!=(numeric_value const& rhs) const { return !this->operator==(rhs); }

private:
	bool       m_set{false};
	value_type m_value{};
};

/**
 * plain fixed integral value
 * gives error if decoded value doesn't match the fixed one
 */
template <class TRAITS>
struct const_integer : IE<IE_VALUE>
{
	using traits     = TRAITS;
	using value_type = typename traits::value_type;
	using base_t = const_integer;
	using writable = numeric_value<traits>;

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

	bool operator==(const_integer const&) const         { return true; } //equal to itself by definition
	bool operator!=(const_integer const&) const         { return false; }
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
	bool operator==(init_integer const&) const          { return true; }
	bool operator!=(init_integer const&) const          { return false; }
};

/**
 * integer with a default value
 */
template <class TRAITS>
struct def_integer : numeric_value<TRAITS>
{
	using traits     = TRAITS;
	using value_type = typename traits::value_type;
	using base_t     = def_integer;

	//NOTE: do not override!
	value_type get_encoded() const
	{
		return this->is_set() ? numeric_value<TRAITS>::get_encoded() : traits::default_value;
	}
};


//meta-function to select proper implementation of numeric value
template <class T, typename Enable = void>
struct value_selector;

template <class T>
struct value_selector<T,
	std::enable_if_t<
		std::is_same_v<typename T::value_type, std::remove_const_t<decltype(T::value)>>
		&& T::is_const
	>
>
{
	using type = const_integer<T>;
};

template <class T>
struct value_selector<T,
	std::enable_if_t<
		std::is_same_v<typename T::value_type, std::remove_const_t<decltype(T::value)>>
		&& !T::is_const
	>
>
{
	using type = init_integer<T>;
};

template <class T>
struct value_selector<T,
	std::enable_if_t<
		std::is_same_v<typename T::value_type, std::remove_const_t<decltype(T::default_value)>>
	>
>
{
	using type = def_integer<T>;
};


template <class T, class Enable, class... EXT_TRAITS>
struct value;

template <class T>
struct value<T, std::enable_if_t<is_value_traits<T>::value>>
		: value_selector<T>::type {};

template<class T, class... EXT_TRAITS>
struct value<T, std::enable_if_t<std::is_arithmetic_v<T>>, EXT_TRAITS...>
		: numeric_value<value_traits<T, EXT_TRAITS...>> {};

template<std::size_t N, class... EXT_TRAITS>
struct value<bytes<N>, void, EXT_TRAITS...>
		: numeric_value<value_traits<bytes<N>, EXT_TRAITS...>> {};

template<std::size_t N, class... EXT_TRAITS>
struct value<bits<N>, void, EXT_TRAITS...>
		: numeric_value<value_traits<bits<N>, EXT_TRAITS...>> {};

} //end: namespace detail

/**
 * generic value - a facade for numeric_values above
 */
template <class T, class... EXT_TRAITS>
struct value : detail::value<T, void, EXT_TRAITS...> {};

} //namespace med
