/**
@file
snapshot updating entry point

@copyright Denis Priyomov 2016-2017
Distributed under the MIT License
(See accompanying file LICENSE or visit https://github.com/cppden/med)
*/

#pragma once

#include "functional.hpp"
#include "state.hpp"
#include "encode.hpp"
#include "debug.hpp"


namespace med {

template <class FUNC, class IE>
inline void update(FUNC&& func, IE const& ie)
{
	static_assert(has_ie_type<IE>(), "IE IS EXPECTED");
	static_assert(std::is_base_of<with_snapshot, IE>(), "IE WITH med::with_snapshot EXPECTED");
	CODEC_TRACE("update %s", name<IE>());
	func(SET_STATE{}, ie);
	sl::encode_ie<IE>(func, ie, typename IE::ie_type{});
}

}	//end: namespace med
