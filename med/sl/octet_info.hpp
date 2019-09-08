#pragma once

#include "value_traits.hpp"
#include "tag.hpp"

namespace med::sl {

struct octet_info
{
	template <class IE>
	static constexpr std::size_t size_of()      { return bits_to_bytes(IE::traits::bits); }

	template <class IE>
	static constexpr auto get_tag_type()
	{
		using meta_info = get_meta_info_t<IE>;
		if constexpr (meta::list_is_empty_v<meta_info>)
		{
			return meta::wrap<>{};
		}
		else
		{
			using mi_1st = meta::list_first_t<meta_info>;
			if constexpr (mi_1st::kind == mik::TAG)
			{
				return meta::wrap<mi_1st>{};
			}
			else
			{
				return meta::wrap<>{};
			}
		}
	}
};

} //end: namespace med::sl
