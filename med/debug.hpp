/*!
@file
TODO: define.

@copyright Denis Priyomov 2016
Distributed under the MIT License
(See accompanying file LICENSE or copy at https://opensource.org/licenses/MIT)
*/

#pragma once

//#define CODEC_TRACE_ENABLE


#ifdef CODEC_TRACE_ENABLE

//#define CODEC_TRACE(FMT, ...) printf("%s[%u]:" FMT "\n", std::strrchr(__FILE__, PS) + 1, __LINE__, __VA_ARGS__)
#define CODEC_TRACE(FMT, ...) printf("%s[%u]:" FMT "\n", __FILE__, __LINE__, __VA_ARGS__)

#else
#define CODEC_TRACE(...)
#endif
