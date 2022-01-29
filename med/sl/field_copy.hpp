#pragma once

/**
@file
copy a field.

@copyright Denis Priyomov 2016-2018
Distributed under the MIT License
(See accompanying file LICENSE or visit https://github.com/cppden/med)
*/

#include "../field.hpp"

namespace med::sl {

template <class IE, class ...ARGS>
constexpr void field_copy(IE& to, IE const& from, ARGS&&... args)
{
	if (from.is_set())
	{
		if constexpr (is_multi_field_v<IE>)
		{
			to.clear();
			for (auto const& rhs : from)
			{
				auto* p = to.push_back(std::forward<ARGS>(args)...);
				p->copy(rhs, std::forward<ARGS>(args)...);
			}
		}
		else
		{
			return to.copy(from, std::forward<ARGS>(args)...);
		}
	}
}

} //namespace med::sl
