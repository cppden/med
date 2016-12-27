/*!
@file
TODO: define.

@copyright Denis Priyomov 2016
Distributed under the MIT License
(See accompanying file LICENSE or copy at https://opensource.org/licenses/MIT)
*/

#pragma once

#include "ie.hpp"
#include "count.hpp"

namespace med {

template<
	class TAG_FLD_LEN,
	class FLD_LEN   = void,
	class FLD       = void,
	typename Enable = void >
struct mandatory
{
	static_assert(std::is_same<TAG_FLD_LEN, void>(), "MALFORMED MANDATORY");
};

template <class FIELD>
struct mandatory<
	FIELD,
	void,
	void,
	std::enable_if_t<is_field_v<FIELD>>
> : field_t<FIELD>
{
};

template <class FIELD, class FUNC>
struct mandatory<
	FIELD,
	FUNC,
	void,
	std::enable_if_t<is_field_v<FIELD> && is_setter_v<FUNC>>
> : field_t<FIELD>
{
	using setter_type = FUNC;
};

template <class SEQOF, class COUNTER>
struct mandatory<
	SEQOF,
	COUNTER,
	void,
	std::enable_if_t<is_sequence_of_v<SEQOF> && is_count_getter_v<COUNTER>>
> : field_t<SEQOF>
{
	using count_getter = COUNTER;
};

template <class COUNTER, class SEQOF>
struct mandatory<
	COUNTER,
	SEQOF,
	void,
	std::enable_if_t<is_sequence_of_v<SEQOF> && is_counter_v<COUNTER>>
> : field_t<SEQOF>, COUNTER
{
};

template <class TAG, class FIELD>
struct mandatory<
	TAG,
	FIELD,
	void,
	std::enable_if_t<is_tag_v<TAG> && is_field_v<FIELD>>
> : tag_value_t<TAG, FIELD>
{
};

template <class LEN, class FIELD>
struct mandatory<
	LEN,
	FIELD,
	void,
	std::enable_if_t<is_field_v<FIELD> && is_length_v<LEN>>
> : length_value_t<LEN, FIELD>
{
};

template <class TAG, class LEN, class FIELD>
struct mandatory<
	TAG,
	LEN,
	FIELD,
	std::enable_if_t<is_field_v<FIELD> && is_tag_v<TAG> && is_length_v<LEN>>
> : tag_value_t<TAG, length_value_t<LEN, FIELD>>
{
};

} //namespace med
