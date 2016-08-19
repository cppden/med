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
	class FLD_LEN_NUM = void,
	class FLD_NUM     = void,
	class MIN_MAX     = min<0>,
	class MAX_MIN     = max<1>,
	typename Enable   = void >
struct mandatory
{
	static_assert(std::is_same<TAG_FLD_LEN, void>(), "MALFORMED MANDATORY");
};

template <class FIELD>
struct mandatory<
	FIELD,
	void,
	void,
	min<0>,
	max<1>,
	std::enable_if_t<is_field<FIELD>::value>
> : field_t<FIELD>
{
};

template <class FIELD, class FUNC>
struct mandatory<
	FIELD,
	FUNC,
	void,
	min<0>,
	max<1>,
	std::enable_if_t<is_field<FIELD>::value && is_setter_v<FUNC>>
> : field_t<FIELD>
{
	using setter_type = FUNC;
};


template <class FIELD, std::size_t NUM>
struct mandatory<
	FIELD,
	arity<NUM>,
	void,
	min<0>,
	max<1>,
	std::enable_if_t<is_field<FIELD>::value>
> : multi_field_t<FIELD, NUM, NUM>
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
	std::enable_if_t<is_field<FIELD>::value && is_count_getter_v<COUNTER>>
> : multi_field_t<FIELD, MIN, MAX>
{
	using count_getter = COUNTER;
	static_assert(MIN > 0, "MIN SHOULD BE MORE THAN 0 OR NOT SPECIFIED");
	static_assert(MAX > MIN, "MAX SHOULD BE MORE THAN MIN OR NOT SPECIFIED");
};

template <class FIELD, std::size_t MIN, std::size_t MAX>
struct mandatory<
	FIELD,
	min<MIN>,
	max<MAX>,
	min<0>,
	max<1>,
	std::enable_if_t<is_tagged_field<FIELD>::value>
> : multi_field_t<FIELD, MIN, MAX>
{
	static_assert(MIN > 0, "MIN SHOULD BE MORE THAN 0 OR NOT SPECIFIED");
	static_assert(MAX > MIN, "MAX SHOULD BE MORE THAN MIN OR NOT SPECIFIED");
};

template <class FIELD, std::size_t MAX>
struct mandatory<
	FIELD,
	max<MAX>,
	void,
	min<0>,
	max<1>,
	std::enable_if_t<is_field<FIELD>::value>
> : multi_field_t<FIELD, 1, MAX>
{
	static_assert(MAX > 1, "MAX SHOULD BE MORE THAN 1 OR NOT SPECIFIED");
};

template <class FIELD, std::size_t MAX, class COUNTER>
struct mandatory<
	FIELD,
	max<MAX>,
	COUNTER,
	min<0>,
	max<1>,
	std::enable_if_t<is_field<FIELD>::value && is_count_getter_v<COUNTER>>
> : multi_field_t<FIELD, 1, MAX>
{
	using count_getter = COUNTER;
	static_assert(MAX > 1, "MAX SHOULD BE MORE THAN 1 OR NOT SPECIFIED");
};

template <class TAG, class FIELD>
struct mandatory<
	TAG,
	FIELD,
	void,
	min<0>,
	max<1>,
	std::enable_if_t<is_tag<TAG>::value && is_field<FIELD>::value>
> : tag_value_t<TAG, FIELD>
{
};

template <class TAG, class FIELD, std::size_t NUM>
struct mandatory<
	TAG,
	FIELD,
	arity<NUM>,
	min<0>,
	max<1>,
	std::enable_if_t<is_tag<TAG>::value && is_field<FIELD>::value>
> : multi_tag_value_t<TAG, FIELD, NUM, NUM>
{
	static_assert(NUM > 1, "ARITY SHOULD BE MORE THAN 1 OR NOT SPECIFIED");
};

template <class TAG, class FIELD, std::size_t MAX>
struct mandatory<
	TAG,
	FIELD,
	max<MAX>,
	min<0>,
	max<1>,
	std::enable_if_t<is_tag<TAG>::value && is_field<FIELD>::value>
> : multi_tag_value_t<TAG, FIELD, 1, MAX>
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
	std::enable_if_t<is_tag<TAG>::value && is_field<FIELD>::value>
> : multi_tag_value_t<TAG, FIELD, MIN, MAX>
{
	static_assert(MIN > 0, "MIN SHOULD BE MORE THAN 0 OR NOT SPECIFIED");
	static_assert(MAX > MIN, "MAX SHOULD BE MORE THAN MIN OR NOT SPECIFIED");
};

template <class LEN, class FIELD>
struct mandatory<
	LEN,
	FIELD,
	void,
	min<0>,
	max<1>,
	std::enable_if_t<is_field<FIELD>::value && !is_tag<LEN>::value>
> : length_value_t<LEN, FIELD>
{
};

template <class LEN, class FIELD, std::size_t NUM>
struct mandatory<
	LEN,
	FIELD,
	arity<NUM>,
	min<0>,
	max<1>,
	std::enable_if_t<is_field<FIELD>::value && !is_tag<LEN>::value>
> : multi_length_value_t<LEN, FIELD, NUM, NUM>
{
	static_assert(NUM > 1, "ARITY SHOULD BE MORE THAN 1 OR NOT SPECIFIED");
};

template <class LEN, class FIELD, std::size_t MAX>
struct mandatory<
	LEN,
	FIELD,
	max<MAX>,
	min<0>,
	max<1>,
	std::enable_if_t<is_field<FIELD>::value && !is_tag<LEN>::value>
> : multi_length_value_t<LEN, FIELD, 1, MAX>
{
	static_assert(MAX > 1, "MAX SHOULD BE MORE THAN 1 OR NOT SPECIFIED");
};

template <class TAG, class LEN, class FIELD>
struct mandatory<
	TAG,
	LEN,
	FIELD,
	min<0>,
	max<1>,
	std::enable_if_t<is_field<FIELD>::value && is_tag<TAG>::value && !is_tag<LEN>::value>
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
	std::enable_if_t<is_field<FIELD>::value && is_tag<TAG>::value && !is_tag<LEN>::value>
> : multi_tag_value_t<TAG, length_value_t<LEN, FIELD>, NUM, NUM>
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
	std::enable_if_t<is_field<FIELD>::value && is_tag<TAG>::value && !is_tag<LEN>::value>
> : multi_tag_value_t<TAG, length_value_t<LEN, FIELD>, 1, MAX>
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
	std::enable_if_t<is_field<FIELD>::value && is_tag<TAG>::value && !is_tag<LEN>::value>
> : multi_tag_value_t<TAG, length_value_t<LEN, FIELD>, MIN, MAX>
{
	static_assert(MIN > 0, "MIN SHOULD BE MORE THAN 0 OR NOT SPECIFIED");
	static_assert(MAX > MIN, "MAX SHOULD BE MORE THAN MIN OR NOT SPECIFIED");
};

} //namespace med
