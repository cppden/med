#pragma once

#include "value_traits.hpp"
#include "tag.hpp"

namespace med::sl {

struct octet_info
{
	template <class IE>
	static constexpr std::size_t size_of()      { return bits_to_bytes(IE::traits::bits); }

	template <class IE>
	static constexpr auto produce_meta_info()
	{
		return meta::wrap<get_meta_info_t<IE>>{};
	}
};

} //end: namespace med::sl
