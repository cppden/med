#pragma once
/*
@file
named tags for text based codecs

@copyright Denis Priyomov 2018
Distributed under the MIT License
(See accompanying file LICENSE or visit https://github.com/cppden/ctstring)
*/

#include <utility>
#include <type_traits>

#include "ie_type.hpp"
#include "hash.hpp"

namespace med {

constexpr char to_upper(char const c)
{
	return (c >= 'a' && c <= 'z') ? c - ('a' - 'A') : c;
}

constexpr char to_lower(char const c)
{
	return (c >= 'A' && c <= 'Z') ? c + ('a' - 'A') : c;
}

constexpr bool is_equal(char const c1, char const c2)
{
	return c1 == c2;
}

constexpr bool is_iequal(char const c1, char const c2)
{
	return c1 == c2 || to_upper(c1) == c2;
}


template <char... CHARS>
class Chars
{
public:
	static constexpr std::size_t size()     { return sizeof...(CHARS); }
	static constexpr char const* data()     { return m_string; }

private:
	static constexpr char m_string[] = {CHARS...};
};

template <char... CHARS>
constexpr char Chars<CHARS...>::m_string[];

namespace literals {

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wgnu-string-literal-operator-template"
#endif

template <typename T, T... CHARS>
constexpr Chars<CHARS...> operator""_name() { return { }; }

#ifdef __clang__
#pragma clang diagnostic pop
#endif

} //end: namespace literals

template <class NAME>
struct tag_named
{
	using traits = detail::octet_traits<char, NAME::size()>;
	using ie_type = IE_OCTET_STRING;
	using name_type = std::string_view;
	using value_type = uint64_t;
	using tag_type = tag_named<NAME>;

	static constexpr name_type   name_value{ NAME::data(), NAME::size() };
	static constexpr value_type get()          { return hash<value_type>::compute(name_value); }
	static constexpr value_type get_encoded()  { return get(); }

	//tag prereqs
	static constexpr bool is_const = true;
	static constexpr bool match(value_type v)  { return get() == v; }

	//oct-string prereqs
	static constexpr std::size_t size()        { return NAME::size(); }
	static constexpr auto* data()              { return NAME::data(); }
};


} //end: namespace med

