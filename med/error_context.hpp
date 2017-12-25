/**
@file
error context to hold current error codes and their related arguments

@copyright Denis Priyomov 2016-2017
Distributed under the MIT License
(See accompanying file LICENSE or visit https://github.com/cppden/med)
*/

#pragma once

#include <cstdint>
#include <cstddef>
#include <cstdio>

#include "exception.hpp"
#include "error.hpp"
#include "debug.hpp"

namespace med {

class error_context
{
public:

#if (MED_EXCEPTIONS)
	static constexpr void reset()          { }
	void set_error(error err, char const* name = nullptr, std::size_t val0 = 0, std::size_t val1 = 0)
	{
		if (error::SUCCESS != err)
		{
			CODEC_TRACE("ERROR[%s]=%d %zu %zu", name, static_cast<int>(err), val0, val1);

			switch (err)
			{
			case error::OVERFLOW:
				if (val1)
				{
					throw exception(err, "%zu bits left, '%s' needs %zu", val0, name, val1);
				}
				throw exception(err, "%zu bits left after '%s'", val0, name);

			case error::INCORRECT_VALUE:
				throw exception(err, "Invalid value of '%s' at %zu: 0x%zX", name, val1, val0);

			case error::INCORRECT_TAG:
				throw exception(err, "Wrong tag of '%s': %zu", name, val0);

			case error::MISSING_IE:
				throw exception(err, "Missing IE '%s': at least %zu expected, got %zu", name, val0, val1);

			case error::EXTRA_IE:
				throw exception(err, "Excessive IE '%s': no more than %zu expected, got %zu", name, val0, val1);

			case error::OUT_OF_MEMORY:
				throw exception(err, "No space to allocate IE '%s': %zu bytes", name, val0);

			default:
				break;
			}
		}
	}

#else

	explicit operator bool() const         { return success(); }
	bool success() const                   { return error::SUCCESS == m_error; }
	error get_error() const                { return m_error; }

	void reset()                           { m_error = error::SUCCESS; }
	bool set_error(error err, char const* name = nullptr, std::size_t val0 = 0, std::size_t val1 = 0)
	{
		CODEC_TRACE("ERROR[%s]=%d %zu %zu", name, static_cast<int>(err), val0, val1);
		m_name     = name;
		m_param[0] = val0;
		m_param[1] = val1;
		m_error    = err;
		return success();
	}

private:
	friend char const* toString(error_context const&);

	enum { MAX_PARAMS = 2 };

	char const* m_name;
	std::size_t m_param[MAX_PARAMS];
	error       m_error{ error::SUCCESS };

#endif //MED_EXCEPTIONS

public:
	//TODO: include buffer offset in all errors
	MED_RESULT spaceError(char const* name, std::size_t bits_left, std::size_t requested)
		{ return set_error(error::OVERFLOW, name, bits_left, requested); }

	MED_RESULT valueError(char const* name, std::size_t got, std::size_t ofs)
		{ return set_error(error::INCORRECT_VALUE, name, got, ofs); }

	MED_RESULT allocError(char const* name, std::size_t requested)
		{ return set_error(error::OUT_OF_MEMORY, name, requested); }
};

#if (MED_EXCEPTIONS)
#else
//TODO: add offsets for all errors
inline char const* toString(error_context const& ec)
{
	static char sz[80];

	switch (ec.m_error)
	{
	case error::SUCCESS:
		return nullptr;

	case error::OVERFLOW:
		if (ec.m_param[1])
		{
			std::snprintf(sz, sizeof(sz), "%zu bits left, '%s' needs %zu", ec.m_param[0], ec.m_name, ec.m_param[1]);
		}
		else
		{
			std::snprintf(sz, sizeof(sz), "%zu bits left after '%s'", ec.m_param[0], ec.m_name);
		}
		break;

	case error::INCORRECT_VALUE:
		std::snprintf(sz, sizeof(sz), "Invalid value of '%s' at %zu: 0x%zX", ec.m_name, ec.m_param[1], ec.m_param[0]);
		break;

	case error::INCORRECT_TAG:
		std::snprintf(sz, sizeof(sz), "Wrong tag of '%s': %zu (0x%zX)", ec.m_name, ec.m_param[0], ec.m_param[0]);
		break;

	case error::MISSING_IE:
		std::snprintf(sz, sizeof(sz), "Missing IE '%s': at least %zu expected, got %zu", ec.m_name, ec.m_param[0], ec.m_param[1]);
		break;

	case error::EXTRA_IE:
		std::snprintf(sz, sizeof(sz), "Excessive IE '%s': no more than %zu expected, got %zu", ec.m_name, ec.m_param[0], ec.m_param[1]);
		break;

	case error::OUT_OF_MEMORY:
		std::snprintf(sz, sizeof(sz), "No space to allocate IE '%s': %zu bytes", ec.m_name, ec.m_param[0]);
		break;
	}

	return sz;
}
#endif //MED_EXCEPTIONS

}	//end: namespace med
