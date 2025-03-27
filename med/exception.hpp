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
#include <cstring>
#include <exception>
#include <type_traits>

#include "config.hpp"
#include "debug.hpp"

namespace med {

class exception : public std::exception
{
public:
	exception& operator=(exception const&) = delete;
	~exception() noexcept override = default;
	const char* what() const noexcept override   { return m_what; }

protected:
	//only derived can be created
	exception() = default;

	template <typename... ARGS>
	void format(char const* bufpos, char const* fmt, ARGS&&... args) noexcept
	{
		int res = std::snprintf(m_what, sizeof(m_what), fmt, std::forward<ARGS>(args)...);
		if (bufpos && res > 0 && res < int(sizeof(m_what)))
		{
			std::strncpy(m_what + res, bufpos, sizeof(m_what) - res - 1);
		}
		m_what[sizeof(m_what) - 1] = 0;
	}

	char m_what[128];
};

//OVERFLOW
struct overflow : exception
{
	overflow(char const* name, std::size_t bytes, char const* bufpos = nullptr) noexcept
		{ format(bufpos, "'%.64s' needs %zu octets.", name, bytes); }

	template <class CTX>
	overflow(char const* name, std::size_t bytes, CTX const& ctx) noexcept
		: overflow{name, bytes, ctx.toString()} {}
};

struct value_exception : public exception {};

struct invalid_value : public value_exception
{
	invalid_value(char const* name, std::size_t val, char const* bufpos = nullptr) noexcept
		{ format(bufpos, "Invalid value of '%.64s' = 0x%zX.", name, val); }

	template <class CTX>
	invalid_value(char const* name, std::size_t val, CTX const& ctx) noexcept
		: invalid_value{name, val, ctx.toString()} {}
};
struct unknown_tag : public value_exception
{
	unknown_tag(char const* name, std::size_t val, char const* bufpos = nullptr) noexcept
		{ format(bufpos, "Unknown tag of '%.64s' = 0x%zX.", name, val); }

	template <class CTX>
	unknown_tag(char const* name, std::size_t val, CTX const& ctx) noexcept
		: unknown_tag{name, val, ctx.toString()} {}
};

struct ie_exception : public exception {};

struct missing_ie : public ie_exception
{
	missing_ie(char const* name, std::size_t exp, std::size_t got, char const* bufpos = nullptr) noexcept
		{ format(bufpos, "Missing IE '%.64s': at least %zu expected, got %zu.", name, exp, got); }

	template <class CTX>
	missing_ie(char const* name, std::size_t exp, std::size_t got, CTX const& ctx) noexcept
		: missing_ie{name, exp, got, ctx.toString()} {}
};
struct extra_ie : public ie_exception
{
	extra_ie(char const* name, std::size_t exp, std::size_t got, char const* bufpos = nullptr) noexcept
		{ format(bufpos, "Excessive IE '%.64s': no more than %zu expected, got %zu.", name, exp, got); }

	template <class CTX>
	extra_ie(char const* name, std::size_t exp, std::size_t got, CTX const& ctx) noexcept
		: extra_ie{name, exp, got, ctx.toString()} {}
};

//OUT_OF_MEMORY
struct out_of_memory : public exception
{
	out_of_memory(char const* name, std::size_t bytes, char const* bufpos = nullptr) noexcept
		{ format(bufpos, "No space to allocate '%.64s': %zu octets.", name, bytes); }

	template <class CTX>
	out_of_memory(char const* name, std::size_t bytes, CTX const& ctx) noexcept
		: out_of_memory{name, bytes, ctx.toString()} {}
};

#define MED_THROW_EXCEPTION(ex, ...) { CODEC_TRACE("THROW: %s", #ex); throw ex(__VA_ARGS__); }

}	//end: namespace med
