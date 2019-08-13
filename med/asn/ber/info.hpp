#pragma once

#include "meta/typelist.hpp"

namespace med::asn::ber {

struct info
{
	template <class T>
	static constexpr auto get_tag_type()    { return meta::wrap<void>{}; }
};

} //end: namespace med::asn::ber

