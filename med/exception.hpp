/**
@file
med exception is only used if MED_NO_EXCEPTION is defined

@copyright Denis Priyomov 2016-2017
Distributed under the MIT License
(See accompanying file LICENSE or visit https://github.com/cppden/med)
*/

#pragma once


#ifdef MED_NO_EXCEPTION

#define MED_RESULT           bool
#define MED_RETURN_SUCCESS   return true
#define MED_RETURN_FAILURE   return false
#define MED_AND              &&
#define MED_EXPR_AND(expr)   (expr) &&
#define MED_CHECK_FAIL(expr) if (!(expr)) MED_RETURN_FAILURE

#else //!MED_NO_EXCEPTION

#include <cstdio>
#include <cstdarg>
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
	//exception(exception const&) = delete;
	exception& operator=(exception const&) = delete;

	exception(med::error err, char const* fmt, ...) noexcept
		: m_error{ err }
	{
		va_list p;
		va_start(p, fmt);
		vformat(p, fmt);
		va_end(p);
	}

	virtual ~exception() noexcept                        { }

	virtual const char* what() const noexcept override   { return m_what; }
	med::error error() const noexcept                    { return m_error; }

protected:
	void format(char const* fmt, ...) noexcept
	{
		va_list p;
		va_start(p, fmt);
		vformat(p, fmt);
		va_end(p);
	}

	void vformat(va_list pArg, char const* fmt) noexcept
	{
		std::vsnprintf(m_what, sizeof(m_what), fmt, pArg);
	}

	med::error m_error;
	char       m_what[128];

};

}	//end: namespace med

#endif //MED_NO_EXCEPTION
