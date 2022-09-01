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
#include "concepts.hpp"


namespace med {

//traits representing a fixed value of particular size (in bits/bytes/int)
//which is predefined during encode and decode
template <std::size_t VAL, typename T, class... EXT_TRAITS>
struct fixed : value_traits<T, EXT_TRAITS...>
{
	static constexpr typename value_traits<T, EXT_TRAITS...>::value_type value = VAL;
};

//traits representing initialized value with a default
template <std::size_t VAL, class T, class... EXT_TRAITS>
struct init : value_traits<T, EXT_TRAITS...>
{
	static constexpr typename value_traits<T, EXT_TRAITS...>::value_type value = VAL;
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

#if !defined(__clang__) && defined(__GNUC__) && (__GNUC__ < 9)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
#endif
	bool operator==(numeric_value const& rhs) const noexcept
	{
		auto const res = (is_set() == rhs.is_set())
					&& (!is_set() || get_encoded() == rhs.get_encoded());
#ifdef CODEC_TRACE_ENABLE
		if (!res)
		{
			if (is_set() != rhs.is_set()) CODEC_TRACE("??? is_set differs: %d != %d", is_set(), rhs.is_set());
			else CODEC_TRACE("??? value differs: %#zX != %#zX", size_t(get_encoded()), size_t(rhs.get_encoded()));
		}
#endif //CODEC_TRACE_ENABLE
		return res;
	}
#if !defined(__clang__) && defined(__GNUC__) && (__GNUC__ < 9)
#pragma GCC diagnostic pop
#endif

private:
	bool       m_set{false};
	value_type m_value{};
};

/**
 * plain fixed integral value
 * gives error if decoded value doesn't match the fixed one
 */
template <class TRAITS>
struct const_value : IE<IE_VALUE>
{
	using traits     = TRAITS;
	using value_type = typename traits::value_type;
	using base_t     = const_value;

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

	bool operator==(base_t const&) const noexcept       { return true; } //equal to itself by definition
};

/**
 * plain initialized/set field
 * decoded even if doesn't match initial value
 */
template <class TRAITS>
struct init_value : IE<IE_VALUE>
{
	using traits     = TRAITS;
	using value_type = typename traits::value_type;
	using base_t     = init_value;

	constexpr void clear()                              { set_encoded(traits::value); }
	constexpr value_type get() const                    { return get_encoded(); }
	constexpr auto set(value_type v)                    { return set_encoded(v); }
	//NOTE: do not override!
	static constexpr bool is_const = true;
	constexpr value_type get_encoded() const noexcept   { return m_value; }
	constexpr void set_encoded(value_type v)            { m_value = v; }
	bool is_default() const noexcept                    { return get_encoded() == traits::value; }
	static constexpr bool is_set() noexcept             { return true; }
	explicit operator bool() const noexcept             { return is_set(); }
	template <class... ARGS>
	void copy(base_t const& from, ARGS&&...)            { m_value = from.m_value; }

#if !defined(__clang__) && defined(__GNUC__) && (__GNUC__ < 9)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
#endif
	bool operator==(base_t const& rhs) const noexcept
	{
		auto const res = get_encoded() == rhs.get_encoded();
#ifdef CODEC_TRACE_ENABLE
		if (!res)
		{
			CODEC_TRACE("??? value differs: %#zX != %#zX", size_t(get_encoded()), size_t(rhs.get_encoded()));
		}
#endif //CODEC_TRACE_ENABLE
		return res;
	}
#if !defined(__clang__) && defined(__GNUC__) && (__GNUC__ < 9)
#pragma GCC diagnostic pop
#endif

private:
	value_type m_value {traits::value};
};

//meta-function to select proper implementation of numeric value
template <class T> struct value_selector;

template <class T>
concept AValueDefined = std::is_same_v<typename T::value_type, std::remove_const_t<decltype(T::value)>>;

template <std::size_t VAL, class... ETS>
struct value_selector<fixed<VAL, ETS...>>
{
	using type = const_value<init<VAL, ETS...>>;
};

template <std::size_t VAL, class... ETS>
struct value_selector<init<VAL, ETS...>>
{
	using type = init_value<init<VAL, ETS...>>;
};

} //end: namespace detail

template <class VALUE>
using as_writable_t = detail::numeric_value<typename VALUE::traits>;

/**
 * generic value - a facade for numeric_values above
 */
template <class T, class... EXT_TRAITS>
struct value;

template <AValueTraits T>
struct value<T> : detail::value_selector<T>::type {};

template<Arithmetic T, class... EXT_TRAITS>
struct value<T, EXT_TRAITS...>
		: detail::numeric_value<value_traits<T, EXT_TRAITS...>> {};

template<std::size_t N, class... EXT_TRAITS>
struct value<bytes<N>, EXT_TRAITS...>
		: detail::numeric_value<value_traits<bytes<N>, EXT_TRAITS...>> {};

template<std::size_t N, class... EXT_TRAITS>
struct value<bits<N>, EXT_TRAITS...>
		: detail::numeric_value<value_traits<bits<N>, EXT_TRAITS...>> {};

} //namespace med
