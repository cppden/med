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

struct octet_error_ctx_traits
{
	//static constexpr std::size_t unit_size = 8;
	static constexpr char const* overflow()      { return "%zu octets left, '%.32s' needs %zu"; }
	static constexpr char const* invalid_value() { return "Invalid value of '%.32s' at %zu: 0x%zX"; }
	static constexpr char const* unknown_tag()   { return "Unknown tag of '%.32s': %zu (0x%zX)"; }
	static constexpr char const* missing_ie()    { return "Missing IE '%.32s': at least %zu expected, got %zu"; }
	static constexpr char const* excessive_ie()  { return "Excessive IE '%.32s': no more than %zu expected, got %zu"; }
	static constexpr char const* out_of_memory() { return "No space to allocate IE '%.32s': %zu octets"; }
};

template <class TRAITS>
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
				throw overflow(TRAITS::overflow(), val0, name, val1);

			case error::INVALID_VALUE:
				throw invalid_value(TRAITS::invalid_value(), name, val1, val0);

			case error::UNKNOWN_TAG:
				throw unknown_tag(TRAITS::unknown_tag(), name, val0);

			case error::MISSING_IE:
				throw missing_ie(TRAITS::missing_ie(), name, val0, val1);

			case error::EXTRA_IE:
				throw extra_ie(TRAITS::excessive_ie(), name, val0, val1);

			case error::OUT_OF_MEMORY:
				throw out_of_memory(TRAITS::out_of_memory(), name, val0);

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
	template <class T>
	friend char const* toString(error_context<T> const&);

	char const* m_name {nullptr};
	std::size_t m_param[2] {};
	error       m_error { error::SUCCESS };

#endif //MED_EXCEPTIONS

public:
	//TODO: include buffer offset in all errors
};

#if (MED_EXCEPTIONS)
#else
//TODO: add offsets for all errors
template <class TRAITS>
inline char const* toString(error_context<TRAITS> const& ec)
{
	static char sz[128];

	switch (ec.m_error)
	{
	case error::SUCCESS:
		return nullptr;

	case error::OVERFLOW:
		std::snprintf(sz, sizeof(sz), TRAITS::overflow(), ec.m_param[0], ec.m_name, ec.m_param[1]);
		break;

	case error::INVALID_VALUE:
		std::snprintf(sz, sizeof(sz), TRAITS::invalid_value(), ec.m_name, ec.m_param[1], ec.m_param[0]);
		break;

	case error::UNKNOWN_TAG:
		std::snprintf(sz, sizeof(sz), TRAITS::unknown_tag(), ec.m_name, ec.m_param[0], ec.m_param[0]);
		break;

	case error::MISSING_IE:
		std::snprintf(sz, sizeof(sz), TRAITS::missing_ie(), ec.m_name, ec.m_param[0], ec.m_param[1]);
		break;

	case error::EXTRA_IE:
		std::snprintf(sz, sizeof(sz), TRAITS::excessive_ie(), ec.m_name, ec.m_param[0], ec.m_param[1]);
		break;

	case error::OUT_OF_MEMORY:
		std::snprintf(sz, sizeof(sz), TRAITS::out_of_memory(), ec.m_name, ec.m_param[0]);
		break;
	}

	return sz;
}
#endif //MED_EXCEPTIONS

}	//end: namespace med
