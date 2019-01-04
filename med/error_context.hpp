/**
@file
error context to hold current error codes and their related arguments

@copyright Denis Priyomov 2016-2017
Distributed under the MIT License
(See accompanying file LICENSE or visit https://github.com/cppden/med)
*/

#pragma once

#include <cstddef>
#include <cstdio>
#include <iostream>

#include "exception.hpp"
#include "error.hpp"

namespace med {

struct octet_error_ctx_traits
{
	//static constexpr std::size_t unit_size = 8;
	static constexpr char const* overflow()      { return "'%.32s' needs %zu octets. "; }
	static constexpr char const* invalid_value() { return "Invalid value of '%.32s' at %zu: 0x%zX. "; }
	static constexpr char const* unknown_tag()   { return "Unknown tag of '%.32s': %zu (0x%zX). "; }
	static constexpr char const* missing_ie()    { return "Missing IE '%.32s': at least %zu expected, got %zu. "; }
	static constexpr char const* excessive_ie()  { return "Excessive IE '%.32s': no more than %zu expected, got %zu. "; }
	static constexpr char const* out_of_memory() { return "No space to allocate IE '%.32s': %zu octets. "; }
};


template <class TRAITS>
class error_context
{
public:
	using traits = TRAITS;

	template <class BUFFER>
	static void set_error(BUFFER const& bufpos, error err, char const* name, std::size_t val0 = 0, std::size_t val1 = 0)
	{
		if (error::SUCCESS != err)
		{
			char const* psz_buf = nullptr;
			if constexpr (std::is_class_v<BUFFER>)
			{
				psz_buf = bufpos.toString();
			}

			switch (err)
			{
			case error::OVERFLOW:
				throw overflow(psz_buf, traits::overflow(), name, val1);

			case error::INVALID_VALUE:
				throw invalid_value(psz_buf, traits::invalid_value(), name, val1, val0);

			case error::UNKNOWN_TAG:
				throw unknown_tag(psz_buf, traits::unknown_tag(), name, val0);

			case error::MISSING_IE:
				throw missing_ie(psz_buf, traits::missing_ie(), name, val0, val1);

			case error::EXTRA_IE:
				throw extra_ie(psz_buf, traits::excessive_ie(), name, val0, val1);

			case error::OUT_OF_MEMORY:
				throw out_of_memory(psz_buf, traits::out_of_memory(), name, val0);

			default:
				break;
			}
		}
	}
};


}	//end: namespace med
