/**
@file
med exception is only used if MED_NO_EXCEPTIONS is defined

@copyright Denis Priyomov 2016-2017
Distributed under the MIT License
(See accompanying file LICENSE or visit https://github.com/cppden/med)
*/

#pragma once

#ifdef MED_NO_EXCEPTIONS

#define MED_RESULT         bool
#define MED_RETURN_SUCCESS return true
#define MED_RETURN_FAILURE return false
#define MED_AND            &&

#else //!MED_NO_EXCEPTIONS

#include <cstdio>
#include <cstdarg>
#include <exception>

#define MED_RESULT         void
#define MED_RETURN_SUCCESS return
#define MED_RETURN_FAILURE return
#define MED_AND            ,


namespace med {

class exception : public std::exception
{
public:
	exception(exception const&) = delete;
	exception& operator=(exception const&) = delete;

	exception(char const* fmt, ...) noexcept
	{
		va_list p;
		va_start(p, fmt);
		vformat(p, fmt);
		va_end(p);
	}

	virtual ~exception() noexcept                        { }

	virtual const char* what() const override noexcept   { return m_what; }

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

	char m_what[128];
};

}	//end: namespace med

#endif //MED_NO_EXCEPTIONS
