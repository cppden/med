/**
@file
med exception is only used if MED_EXCEPTIONS=1 was defined

@copyright Denis Priyomov 2016-2017
Distributed under the MIT License
(See accompanying file LICENSE or visit https://github.com/cppden/med)
*/

#pragma once

#include "config.hpp"

#if (MED_EXCEPTIONS)

#include <cstdio>
#include <exception>

#include "error.hpp"

#define MED_RESULT           void
#define MED_RETURN_SUCCESS   return
#define MED_RETURN_FAILURE   return
#define MED_AND              ,
#define MED_EXPR_AND(expr)
#define MED_CHECK_FAIL(expr) expr


namespace med {

class exception : public std::exception
{
public:
	exception& operator=(exception const&) = delete;

	virtual ~exception() noexcept = default;

	virtual const char* what() const noexcept override   { return m_what; }

protected:
	//only derived can be created
	exception() = default;
//	template <typename... ARGS>
//	exception(char const* fmt, ARGS&&... args) noexcept { format(fmt, std::forward<ARGS>(args)...); }

	template <typename... ARGS>
	void format(char const* fmt, ARGS&&... args) noexcept
	{
		std::snprintf(m_what, sizeof(m_what), fmt, std::forward<ARGS>(args)...);
	}

private:
	char       m_what[128];
};

//OVERFLOW
struct overflow : exception
{
//	using exception::exception;
	template <typename... ARGS>
	overflow(char const* fmt, ARGS&&... args) noexcept	{ format(fmt, std::forward<ARGS>(args)...); }
};

//INCORRECT_VALUE,
//INCORRECT_TAG
struct value_exception : public exception {};

struct invalid_value : public value_exception
{
//	using value_exception::value_exception;
	template <typename... ARGS>
	invalid_value(char const* fmt, ARGS&&... args) noexcept	{ format(fmt, std::forward<ARGS>(args)...); }
};
struct invalid_tag : public value_exception
{
//	using value_exception::value_exception;
	template <typename... ARGS>
	invalid_tag(char const* fmt, ARGS&&... args) noexcept	{ format(fmt, std::forward<ARGS>(args)...); }
};

//MISSING_IE
//EXTRA_IE
struct ie_exception : public exception {};

struct missing_ie : public ie_exception
{
//	using ie_exception::ie_exception;
	template <typename... ARGS>
	missing_ie(char const* fmt, ARGS&&... args) noexcept	{ format(fmt, std::forward<ARGS>(args)...); }
};
struct extra_ie : public ie_exception
{
//	using ie_exception::ie_exception;
	template <typename... ARGS>
	extra_ie(char const* fmt, ARGS&&... args) noexcept	{ format(fmt, std::forward<ARGS>(args)...); }
};

//OUT_OF_MEMORY
struct out_of_memory : public exception
{
//	using exception::exception;
	template <typename... ARGS>
	out_of_memory(char const* fmt, ARGS&&... args) noexcept	{ format(fmt, std::forward<ARGS>(args)...); }
};

}	//end: namespace med

#else //w/o exceptions

#define MED_RESULT           bool
#define MED_RETURN_SUCCESS   return true
#define MED_RETURN_FAILURE   return false
#define MED_AND              &&
#define MED_EXPR_AND(expr)   (expr) &&
#define MED_CHECK_FAIL(expr) if (!(expr)) MED_RETURN_FAILURE

#endif //MED_EXCEPTIONS
