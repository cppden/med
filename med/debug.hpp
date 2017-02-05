/**
@file
helper macros for debugging via printfs/logs

@copyright Denis Priyomov 2016-2017
Distributed under the MIT License
(See accompanying file LICENSE or visit https://github.com/cppden/med)
*/

#pragma once
#include <cstdio>
#include <cstring>

//#define CODEC_TRACE_ENABLE


#ifdef CODEC_TRACE_ENABLE

namespace med{

namespace {

inline char const* filename(char const* fname)
{
	if (char const* p = std::strrchr(fname, '/'))
	{
		return p + 1;
	}
	else
	{
		return fname;
	}
}

} //end: namespace
} //end: namespace med

#define CODEC_TRACE(FMT, ...) std::printf("%s[%u]:" FMT "\n", med::filename(__FILE__), __LINE__, __VA_ARGS__)

#else
#define CODEC_TRACE(...)
#endif
