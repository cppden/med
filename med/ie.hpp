/*!
@file
TODO: define.

@copyright Denis Priyomov 2016
Distributed under the MIT License
(See accompanying file LICENSE or copy at https://opensource.org/licenses/MIT)
*/

#pragma once

#include "ie_type.hpp"
#include "tag.hpp"
#include "length.hpp"


namespace med {

template <typename PTR>
class buffer;


template <class FIELD>
struct field_t : FIELD
{
	using field_type = FIELD;

	field_type& ref_field()             { return *this; }
	field_type const& ref_field() const { return *this; }
};


template <class TAG, class VAL>
struct tag_value_t : field_t<VAL>, tag_t<TAG>
{
	using ie_type = IE_TV;
};

template <class LEN, class VAL>
struct length_value_t : field_t<VAL>, LEN
{
	using ie_type = IE_LV;
};

template <class FIELD, std::size_t MIN, std::size_t MAX>
class multi_field_t
{
public:
	using ie_type = typename FIELD::ie_type;
	using field_type = FIELD;

	enum : std::size_t
	{
		min = MIN,
		max = MAX,
	};

	template <std::size_t INDEX>
	auto const& ref_field() const                   { return m_fields[INDEX]; }
	template <std::size_t INDEX>
	auto& ref_field()                               { return m_fields[INDEX]; }


	template <std::size_t INDEX>
	auto const* get_field() const
	{
		static_assert(INDEX < max, "INDEX OUT OF BOUNDS");
		return m_fields[INDEX].is_set() ? &m_fields[INDEX] : nullptr;
	}

	auto const* get_field(std::size_t index) const
	{
		return (index < max && m_fields[index].is_set()) ? &m_fields[index] : nullptr;
	}
	auto* get_field(std::size_t index)
	{
		return (index < max) ? &m_fields[index] : nullptr;
	}

	auto* next_field()
	{
		std::size_t const count = field_count(*this);
		return (count < max) ? &m_fields[count] : nullptr;
	}

private:
	FIELD   m_fields[max];
};


template <class TAG, class VAL, std::size_t MIN, std::size_t MAX>
struct multi_tag_value_t : multi_field_t<VAL, MIN, MAX>, tag_t<TAG>
{
	using ie_type = IE_TV;
};

template <class LEN, class VAL, std::size_t MIN, std::size_t MAX>
struct multi_length_value_t : multi_field_t<VAL, MIN, MAX>, LEN
{
	using ie_type = IE_LV;
};

template <std::size_t MAX>
struct max : std::integral_constant<std::size_t, MAX> {};

template <std::size_t MIN>
struct min : std::integral_constant<std::size_t, MIN> {};

template <std::size_t N>
struct arity : std::integral_constant<std::size_t, N> {};


template <class T, class Enable = void>
struct is_field : std::false_type {};
template <class T>
struct is_field<T, std::enable_if_t<
	std::is_same<bool, decltype(std::declval<T>().is_set())>::value
	>
> : std::true_type {};
template <class T>
constexpr bool is_field_v = is_field<T>::value;


template <class T, class Enable = void>
struct is_tagged_field : std::false_type {};
template <class T>
struct is_tagged_field<T, std::enable_if_t<is_field_v<T> && has_tag_type_v<T>>> : std::true_type {};
template <class T>
constexpr bool is_tagged_field_v = is_tagged_field<T>::value;

}	//end: namespace med
