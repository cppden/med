/**
@file
error context to hold current error/warning codes and their related arguments

@copyright Denis Priyomov 2016
Distributed under the MIT License
(See accompanying file LICENSE or copy at https://opensource.org/licenses/MIT)
*/

#pragma once

#include <cstdint>
#include <cstddef>
#include <cstdio>

#include "error.hpp"
#include "debug.hpp"

namespace med {

class error_context
{
public:
	explicit operator bool() const          { return error::SUCCESS == m_error; }
	error get_error() const                 { return m_error; }

	void reset()                            { m_error = error::SUCCESS; }
	void set_error(error err, char const* name = nullptr, std::size_t val0 = 0, std::size_t val1 = 0, std::size_t val2 = 0)
	{
		CODEC_TRACE("ERROR[%s]=%d %zu %zu %zu", name, static_cast<int>(err), val0, val1, val2);
		m_name     = name;
		m_param[0] = val0;
		m_param[1] = val1;
		m_param[2] = val2;
		m_error    = err;
	}

	warning get_warning() const             { return m_warning; }
	void set_warning(warning warn)          { m_warning = warn; }

	//TODO: include buffer offset in all errors
	void spaceError(char const* name, std::size_t bits_left, std::size_t requested)
		{ set_error(error::OVERFLOW, name, bits_left, requested); }

	void valueError(char const* name, std::size_t got, std::size_t ofs)
		{ set_error(error::INCORRECT_VALUE, name, got, ofs); }

	void allocError(char const* name, std::size_t requested)
		{ set_error(error::OUT_OF_MEMORY, name, requested); }

private:
	friend char const* toString(error_context const&);

	enum { MAX_PARAMS = 3 };

	char const* m_name;
	std::size_t m_param[MAX_PARAMS];
	error       m_error{ error::SUCCESS };
	warning     m_warning{ warning::NONE };
};

//TODO: add offsets for all errors
inline char const* toString(error_context const& ec)
{
	static char sz[80];

	switch (ec.m_error)
	{
	case error::SUCCESS:
		return "";

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


}	//end: namespace med
