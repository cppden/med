#pragma once

#include "value_traits.hpp"

namespace med {
namespace sl {

namespace detail {

template <class, class = void >
struct has_tag_type : std::false_type { };
template <class T>
struct has_tag_type<T, std::void_t<typename T::tag_type>> : std::true_type { };

template <class IE, typename Enable = void>
struct tag_type
{
	using type = void;
};

template <class IE>
struct tag_type<IE, std::enable_if_t<sl::detail::has_tag_type<IE>::value>>
{
	using type = typename IE::tag_type;
};

template <class IE>
struct tag_type<IE, std::enable_if_t<!sl::detail::has_tag_type<IE>::value && sl::detail::has_tag_type<typename IE::field_type>::value>>
{
	using type = typename IE::field_type::tag_type;
};

} //end: namespace detail


struct octet_info
{
	template <class IE>
	static constexpr std::size_t size_of()      { return bits_to_bytes(IE::traits::bits); }

	template <class IE>
	static constexpr auto get_tag_type()        { return typename detail::tag_type<IE>{}; }
};

} //end: namespace sl
} //end: namespace med
