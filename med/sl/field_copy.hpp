#pragma once

/**
@file
copy a field.

@copyright Denis Priyomov 2016-2018
Distributed under the MIT License
(See accompanying file LICENSE or visit https://github.com/cppden/med)
*/

#include "../multi_field.hpp"

namespace med::sl {

template <class IE, class ...ARGS>
inline MED_RESULT field_copy(IE& to, IE const& from, ARGS&&... args)
{
	if (from.is_set())
	{
		if constexpr (is_multi_field_v<IE>)
		{
			to.clear();
			for (auto const& rhs : from)
			{
				if (auto* p = to.push_back(std::forward<ARGS>(args)...))
				{
					MED_CHECK_FAIL(p->copy(rhs, std::forward<ARGS>(args)...));
				}
				else
				{
					MED_RETURN_FAILURE;
				}
			}
		}
		else
		{
			return to.ref_field().copy(from.ref_field(), std::forward<ARGS>(args)...);
		}
	}
	MED_RETURN_SUCCESS;
}

} //namespace med::sl
