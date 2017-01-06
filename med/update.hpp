/**
@file
snapshot updating entry point

@copyright Denis Priyomov 2016
Distributed under the MIT License
(See accompanying file LICENSE or copy at https://opensource.org/licenses/MIT)
*/

#pragma once

#include "functional.hpp"
#include "state.hpp"
#include "encode.hpp"
#include "debug.hpp"


namespace med {

template <class FUNC, class IE>
inline bool update(FUNC&& func, IE const& ie)
{
	static_assert(has_ie_type<IE>(), "IE IS EXPECTED");
	CODEC_TRACE("update %s", name<typename IE::ie_type>());
	return func(SET_STATE{}, ie) && sl::encode<IE>(func, ie, typename IE::ie_type{});
}

}	//end: namespace med
