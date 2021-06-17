#pragma once

#include "../value_traits.hpp"
#include "../tag.hpp"

namespace med::sl {

struct octet_info
{
	template <class IE>
	static constexpr std::size_t size_of()      { return bits_to_bytes(IE::traits::bits); }

	template <class IE>
	static constexpr auto produce_meta_info()
	{
		//TODO: process MI for multi_field itself? so far it's used only in ASN
		if constexpr (is_multi_field_v<IE>)
		{
			return meta::wrap<get_meta_info_t<typename IE::field_type>>{};
		}
		else
		{
			return meta::wrap<get_meta_info_t<IE>>{};
		}
	}
};

} //end: namespace med::sl
