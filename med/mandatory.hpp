/**
@file
set of templates to define mandatory fields within containers

@copyright Denis Priyomov 2016-2017
Distributed under the MIT License
(See accompanying file LICENSE or visit https://github.com/cppden/med)
*/

#pragma once

#include "multi_field.hpp"
#include "count.hpp"

namespace med {

template<
	class TAG_FLD_LEN,
	class FLD_LEN_NUM = void,
	class FLD_NUM     = void,
	class MIN_MAX     = min<1>,
	class MAX_MIN     = max<1>,
	typename Enable   = void >
struct mandatory
{
	static_assert(std::is_void<TAG_FLD_LEN>(), "MALFORMED MANDATORY");
};

template <class FIELD>
struct mandatory<
	FIELD,
	void,
	void,
	min<1>,
	max<1>,
	std::enable_if_t<is_field_v<FIELD>>
> : field_t<FIELD>
{
};

template <class FIELD, class SETTER>
struct mandatory<
	FIELD,
	SETTER,
	void,
	min<1>,
	max<1>,
	std::enable_if_t<is_field_v<FIELD> && is_setter_v<FIELD, SETTER>>
> : field_t<FIELD>
{
	using setter_type = SETTER;
};


template <class FIELD, std::size_t NUM>
struct mandatory<
	FIELD,
	arity<NUM>,
	void,
	min<1>,
	max<1>,
	std::enable_if_t<is_field_v<FIELD>>
> : multi_field<FIELD, NUM, max<NUM>>
{
	static_assert(NUM > 1, "ARITY SHOULD BE MORE THAN 1 OR NOT SPECIFIED");
};

//mandatory multi-instance field as a part of compound
template <class FIELD, std::size_t MIN, std::size_t MAX, class COUNTER>
struct mandatory<
	FIELD,
	min<MIN>,
	max<MAX>,
	COUNTER,
	max<1>,
	std::enable_if_t<is_field_v<FIELD> && is_count_getter_v<COUNTER>>
> : multi_field<FIELD, MIN, max<MAX>>
{
	using count_getter = COUNTER;
	static_assert(MIN > 1, "MIN SHOULD BE MORE THAN 1 OR NOT SPECIFIED");
	static_assert(MAX > MIN, "MAX SHOULD BE MORE THAN MIN");
};

template <class FIELD, std::size_t MIN, std::size_t MAX, class COUNTER>
struct mandatory<
	FIELD,
	min<MIN>,
	pmax<MAX>,
	COUNTER,
	max<1>,
	std::enable_if_t<is_field_v<FIELD> && is_count_getter_v<COUNTER>>
> : multi_field<FIELD, MIN, pmax<MAX>>
{
	using count_getter = COUNTER;
	static_assert(MIN > 1, "MIN SHOULD BE MORE THAN 1 OR NOT SPECIFIED");
	static_assert(MAX > MIN, "MAX SHOULD BE MORE THAN MIN");
};

template <class COUNTER, class FIELD, std::size_t MIN, std::size_t MAX>
struct mandatory<
	COUNTER,
	FIELD,
	min<MIN>,
	max<MAX>,
	max<1>,
	std::enable_if_t<is_field_v<FIELD> && is_counter_v<COUNTER>>
> : multi_field<FIELD, MIN, max<MAX>>, COUNTER
{
	static_assert(MIN > 1, "MIN SHOULD BE MORE THAN 1 OR NOT SPECIFIED");
	static_assert(MAX > MIN, "MAX SHOULD BE MORE THAN MIN");
};

template <class COUNTER, class FIELD, std::size_t MIN, std::size_t MAX>
struct mandatory<
	COUNTER,
	FIELD,
	min<MIN>,
	pmax<MAX>,
	max<1>,
	std::enable_if_t<is_field_v<FIELD> && is_counter_v<COUNTER>>
> : multi_field<FIELD, MIN, pmax<MAX>>, COUNTER
{
	static_assert(MIN > 1, "MIN SHOULD BE MORE THAN 1 OR NOT SPECIFIED");
	static_assert(MAX > MIN, "MAX SHOULD BE MORE THAN MIN");
};

template <class FIELD, std::size_t MIN, std::size_t MAX>
struct mandatory<
	FIELD,
	min<MIN>,
	max<MAX>,
	min<1>,
	max<1>,
	std::enable_if_t<is_tagged_field<FIELD>::value>
> : multi_field<FIELD, MIN, max<MAX>>
{
	static_assert(MIN > 1, "MIN SHOULD BE MORE THAN 1 OR NOT SPECIFIED");
	static_assert(MAX > MIN, "MAX SHOULD BE MORE THAN MIN");
};

template <class FIELD, std::size_t MIN, std::size_t MAX>
struct mandatory<
	FIELD,
	min<MIN>,
	pmax<MAX>,
	min<1>,
	max<1>,
	std::enable_if_t<is_tagged_field<FIELD>::value>
> : multi_field<FIELD, MIN, pmax<MAX>>
{
	static_assert(MIN > 1, "MIN SHOULD BE MORE THAN 1 OR NOT SPECIFIED");
	static_assert(MAX > MIN, "MAX SHOULD BE MORE THAN MIN");
};

template <class FIELD, std::size_t MAX>
struct mandatory<
	FIELD,
	max<MAX>,
	void,
	min<1>,
	max<1>,
	std::enable_if_t<is_field_v<FIELD>>
> : multi_field<FIELD, 1, max<MAX>>
{
	static_assert(MAX > 1, "MAX SHOULD BE MORE THAN 1 OR NOT SPECIFIED");
};

template <class FIELD, std::size_t MAX>
struct mandatory<
	FIELD,
	pmax<MAX>,
	void,
	min<1>,
	max<1>,
	std::enable_if_t<is_field_v<FIELD>>
> : multi_field<FIELD, 1, pmax<MAX>>
{
	static_assert(MAX > 1, "MAX SHOULD BE MORE THAN 1 OR NOT SPECIFIED");
};

template <class FIELD, std::size_t MAX, class COUNTER>
struct mandatory<
	FIELD,
	max<MAX>,
	COUNTER,
	min<1>,
	max<1>,
	std::enable_if_t<is_field_v<FIELD> && is_count_getter_v<COUNTER>>
> : multi_field<FIELD, 1, max<MAX>>
{
	using count_getter = COUNTER;
	static_assert(MAX > 1, "MAX SHOULD BE MORE THAN 1 OR NOT SPECIFIED");
};

template <class FIELD, std::size_t MAX, class COUNTER>
struct mandatory<
	FIELD,
	pmax<MAX>,
	COUNTER,
	min<1>,
	max<1>,
	std::enable_if_t<is_field_v<FIELD> && is_count_getter_v<COUNTER>>
> : multi_field<FIELD, 1, pmax<MAX>>
{
	using count_getter = COUNTER;
	static_assert(MAX > 1, "MAX SHOULD BE MORE THAN 1 OR NOT SPECIFIED");
};

template <class COUNTER, class FIELD, std::size_t MAX>
struct mandatory<
	COUNTER,
	FIELD,
	max<MAX>,
	min<1>,
	max<1>,
	std::enable_if_t<is_field_v<FIELD> && is_counter_v<COUNTER>>
> : multi_field<FIELD, 1, max<MAX>>, COUNTER
{
	static_assert(MAX > 1, "MAX SHOULD BE MORE THAN 1 OR NOT SPECIFIED");
};
template <class COUNTER, class FIELD, std::size_t MAX>
struct mandatory<
	COUNTER,
	FIELD,
	pmax<MAX>,
	min<1>,
	max<1>,
	std::enable_if_t<is_field_v<FIELD> && is_counter_v<COUNTER>>
> : multi_field<FIELD, 1, pmax<MAX>>, COUNTER
{
	static_assert(MAX > 1, "MAX SHOULD BE MORE THAN 1 OR NOT SPECIFIED");
};

template <class TAG, class FIELD>
struct mandatory<
	TAG,
	FIELD,
	void,
	min<1>,
	max<1>,
	std::enable_if_t<is_tag_v<TAG> && is_field_v<FIELD>>
> : tag_value_t<TAG, FIELD>
{
};

template <class TAG, class FIELD, std::size_t NUM>
struct mandatory<
	TAG,
	FIELD,
	arity<NUM>,
	min<1>,
	max<1>,
	std::enable_if_t<is_tag_v<TAG> && is_field_v<FIELD>>
> : multi_tag_value<TAG, FIELD, NUM, max<NUM>>
{
	static_assert(NUM > 1, "ARITY SHOULD BE MORE THAN 1 OR NOT SPECIFIED");
};

template <class TAG, class FIELD, std::size_t MAX>
struct mandatory<
	TAG,
	FIELD,
	max<MAX>,
	min<1>,
	max<1>,
	std::enable_if_t<is_tag_v<TAG> && is_field_v<FIELD>>
> : multi_tag_value<TAG, FIELD, 1, max<MAX>>
{
	static_assert(MAX > 1, "MAX SHOULD BE MORE THAN 1 OR NOT SPECIFIED");
};

template <class TAG, class FIELD, std::size_t MAX>
struct mandatory<
	TAG,
	FIELD,
	pmax<MAX>,
	min<1>,
	max<1>,
	std::enable_if_t<is_tag_v<TAG> && is_field_v<FIELD>>
> : multi_tag_value<TAG, FIELD, 1, pmax<MAX>>
{
	static_assert(MAX > 1, "MAX SHOULD BE MORE THAN 1 OR NOT SPECIFIED");
};

template <class TAG, class FIELD, std::size_t MIN, std::size_t MAX>
struct mandatory<
	TAG,
	FIELD,
	min<MIN>,
	max<MAX>,
	max<1>,
	std::enable_if_t<is_tag_v<TAG> && is_field_v<FIELD>>
> : multi_tag_value<TAG, FIELD, MIN, max<MAX>>
{
	static_assert(MIN > 1, "MIN SHOULD BE MORE THAN 1 OR NOT SPECIFIED");
	static_assert(MAX > MIN, "MAX SHOULD BE MORE THAN MIN");
};

template <class TAG, class FIELD, std::size_t MIN, std::size_t MAX>
struct mandatory<
	TAG,
	FIELD,
	min<MIN>,
	pmax<MAX>,
	max<1>,
	std::enable_if_t<is_tag_v<TAG> && is_field_v<FIELD>>
> : multi_tag_value<TAG, FIELD, MIN, pmax<MAX>>
{
	static_assert(MIN > 1, "MIN SHOULD BE MORE THAN 1 OR NOT SPECIFIED");
	static_assert(MAX > MIN, "MAX SHOULD BE MORE THAN MIN");
};

template <class LEN, class FIELD>
struct mandatory<
	LEN,
	FIELD,
	void,
	min<1>,
	max<1>,
	std::enable_if_t<is_field_v<FIELD> && is_length_v<LEN>>
> : length_value_t<LEN, FIELD>
{
};

template <class LEN, class FIELD, std::size_t NUM>
struct mandatory<
	LEN,
	FIELD,
	arity<NUM>,
	min<1>,
	max<1>,
	std::enable_if_t<is_field_v<FIELD> && is_length_v<LEN>>
> : multi_length_value<LEN, FIELD, NUM, max<NUM>>
{
	static_assert(NUM > 1, "ARITY SHOULD BE MORE THAN 1 OR NOT SPECIFIED");
};

template <class LEN, class FIELD, std::size_t MAX>
struct mandatory<
	LEN,
	FIELD,
	max<MAX>,
	min<1>,
	max<1>,
	std::enable_if_t<is_field_v<FIELD> && is_length_v<LEN>>
> : multi_length_value<LEN, FIELD, 1, max<MAX>>
{
	static_assert(MAX > 1, "MAX SHOULD BE MORE THAN 1 OR NOT SPECIFIED");
};
template <class LEN, class FIELD, std::size_t MAX>
struct mandatory<
	LEN,
	FIELD,
	pmax<MAX>,
	min<1>,
	max<1>,
	std::enable_if_t<is_field_v<FIELD> && is_length_v<LEN>>
> : multi_length_value<LEN, FIELD, 1, pmax<MAX>>
{
	static_assert(MAX > 1, "MAX SHOULD BE MORE THAN 1 OR NOT SPECIFIED");
};


template <class TAG, class LEN, class FIELD>
struct mandatory<
	TAG,
	LEN,
	FIELD,
	min<1>,
	max<1>,
	std::enable_if_t<is_field_v<FIELD> && is_tag_v<TAG> && is_length_v<LEN>>
> : tag_value_t<TAG, length_value_t<LEN, FIELD>>
{
};

template <class TAG, class LEN, class FIELD, std::size_t NUM>
struct mandatory<
	TAG,
	LEN,
	FIELD,
	arity<NUM>,
	max<1>,
	std::enable_if_t<is_field_v<FIELD> && is_tag_v<TAG> && is_length_v<LEN>>
> : multi_tag_value<TAG, length_value_t<LEN, FIELD>, NUM, max<NUM>>
{
	static_assert(NUM > 1, "ARITY SHOULD BE MORE THAN 1 OR NOT SPECIFIED");
};

template <class TAG, class LEN, class FIELD, std::size_t MAX>
struct mandatory<
	TAG,
	LEN,
	FIELD,
	max<MAX>,
	max<1>,
	std::enable_if_t<is_field_v<FIELD> && is_tag_v<TAG> && is_length_v<LEN>>
> : multi_tag_value<TAG, length_value_t<LEN, FIELD>, 1, max<MAX>>
{
	static_assert(MAX > 1, "MAX SHOULD BE MORE THAN 1 OR NOT SPECIFIED");
};
template <class TAG, class LEN, class FIELD, std::size_t MAX>
struct mandatory<
	TAG,
	LEN,
	FIELD,
	pmax<MAX>,
	max<1>,
	std::enable_if_t<is_field_v<FIELD> && is_tag_v<TAG> && is_length_v<LEN>>
> : multi_tag_value<TAG, length_value_t<LEN, FIELD>, 1, pmax<MAX>>
{
	static_assert(MAX > 1, "MAX SHOULD BE MORE THAN 1 OR NOT SPECIFIED");
};


template <class TAG, class LEN, class FIELD, std::size_t MIN, std::size_t MAX>
struct mandatory<
	TAG,
	LEN,
	FIELD,
	min<MIN>,
	max<MAX>,
	std::enable_if_t<is_field_v<FIELD> && is_tag_v<TAG> && is_length_v<LEN>>
> : multi_tag_value<TAG, length_value_t<LEN, FIELD>, MIN, max<MAX>>
{
	static_assert(MIN > 1, "MIN SHOULD BE MORE THAN 1 OR NOT SPECIFIED");
	static_assert(MAX > MIN, "MAX SHOULD BE MORE THAN MIN");
};

template <class TAG, class LEN, class FIELD, std::size_t MIN, std::size_t MAX>
struct mandatory<
	TAG,
	LEN,
	FIELD,
	min<MIN>,
	pmax<MAX>,
	std::enable_if_t<is_field_v<FIELD> && is_tag_v<TAG> && is_length_v<LEN>>
> : multi_tag_value<TAG, length_value_t<LEN, FIELD>, MIN, pmax<MAX>>
{
	static_assert(MIN > 1, "MIN SHOULD BE MORE THAN 1 OR NOT SPECIFIED");
	static_assert(MAX > MIN, "MAX SHOULD BE MORE THAN MIN");
};

} //namespace med
