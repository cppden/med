#pragma once

#include "meta/typelist.hpp"
#include "value.hpp"
#include "asn/ber/tag.hpp"

namespace med::asn::ber {

struct info
{
	template <class IE>
	static constexpr auto get_tag_type()
	{
//		using meta_info = get_meta_info_t<IE>;
//		if constexpr (meta::list_is_empty_v<meta_info>)
//		{
//			return meta::wrap<>{};
//		}
//		else
//		{
//			using mi_1st = meta::list_first_t<meta_info>;
//			if constexpr (mi_1st::kind == mik::TAG)
//			{
//				return meta::wrap<mi_1st>{};
//			}
//			else
//			{
//				return meta::wrap<>{};
//			}
//		}

		if constexpr (std::is_void_v<IE>)
		{
			return meta::wrap<void>{};
		}
		else
		{
			constexpr bool constructed = std::is_base_of_v<CONTAINER, typename IE::ie_type>;
			using tv = tag_value<typename IE::traits, constructed>;
			using type = med::value<
				med::fixed< tv::value(), med::bytes<tv::num_bytes()>/*, typename IE::traits*/ >
			>;
			return meta::wrap<type>{};
		}
	}
};

} //end: namespace med::asn::ber

