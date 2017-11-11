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
inline MED_RESULT update(FUNC&& func, IE const& ie)
{
	static_assert(has_ie_type<IE>(), "IE IS EXPECTED");
	CODEC_TRACE("update %s", name<typename IE::ie_type>());
	return func(SET_STATE{}, ie)
		MED_AND sl::encode_ie<IE>(func, ie, typename IE::ie_type{});
}

}	//end: namespace med
