/**
@file
helper macros for debugging via printfs/logs

@copyright Denis Priyomov 2016-2017
Distributed under the MIT License
(See accompanying file LICENSE or visit https://github.com/cppden/med)
*/

#pragma once
#include <cstdio>
#include <string_view>

//#define CODEC_TRACE_ENABLE

#define CODEC_TRACE(FMT, ...) CODEC_TRACE_FL(__FILE__, __LINE__, FMT, __VA_ARGS__)

#ifdef CODEC_TRACE_ENABLE

namespace med{

namespace debug {

// NOTE! fname is expected to be NULL-terminated thus allowing to return C-string
// this trick is only needed for constexpr since strrchr is not such.
constexpr char const* filename(std::string_view fname)
{
	if (auto pos = fname.rfind('/'); pos != std::string_view::npos)
	{
		return fname.substr(pos + 1).data();
	}
	else
	{
		return fname.data();
	}
}

} //end: namespace
} //end: namespace med

#define CODEC_TRACE_FL(F, L, FMT, ...) std::printf("%s:%u\t" FMT "\n", med::debug::filename(F), L, __VA_ARGS__)

#else
#define CODEC_TRACE_FL(...)
#endif
