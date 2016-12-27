/*!
@file
TODO: define.

@copyright Denis Priyomov 2016
Distributed under the MIT License
(See accompanying file LICENSE or copy at https://opensource.org/licenses/MIT)
*/

#pragma once

#include "ie.hpp"
#include "length.hpp"
#include "count.hpp"

namespace med {

template<
	class TAG_FLD,
	class FLD_LEN   = void,
	class FLD       = void,
	typename Enable = void >
struct optional
{
	static_assert(std::is_same<TAG_FLD, void>(), "MALFORMED OPTIONAL");
};


//optional field as a part of compound
template <class FIELD, class ISSETFUNC>
struct optional<
	FIELD,
	ISSETFUNC,
	void,
	std::enable_if_t<is_field_v<FIELD> && is_set_function_v<ISSETFUNC>>
> : field_t<FIELD>, optional_t
{
	using optional_type = ISSETFUNC;
};

template <class LEN, class FIELD>
struct optional<
	LEN,
	FIELD,
	void,
	std::enable_if_t<is_length_v<LEN> && is_field_v<FIELD>>
> : length_value_t<LEN, FIELD>, optional_t
{
};

//optional field as a part of compound
template <class LEN, class FIELD, class ISSETFUNC>
struct optional<
	LEN,
	FIELD,
	ISSETFUNC,
	std::enable_if_t<is_field_v<FIELD> && is_length_v<LEN> && is_set_function_v<ISSETFUNC>>
> : length_value_t<LEN, FIELD>, optional_t
{
	using optional_type = ISSETFUNC;
};

template <class FIELD>
struct optional<
	FIELD,
	void,
	void,
	std::enable_if_t<is_field_v<FIELD>>
> : field_t<FIELD>, optional_t
{
};

template <class SEQOF, class COUNTER>
struct optional<
	SEQOF,
	COUNTER,
	void,
	std::enable_if_t<is_sequence_of_v<SEQOF> && is_count_getter_v<COUNTER>>
> : field_t<SEQOF>, optional_t
{
	using count_getter = COUNTER;
};

template <class COUNTER, class SEQOF>
struct optional<
	COUNTER,
	SEQOF,
	void,
	std::enable_if_t<is_sequence_of_v<SEQOF> && is_counter_v<COUNTER>>
> : field_t<SEQOF>, COUNTER, optional_t
{
};

template <class TAG, class FIELD>
struct optional<
	TAG,
	FIELD,
	void,
	std::enable_if_t<is_tag_v<TAG> && is_field_v<FIELD>>
> : tag_value_t<TAG, FIELD>, optional_t
{
};

template <class TAG, class LEN, class FIELD>
struct optional<
	TAG,
	LEN,
	FIELD,
	std::enable_if_t<is_tag_v<TAG> && is_length_v<LEN> && is_field_v<FIELD>>
> : tag_value_t<TAG, length_value_t<LEN, FIELD>>, optional_t
{
};

//TODO: check field is sequence_of
template <class TAG, class COUNTER, class SEQOF>
struct optional<
	TAG,
	COUNTER,
	SEQOF,
	std::enable_if_t<is_tag_v<TAG> && is_counter_v<COUNTER> && is_sequence_of_v<SEQOF>>
> : tag_value_t<TAG, SEQOF>, COUNTER, optional_t
{
};

} //namespace med
