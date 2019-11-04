#pragma once

#include "meta/typelist.hpp"
#include "value.hpp"
#include "asn/ber/tag.hpp"

namespace med::asn::ber {

struct info
{
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
		constexpr bool constructed = std::is_base_of_v<CONTAINER, typename IE::ie_type>;
		using tv = tag_value<typename IE::traits, constructed>;
		using type = med::value<
			med::fixed< tv::value(), med::bytes<tv::num_bytes()> >
		>;
		return meta::wrap<type>{};
	}
};

} //end: namespace med::asn::ber

