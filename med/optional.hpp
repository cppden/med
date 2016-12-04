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
	class FLD_LEN = void,
	class FLD_NUM = void,
	class MIN_MAX = min<0>,
	class MAX_MIN = max<1>,
	typename Enable = void >
struct optional
{
	static_assert(std::is_same<TAG_FLD, void>(), "MALFORMED OPTIONAL");
};


//optional field as a part of compound
template <class FIELD, class IS_SET>
struct optional<
	FIELD,
	IS_SET,
	void,
	min<0>,
	max<1>,
	std::enable_if_t<is_field<FIELD>::value && is_set_function_v<IS_SET>>
> : field_t<FIELD>, optional_t
{
	using optional_type = IS_SET;
};

//optional field as a part of compound
template <class LEN, class FIELD, class IS_SET>
struct optional<
	LEN,
	FIELD,
	IS_SET,
	min<0>,
	max<1>,
	std::enable_if_t<is_field<FIELD>::value && is_length<LEN>::value && is_set_function_v<IS_SET>>
> : length_value_t<LEN, FIELD>, optional_t
{
	using optional_type = IS_SET;
};

template <class FIELD>
struct optional<
	FIELD,
	void,
	void,
	min<0>,
	max<1>,
	std::enable_if_t<is_field<FIELD>::value>
> : field_t<FIELD>, optional_t
{
};

template <class FIELD, std::size_t NUM>
struct optional<
	FIELD,
	arity<NUM>,
	void,
	min<0>,
	max<1>,
	std::enable_if_t<is_tagged_field<FIELD>::value>
> : multi_field_t<FIELD, NUM, NUM>, optional_t
{
	static_assert(NUM > 1, "ARITY SHOULD BE MORE THAN 1 OR NOT SPECIFIED");
};

template <class FIELD, std::size_t MAX>
struct optional<
	FIELD,
	max<MAX>,
	void,
	min<0>,
	max<1>,
	std::enable_if_t<is_field<FIELD>::value>
> : multi_field_t<FIELD, 0, MAX>, optional_t
{
	static_assert(MAX > 1, "MAX SHOULD BE MORE THAN 1 OR NOT SPECIFIED");
};

//multi-instance field as a part of compound
template <class FIELD, std::size_t MAX, class COUNTER>
struct optional<
	FIELD,
	max<MAX>,
	COUNTER,
	min<0>,
	max<1>,
	std::enable_if_t<is_field<FIELD>::value && is_count_getter_v<COUNTER>>
> : multi_field_t<FIELD, 0, MAX>, optional_t
{
	using count_getter = COUNTER;
	static_assert(MAX > 1, "MAX SHOULD BE MORE THAN 1 OR NOT SPECIFIED");
};

template <class COUNTER, class FIELD, std::size_t MAX>
struct optional<
	COUNTER,
	FIELD,
	max<MAX>,
	min<0>,
	max<1>,
	std::enable_if_t<is_field<FIELD>::value && is_counter<COUNTER>::value>
> : multi_field_t<FIELD, 0, MAX>, COUNTER, optional_t
{
	static_assert(MAX > 1, "MAX SHOULD BE MORE THAN 1 OR NOT SPECIFIED");
};

template <class FIELD, std::size_t MAX, class IS_SET>
struct optional<
	FIELD,
	max<MAX>,
	IS_SET,
	min<0>,
	max<1>,
	std::enable_if_t<is_field<FIELD>::value && is_set_function_v<IS_SET>>
> : multi_field_t<FIELD, 0, MAX>, optional_t
{
	using optional_type = IS_SET;
	static_assert(MAX > 1, "MAX SHOULD BE MORE THAN 1 OR NOT SPECIFIED");
};

template <class FIELD, std::size_t MIN, std::size_t MAX>
struct optional<
	FIELD,
	min<MIN>,
	max<MAX>,
	min<0>,
	max<1>,
	std::enable_if_t<is_tagged_field<FIELD>::value>
> : multi_field_t<FIELD, MIN, MAX>, optional_t
{
	static_assert(MIN > 0, "MIN SHOULD BE MORE THAN 0 OR NOT SPECIFIED");
	static_assert(MAX > MIN, "MAX SHOULD BE MORE THAN MIN OR NOT SPECIFIED");
};

//multi-instance field as a part of compound
template <class FIELD, std::size_t MIN, std::size_t MAX, class COUNTER>
struct optional<
	FIELD,
	min<MIN>,
	max<MAX>,
	COUNTER,
	max<1>,
	std::enable_if_t<is_field<FIELD>::value && is_count_getter_v<COUNTER>>
> : multi_field_t<FIELD, MIN, MAX>, optional_t
{
	using count_getter = COUNTER;
	static_assert(MIN > 0, "MIN SHOULD BE MORE THAN 0 OR NOT SPECIFIED");
	static_assert(MAX > MIN, "MAX SHOULD BE MORE THAN MIN OR NOT SPECIFIED");
};

template <class COUNTER, class FIELD, std::size_t MIN, std::size_t MAX>
struct optional<
	COUNTER,
	FIELD,
	min<MIN>,
	max<MAX>,
	max<1>,
	std::enable_if_t<is_field<FIELD>::value && is_counter<COUNTER>::value>
> : multi_field_t<FIELD, MIN, MAX>, COUNTER, optional_t
{
	static_assert(MIN > 0, "MIN SHOULD BE MORE THAN 0 OR NOT SPECIFIED");
	static_assert(MAX > MIN, "MAX SHOULD BE MORE THAN MIN OR NOT SPECIFIED");
};

