#pragma once
/*
@file
named tags for text based codecs

@copyright Denis Priyomov 2018
Distributed under the MIT License
(See accompanying file LICENSE or visit https://github.com/cppden/ctstring)
*/

#include <cstdint>
#include <utility>
#include <type_traits>
#include <string_view>


namespace med {

namespace hash {

constexpr std::size_t init = 5381;

namespace detail {

constexpr std::size_t const_hash_impl(char const* end, std::size_t count)
{
	return count > 0
			? std::size_t(end[0]) + 33 * const_hash_impl(end - (count > 1 ? 1 : 0), count - 1)
			: init;
}

} //end: namespace detail

constexpr std::size_t compute(std::string_view const& sv)
{
	return detail::const_hash_impl(sv.data() + sv.size() - 1, sv.size());
}

constexpr std::size_t update(char const c, std::size_t hval)
{
	return ((hval << 5) + hval) + std::size_t(c); //33*hash + c
}

//Fowler–Noll–Vo : http://isthe.com/chongo/tech/comp/fnv/
// constexpr unsigned hash(int n=0, unsigned h=2166136261)
// {
// 	return n == size ? h : hash(n+1,(h * 16777619) ^ (sv[n]));
// }

} //end: namespace hash


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
	using ie_type = IE_OCTET_STRING;
	using name_type = std::string_view;
	using value_type = uint64_t;
	using tag_type = tag_named<NAME>;

	static constexpr name_type   name_value{ NAME::data(), NAME::size() };
	static constexpr value_type get()          { return hash::compute(name_value); }
	static constexpr value_type get_encoded()  { return get(); }

	//tag prereqs
	static constexpr bool is_const = true;
	static constexpr bool match(value_type v)  { return get() == v; }

	//oct-string prereqs
	static constexpr std::size_t size()        { return NAME::size(); }
	static constexpr auto* data()              { return NAME::data(); }
	enum limits_e : std::size_t
	{
		min_octets = NAME::size(),
		max_octets = min_octets,
	};
};


} //end: namespace med

