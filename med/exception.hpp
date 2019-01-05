/**
@file
med exceptions

@copyright Denis Priyomov 2016-2017
Distributed under the MIT License
(See accompanying file LICENSE or visit https://github.com/cppden/med)
*/

#pragma once

#include <cstdint>
#include <cstdio>
#include <exception>
#undef OVERFLOW

#include "config.hpp"
#include "debug.hpp"

namespace med {

namespace detail {

template <class, class Enable = void >
struct has_to_str : std::false_type { };
template <class T>
struct has_to_str<T,
	std::enable_if_t<
		std::is_same_v<char const*, decltype(std::declval<T>().toString())>
	>
> : std::true_type { };
template <class T>
constexpr bool has_to_str_v = has_to_str<T>::value;

template <class T>
inline auto to_str(T& t) -> std::enable_if_t<has_to_str_v<T>, char const*>
{
	return t.toString();
}
template <class T>
inline auto to_str(T& t) -> std::enable_if_t<has_to_str_v<t.buffer()>, char const*>
{
	return t.buffer().toString();
}
template <class T>
inline auto to_str(T& t) -> std::enable_if_t<has_to_str_v<t.ctx.buffer()>, char const*>
{
	return t.ctx.buffer().toString();
}

}

template <int N, typename... ARGS>
char const* format(char(&out)[N], char const* bufpos, char const* fmt, ARGS&&... args) noexcept
{
	int res = std::snprintf(out, sizeof(out), fmt, std::forward<ARGS>(args)...);
	if (bufpos && res > 0 && res < N)
	{
		std::strncpy(out + res, bufpos, sizeof(out) - res);
	}
	out[N-1] = 0;
	return out;
}

class exception : public std::exception
{
public:
	exception& operator=(exception const&) = delete;
	~exception() noexcept override = default;
	const char* what() const noexcept override   { return m_what; }

protected:
	//only derived can be created
	exception() = default;

	char m_what[128];
};

//OVERFLOW
struct overflow : exception
{
	overflow(char const* name, std::size_t bytes, char const* aux = nullptr) noexcept
		{ format(this->m_what, "'%.32s' needs %zu octets. %s", name, bytes, aux?aux:""); }

	template <class CTX>
	overflow(char const* name, std::size_t bytes, CTX const& ctx) noexcept
		: overflow{name, bytes, detail::to_str(ctx)} {}
};

struct value_exception : public exception {};

struct invalid_value : public value_exception
{
	invalid_value(char const* name, std::size_t val, char const* aux = nullptr) noexcept
		{ format(this->m_what, "Invalid value of '%.32s' = 0x%zX. %s", name, val, aux?aux:""); }

	template <class CTX>
	invalid_value(char const* name, std::size_t val, CTX const& ctx) noexcept
		: invalid_value{name, val, detail::to_str(ctx)} {}
};
struct unknown_tag : public value_exception
{
	unknown_tag(char const* name, std::size_t val, char const* aux = nullptr) noexcept
		{ format(this->m_what, "Unknown tag of '%.32s' = 0x%zX. %s", name, val, aux?aux:""); }

	template <class CTX>
	unknown_tag(char const* name, std::size_t val, CTX const& ctx) noexcept
		: unknown_tag{name, val, detail::to_str(ctx)} {}
};

struct ie_exception : public exception {};

struct missing_ie : public ie_exception
{
	missing_ie(char const* name, std::size_t exp, std::size_t got, char const* aux = nullptr) noexcept
		{ format(this->m_what, "Missing IE '%.32s': at least %zu expected, got %zu. %s", name, exp, got, aux?aux:""); }

	template <class CTX>
	missing_ie(char const* name, std::size_t exp, std::size_t got, CTX const& ctx) noexcept
		: missing_ie{name, exp, got, detail::to_str(ctx)} {}
};
struct extra_ie : public ie_exception
{
	extra_ie(char const* name, std::size_t exp, std::size_t got, char const* aux = nullptr) noexcept
		{ format(this->m_what, "Excessive IE '%.32s': no more than %zu expected, got %zu. %s", name, exp, got, aux?aux:""); }

	template <class CTX>
	extra_ie(char const* name, std::size_t exp, std::size_t got, CTX const& ctx) noexcept
		: extra_ie{name, exp, got, detail::to_str(ctx)} {}
};

//OUT_OF_MEMORY
struct out_of_memory : public exception
{
	out_of_memory(char const* name, std::size_t bytes, char const* aux = nullptr) noexcept
		{ format(this->m_what, "No space to allocate IE '%.32s': %zu octets. %s", name, bytes, aux?aux:""); }

	template <class CTX>
	out_of_memory(char const* name, std::size_t bytes, CTX const& ctx) noexcept
		: out_of_memory{name, bytes, detail::to_str(ctx)} {}
};

#define MED_THROW_EXCEPTION(ex, ...) { CODEC_TRACE("THROW: %s", #ex); throw ex(__VA_ARGS__); }

}	//end: namespace med
