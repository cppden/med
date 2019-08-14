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
		if constexpr (std::is_void_v<IE>)
		{
			return meta::wrap<void>{};
		}
		else
		{
			constexpr bool constructed = std::is_base_of_v<CONTAINER, typename IE::ie_type>;
			using tv = tag_value<typename IE::traits, constructed>;
			using tag_type = med::value<
				med::fixed< tv::value(), med::bytes<tv::num_bytes()>/*, typename IE::traits*/ >
			>;
			return meta::wrap<tag_type>{};
		}
	}
};

} //end: namespace med::asn::ber

