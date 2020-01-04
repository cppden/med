#pragma once
#include <utility>

#include "meta/typelist.hpp"
#include "value.hpp"
#include "../../length.hpp"
#include "asn/ber/tag.hpp"

namespace med::asn::ber {

struct info
{
private:
	template <class T, class CONSTRUCTED = std::true_type>
	struct make_tag
	{
		template <class V>
		using tag_of = med::value< med::fixed< V::value(), med::bytes<V::num_bytes()> > >;
		using type = mi< mik::TAG, tag_of<tag_value<T, CONSTRUCTED::value>> >;
	};

	template <std::size_t I, std::size_t N>
	struct not_last
	{
		using type = std::bool_constant<I < (N-1)>;
	};


public:
	template <class IE>
	static constexpr auto produce_meta_info()
	{
/*
8.14 Encoding of a value of a prefixed type
8.14.2 Encoding of tagged value is derived from complete encoding of corresponding value of the type
appearing in "TaggedType" notation (called the base encoding) as specified in 8.14.3 and 8.14.4.
8.14.3 If implicit tagging (see X.680, 31.2.7) was not used, then:
the encoding is constructed and contents octets is the complete base encoding.
8.14.4 If implicit tagging was used, then:
a) the encoding is constructed if the base encoding is constructed, and primitive otherwise;
b) the contents octets shall be the same as the contents octets of the base encoding.
*/
		constexpr auto get_tags = []
		{
			using asn_traits = get_meta_info_t<IE>;

			if constexpr (std::is_base_of_v<CONTAINER, typename IE::ie_type>)
			{
				return meta::wrap<meta::transform_t<asn_traits, make_tag>>{};
			}
			else
			{
				return meta::wrap<meta::transform_indexed_t<asn_traits, make_tag, not_last>>{};
			}
		};

		//!TODO: LENSIZE need to calc len to known its size (only 1 byte for now)
		using len_t = mi<mik::LEN, med::length_t<med::value<uint8_t>>>;
		using meta_info = meta::interleave_t< meta::unwrap_t<decltype(get_tags())>, len_t>;
		return meta::wrap<meta_info>{};
	}
};

} //end: namespace med::asn::ber

