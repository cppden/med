/**
@file
snapshot updating entry point

@copyright Denis Priyomov 2016-2017
Distributed under the MIT License
(See accompanying file LICENSE or visit https://github.com/cppden/med)
*/

#pragma once

#include "concepts.hpp"
#include "state.hpp"
#include "encode.hpp"
#include "debug.hpp"


namespace med {

template <class FUNC, AHasIeType IE>
constexpr void update(FUNC&& func, IE const& ie)
{
	static_assert(std::is_base_of<with_snapshot, IE>(), "IE WITH med::with_snapshot IS EXPECTED");
	CODEC_TRACE("update %s", name<IE>());
	func(SET_STATE{}, ie);
	encode(func, ie);
}

}	//end: namespace med
