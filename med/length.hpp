/*!
@file
TODO: define.

@copyright Denis Priyomov 2016
Distributed under the MIT License
(See accompanying file LICENSE or copy at https://opensource.org/licenses/MIT)
*/

#pragma once

#include "ie_type.hpp"
#include "value_traits.hpp"
#include "name.hpp"
#include "debug.hpp"


namespace med {

template <class LENGTH>
struct length_t
{
	using length_type = LENGTH;
};


template <class, class Enable = void>
struct has_length_type : std::false_type { };

template <class T>
struct has_length_type<T, void_t<typename T::length_type>> : std::true_type { };


namespace detail {

template <class WRAPPER, typename Enable = void>
struct length_getter;

}


template <class FIELD>
constexpr std::size_t get_length(FIELD const& field)
{
	return detail::length_getter<FIELD>::get(field);
}

template <class WRAPPER, class FIELD, typename Enable = std::enable_if_t<!std::is_same<WRAPPER, FIELD>::value>>
constexpr std::size_t get_length(FIELD const& field)
{
	return detail::length_getter<WRAPPER>::get(field);
}

namespace detail {

template <class WRAPPER, class FIELD>
constexpr std::size_t calc_length(FIELD const& field, IE_NULL const&)
{
	return 0;
}

template <class WRAPPER, class FIELD>
constexpr std::size_t calc_length(FIELD const& field, CONTAINER const&)
{
	CODEC_TRACE("calc_length(%s)...", name<FIELD>());
	return field.calc_length();
}

template <class, typename Enable = void>
struct has_get_length : std::false_type { };

template <class T>
struct has_get_length<T, void_t<decltype(std::declval<T>().get_length())>> : std::true_type { };

template <class WRAPPER, class FIELD>
std::enable_if_t<!has_get_length<FIELD>::value, std::size_t>
constexpr calc_length(FIELD const&, PRIMITIVE const&)
{
	//TODO: assuming length in bytes if IE is not customized
	CODEC_TRACE("length(%s) = %zu", name<FIELD>(), bits_to_bytes<FIELD::traits::bits>::value);
	return bits_to_bytes<FIELD::traits::bits>::value;
}

template <class WRAPPER, class FIELD>
std::enable_if_t<has_get_length<FIELD>::value, std::size_t>
inline calc_length(FIELD const& field, PRIMITIVE const&)
{
	CODEC_TRACE("length(%s) = %zu", name<FIELD>(), field.get_length());
	return field.get_length();
}

template <class WRAPPER, class FIELD>
constexpr std::size_t calc_length(FIELD const& field, IE_TV const&)
{
	using tag_t = typename WRAPPER::tag_type;
	CODEC_TRACE("calc_length(%s) : TV", name<FIELD>());
	return get_length(tag_t{}) + get_length<FIELD>(ref_field(field));
}

template <class WRAPPER, class FIELD>
constexpr std::size_t calc_length(FIELD const& field, IE_LV const&)
{
	using len_t = typename WRAPPER::length_type;
	CODEC_TRACE("calc_length(%s) : LV", name<FIELD>());
	return get_length(len_t{}) + get_length<FIELD>(ref_field(field));
}


template <class WRAPPER>
struct length_getter<WRAPPER, std::enable_if_t<!is_read_only_v<WRAPPER>>>
{
	template <class FIELD>
	static std::size_t get(FIELD const& field)
	{
		return detail::calc_length<WRAPPER>(field, typename WRAPPER::ie_type{});
	}
};

template <class WRAPPER>
struct length_getter<WRAPPER, std::enable_if_t<is_read_only_v<WRAPPER>>>
{
	template <class FIELD>
	static constexpr std::size_t get(FIELD const&)  { return 0; }
};

//---------------------------------

template<class T>
static auto test_value_to_length(int, std::size_t val = 0) ->
	std::enable_if_t<
		std::is_same<bool, decltype(T::value_to_length(val))>::value, std::true_type
	>;

template<class>
static auto test_value_to_length(long) -> std::false_type;

}	//end: namespace detail

template <class T>
using has_length_converters = decltype(detail::test_value_to_length<T>(0));


template <class FIELD>
std::enable_if_t<has_length_converters<FIELD>::value, bool>
inline length_to_value(FIELD& field, std::size_t len)
{
	return FIELD::length_to_value(len) && set_value(field, len);
}

template <class FIELD>
std::enable_if_t<!has_length_converters<FIELD>::value, bool>
inline length_to_value(FIELD& field, std::size_t len)
{
	return set_value(field, len);
}

template <class FIELD>
std::enable_if_t<has_length_converters<FIELD>::value, bool>
inline value_to_length(FIELD const& field, std::size_t& len)
{
	return FIELD::value_to_length(len);
}

template <class FIELD>
std::enable_if_t<!has_length_converters<FIELD>::value, bool>
constexpr value_to_length(FIELD const&, std::size_t&)
{
	return true;
}

}	//end: namespace med
