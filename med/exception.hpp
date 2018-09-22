/**
@file
med exception is only used if MED_EXCEPTIONS=1 was defined

@copyright Denis Priyomov 2016-2017
Distributed under the MIT License
(See accompanying file LICENSE or visit https://github.com/cppden/med)
*/

#pragma once

#include <cstdio>
#include <exception>

#include "config.hpp"
#include "error.hpp"
#include "debug.hpp"

namespace med {

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

#if (MED_EXCEPTIONS)

#define MED_RESULT           void
#define MED_RETURN_SUCCESS   return
#define MED_RETURN_FAILURE   return
#define MED_AND              ,
#define MED_EXPR_AND(expr)
#define MED_CHECK_FAIL(expr) expr

class exception : public std::exception
{
public:
	exception& operator=(exception const&) = delete;

	virtual ~exception() noexcept = default;

	virtual const char* what() const noexcept override   { return m_what; }

protected:
	//only derived can be created
	exception() = default;

	char m_what[128];
};

//OVERFLOW
struct overflow : exception
{
//	using exception::exception;
	template <typename... ARGS>
	overflow(ARGS&&... args) noexcept	{ format(this->m_what, std::forward<ARGS>(args)...); }
};

//INVALID_VALUE,
//UNKNOWN_TAG
struct value_exception : public exception {};

struct invalid_value : public value_exception
{
//	using value_exception::value_exception;
	template <typename... ARGS>
	invalid_value(ARGS&&... args) noexcept	{ format(this->m_what, std::forward<ARGS>(args)...); }
};
struct unknown_tag : public value_exception
{
//	using value_exception::value_exception;
	template <typename... ARGS>
	unknown_tag(ARGS&&... args) noexcept	{ format(this->m_what, std::forward<ARGS>(args)...); }
};

//MISSING_IE
//EXTRA_IE
struct ie_exception : public exception {};

struct missing_ie : public ie_exception
{
//	using ie_exception::ie_exception;
	template <typename... ARGS>
	missing_ie(ARGS&&... args) noexcept		{ format(this->m_what, std::forward<ARGS>(args)...); }
};
struct extra_ie : public ie_exception
{
//	using ie_exception::ie_exception;
	template <typename... ARGS>
	extra_ie(ARGS&&... args) noexcept		{ format(this->m_what, std::forward<ARGS>(args)...); }
};

//OUT_OF_MEMORY
struct out_of_memory : public exception
{
//	using exception::exception;
	template <typename... ARGS>
	out_of_memory(ARGS&&... args) noexcept	{ format(this->m_what, std::forward<ARGS>(args)...); }
};

#else //w/o exceptions

#define MED_RESULT           bool
#define MED_RETURN_SUCCESS   return true
#define MED_RETURN_FAILURE   return false
#define MED_AND              &&
#define MED_EXPR_AND(expr)   (expr) &&
#define MED_CHECK_FAIL(expr) if (!(expr)) MED_RETURN_FAILURE

#endif //MED_EXCEPTIONS

#define MED_RETURN_ERROR(err, func, ...) \
	{ CODEC_TRACE("%s", #err); return func(err, __VA_ARGS__); }

}	//end: namespace med