template <class TAG, class FIELD>
struct optional<
	TAG,
	FIELD,
	void,
	min<0>,
	max<1>,
	std::enable_if_t<is_tag<TAG>::value && is_field<FIELD>::value>
> : tag_value_t<TAG, FIELD>, optional_t
{
};

template <class TAG, class FIELD, std::size_t MAX>
struct optional<
	TAG,
	FIELD,
	max<MAX>,
	min<0>,
	max<1>,
	std::enable_if_t<is_tag<TAG>::value && is_field<FIELD>::value>
> : multi_tag_value_t<TAG, FIELD, 0, MAX>, optional_t
{
	static_assert(MAX > 1, "MAX SHOULD BE MORE THAN 1 OR NOT SPECIFIED");
};

template <class TAG, class FIELD, std::size_t NUM>
struct optional<
	TAG,
	FIELD,
	arity<NUM>,
	min<0>,
	max<1>,
	std::enable_if_t<is_tag<TAG>::value && is_field<FIELD>::value>
> : multi_tag_value_t<TAG, FIELD, NUM, NUM>, optional_t
{
	static_assert(NUM > 1, "ARITY SHOULD BE MORE THAN 1 OR NOT SPECIFIED");
};

template <class TAG, class LEN, class FIELD>
struct optional<
	TAG,
	LEN,
	FIELD,
	min<0>,
	max<1>,
	std::enable_if_t<is_tag<TAG>::value && is_length<LEN>::value && is_field<FIELD>::value>
> : tag_value_t<TAG, length_value_t<LEN, FIELD>>, optional_t
{
};

template <class TAG, class LEN, class FIELD, std::size_t NUM>
struct optional<
	TAG,
	LEN,
	FIELD,
	arity<NUM>,
	max<1>,
	std::enable_if_t<is_tag<TAG>::value && is_length<LEN>::value && is_field<FIELD>::value>
> : multi_tag_value_t<TAG, length_value_t<LEN, FIELD>, NUM, NUM>, optional_t
{
	static_assert(NUM > 1, "ARITY SHOULD BE MORE THAN 1 OR NOT SPECIFIED");
};

template <class TAG, class LEN, class FIELD, std::size_t MAX>
struct optional<
	TAG,
	LEN,
	FIELD,
	max<MAX>,
	max<1>,
	std::enable_if_t<is_tag<TAG>::value && is_length<LEN>::value && is_field<FIELD>::value>
> : multi_tag_value_t<TAG, length_value_t<LEN, FIELD>, 0, MAX>, optional_t
{
	static_assert(MAX > 1, "MAX SHOULD BE MORE THAN 1 OR NOT SPECIFIED");
};

template <class TAG, class COUNTER, class FIELD, std::size_t MAX>
struct optional<
	TAG,
	COUNTER,
	FIELD,
	max<MAX>,
	max<1>,
	std::enable_if_t<is_tag<TAG>::value && is_counter<COUNTER>::value && is_field<FIELD>::value>
> : multi_tag_value_t<TAG, FIELD, 0, MAX>, COUNTER, optional_t
{
	static_assert(MAX > 1, "MAX SHOULD BE MORE THAN 1 OR NOT SPECIFIED");
};

template <class LEN, class FIELD, std::size_t MAX>
struct optional<
	LEN,
	FIELD,
	max<MAX>,
	min<0>,
	max<1>,
	std::enable_if_t<is_length<LEN>::value && is_field<FIELD>::value>
> : multi_length_value_t<LEN, FIELD, 0, MAX>, optional_t
{
	static_assert(MAX > 1, "MAX SHOULD BE MORE THAN 1 OR NOT SPECIFIED");
};

template <class LEN, class FIELD,  std::size_t MIN, std::size_t MAX>
struct optional<
	LEN,
	FIELD,
	min<MIN>,
	max<MAX>,
	max<1>,
	std::enable_if_t<is_length<LEN>::value && is_field<FIELD>::value>
> : multi_length_value_t<LEN, FIELD, MIN, MAX>, optional_t
{
	static_assert(MIN > 0, "MIN SHOULD BE MORE THAN 0 OR NOT SPECIFIED");
	static_assert(MAX > MIN, "MAX SHOULD BE MORE THAN MIN OR NOT SPECIFIED");
};

template <class TAG, class LEN, class FIELD, std::size_t MIN, std::size_t MAX>
struct optional<
	TAG,
	LEN,
	FIELD,
	min<MIN>,
	max<MAX>,
	std::enable_if_t<is_tag<TAG>::value && is_length<LEN>::value && is_field<FIELD>::value>
> : multi_tag_value_t<TAG, length_value_t<LEN, FIELD>, MIN, MAX>, optional_t
{
	static_assert(MIN > 0, "MIN SHOULD BE MORE THAN 0 OR NOT SPECIFIED");
	static_assert(MAX > MIN, "MAX SHOULD BE MORE THAN MIN OR NOT SPECIFIED");
};

template <class TAG, class COUNTER, class FIELD, std::size_t MIN, std::size_t MAX>
struct optional<
	TAG,
	COUNTER,
	FIELD,
	min<MIN>,
	max<MAX>,
	std::enable_if_t<is_tag<TAG>::value && is_counter<COUNTER>::value && is_field<FIELD>::value>
> : multi_tag_value_t<TAG, FIELD, MIN, MAX>, COUNTER, optional_t
{
	static_assert(MIN > 0, "MIN SHOULD BE MORE THAN 0 OR NOT SPECIFIED");
	static_assert(MAX > MIN, "MAX SHOULD BE MORE THAN MIN OR NOT SPECIFIED");
};

} //namespace med
