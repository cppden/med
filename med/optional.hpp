/**
@file
set of templates to define optional fields within containers

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
	class MIN_MAX = min<1>,
	class MAX_MIN = max<1>,
	typename Enable = void >
struct optional
{
	static_assert(std::is_same<TAG_FLD, void>(), "MALFORMED OPTIONAL");
};

//single-instance optional field by end of data
template <class FIELD>
struct optional<
	FIELD,
	void,
	void,
	min<1>,
	max<1>,
	std::enable_if_t<is_field_v<FIELD>>
> : field_t<FIELD>, optional_t
{
};

//multi-instance optional field by end of data
template <class FIELD, std::size_t MAX>
struct optional<
	FIELD,
	max<MAX>,
	void,
	min<1>,
	max<1>,
	std::enable_if_t<is_field_v<FIELD>>
> : multi_field_t<FIELD, 1, MAX>, optional_t
{
	static_assert(MAX > 1, "MAX SHOULD BE MORE THAN 1 OR NOT SPECIFIED");
};

#if 1 ///<<<--- Cond-V

//single-instance conditional field
template <class FIELD, class CONDITION>
struct optional<
	FIELD,
	CONDITION,
	void,
	min<1>,
	max<1>,
	std::enable_if_t<is_field_v<FIELD> && is_condition_v<CONDITION>>
> : field_t<FIELD>, optional_t
{
	using condition = CONDITION;
};

//multi-instance conditional field
template <class FIELD, std::size_t MAX, class CONDITION>
struct optional<
	FIELD,
	max<MAX>,
	CONDITION,
	min<1>,
	max<1>,
	std::enable_if_t<is_field_v<FIELD> && is_condition_v<CONDITION>>
> : multi_field_t<FIELD, 1, MAX>, optional_t
{
	using condition = CONDITION;
	static_assert(MAX > 1, "MAX SHOULD BE MORE THAN 1 OR NOT SPECIFIED");
};

//multi-instance conditional field
template <class FIELD, class CONDITION, std::size_t MAX>
struct optional<
	FIELD,
	CONDITION,
	max<MAX>,
	min<1>,
	max<1>,
	std::enable_if_t<is_field_v<FIELD> && is_condition_v<CONDITION>>
> : multi_field_t<FIELD, 1, MAX>, optional_t
{
	using condition = CONDITION;
	static_assert(MAX > 1, "MAX SHOULD BE MORE THAN 1 OR NOT SPECIFIED");
};

#endif ///>>>--- Cond-V

#if 1 ///<<<--- CV
//multi-instance field w/ counter
template <class COUNTER, class FIELD, std::size_t MAX>
struct optional<
	COUNTER,
	FIELD,
	max<MAX>,
	min<1>,
	max<1>,
	std::enable_if_t<is_field_v<FIELD> && is_counter_v<COUNTER>>
> : multi_field_t<FIELD, 1, MAX>, COUNTER, optional_t
{
	static_assert(MAX > 1, "MAX SHOULD BE MORE THAN 1 OR NOT SPECIFIED");
};

//multi-instance field w/ count-getter
template <class FIELD, std::size_t MAX, class COUNTER>
struct optional<
	FIELD,
	max<MAX>,
	COUNTER,
	min<1>,
	max<1>,
	std::enable_if_t<is_field_v<FIELD> && is_count_getter_v<COUNTER>>
> : multi_field_t<FIELD, 1, MAX>, optional_t
{
	using count_getter = COUNTER;
	static_assert(MAX > 1, "MAX SHOULD BE MORE THAN 1 OR NOT SPECIFIED");
};

//multi-instance field w/ count-getter
template <class FIELD, std::size_t MIN, std::size_t MAX, class COUNTER>
struct optional<
	FIELD,
	min<MIN>,
	max<MAX>,
	COUNTER,
	max<1>,
	std::enable_if_t<is_field_v<FIELD> && is_count_getter_v<COUNTER>>
> : multi_field_t<FIELD, MIN, MAX>, optional_t
{
	using count_getter = COUNTER;
	static_assert(MIN > 1, "MIN SHOULD BE MORE THAN 1 OR NOT SPECIFIED");
	static_assert(MAX > MIN, "MAX SHOULD BE MORE THAN MIN OR NOT SPECIFIED");
};

//multi-instance field w/ counter
template <class COUNTER, class FIELD, std::size_t MIN, std::size_t MAX>
struct optional<
	COUNTER,
	FIELD,
	min<MIN>,
	max<MAX>,
	max<1>,
	std::enable_if_t<is_field_v<FIELD> && is_counter_v<COUNTER>>
> : multi_field_t<FIELD, MIN, MAX>, COUNTER, optional_t
{
	static_assert(MIN > 1, "MIN SHOULD BE MORE THAN 1 OR NOT SPECIFIED");
	static_assert(MAX > MIN, "MAX SHOULD BE MORE THAN MIN OR NOT SPECIFIED");
};
#endif ///>>>--- CV

#if 1 ///<<<--- TV
//single-instance field w/ tag
template <class TAG, class FIELD>
struct optional<
	TAG,
	FIELD,
	void,
	min<1>,
	max<1>,
	std::enable_if_t<is_tag_v<TAG> && is_field_v<FIELD>>
> : tag_value_t<TAG, FIELD>, optional_t
{
};

//multi-instance field w/ tag
template <class TAG, class FIELD, std::size_t MAX>
struct optional<
	TAG,
	FIELD,
	max<MAX>,
	min<1>,
	max<1>,
	std::enable_if_t<is_tag_v<TAG> && is_field_v<FIELD>>
> : multi_tag_value_t<TAG, FIELD, 1, MAX>, optional_t
{
	static_assert(MAX > 1, "MAX SHOULD BE MORE THAN 1 OR NOT SPECIFIED");
};

//multi-instance field w/ tag
template <class TAG, class FIELD, std::size_t NUM>
struct optional<
	TAG,
	FIELD,
	arity<NUM>,
	min<1>,
	max<1>,
	std::enable_if_t<is_tag_v<TAG> && is_field_v<FIELD>>
> : multi_tag_value_t<TAG, FIELD, NUM, NUM>, optional_t
{
	static_assert(NUM > 1, "ARITY SHOULD BE MORE THAN 1 OR NOT SPECIFIED");
};

//multi-instance field w/ tag-type
template <class FIELD, std::size_t MIN, std::size_t MAX>
struct optional<
	FIELD,
	min<MIN>,
	max<MAX>,
	min<1>,
	max<1>,
	std::enable_if_t<is_tagged_field_v<FIELD>>
> : multi_field_t<FIELD, MIN, MAX>, optional_t
{
	static_assert(MIN > 1, "MIN SHOULD BE MORE THAN 1 OR NOT SPECIFIED");
	static_assert(MAX > MIN, "MAX SHOULD BE MORE THAN MIN OR NOT SPECIFIED");
};

//multi-instance field w/ tag-type
template <class FIELD, std::size_t NUM>
struct optional<
	FIELD,
	arity<NUM>,
	void,
	min<1>,
	max<1>,
	std::enable_if_t<is_tagged_field_v<FIELD>>
> : multi_field_t<FIELD, NUM, NUM>, optional_t
{
	static_assert(NUM > 1, "ARITY SHOULD BE MORE THAN 1 OR NOT SPECIFIED");
};

#endif ///>>>--- TV

#if 1 ///<<<--- TLV
template <class TAG, class LEN, class FIELD>
struct optional<
	TAG,
	LEN,
	FIELD,
	min<1>,
	max<1>,
	std::enable_if_t<is_tag_v<TAG> && is_length_v<LEN> && is_field_v<FIELD>>
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
	std::enable_if_t<is_tag_v<TAG> && is_length_v<LEN> && is_field_v<FIELD>>
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
	std::enable_if_t<is_tag_v<TAG> && is_length_v<LEN> && is_field_v<FIELD>>
> : multi_tag_value_t<TAG, length_value_t<LEN, FIELD>, 1, MAX>, optional_t
{
	static_assert(MAX > 1, "MAX SHOULD BE MORE THAN 1 OR NOT SPECIFIED");
};

template <class TAG, class LEN, class FIELD, std::size_t MIN, std::size_t MAX>
struct optional<
	TAG,
	LEN,
	FIELD,
	min<MIN>,
	max<MAX>,
	std::enable_if_t<is_tag_v<TAG> && is_length_v<LEN> && is_field_v<FIELD>>
> : multi_tag_value_t<TAG, length_value_t<LEN, FIELD>, MIN, MAX>, optional_t
{
	static_assert(MIN > 1, "MIN SHOULD BE MORE THAN 1 OR NOT SPECIFIED");
	static_assert(MAX > MIN, "MAX SHOULD BE MORE THAN MIN OR NOT SPECIFIED");
};
#endif ///--->>> TLV


#if 1 ///<<<--- LV
//optional field as a part of compound
template <class LEN, class FIELD, class CONDITION>
struct optional<
	LEN,
	FIELD,
	CONDITION,
	min<1>,
	max<1>,
	std::enable_if_t<is_field_v<FIELD> && is_length_v<LEN> && is_condition_v<CONDITION>>
> : length_value_t<LEN, FIELD>, optional_t
{
	using condition = CONDITION;
};

template <class LEN, class FIELD, std::size_t MAX>
struct optional<
	LEN,
	FIELD,
	max<MAX>,
	min<1>,
	max<1>,
	std::enable_if_t<is_length_v<LEN> && is_field_v<FIELD>>
> : multi_length_value_t<LEN, FIELD, 1, MAX>, optional_t
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
	std::enable_if_t<is_length_v<LEN> && is_field_v<FIELD>>
> : multi_length_value_t<LEN, FIELD, MIN, MAX>, optional_t
{
	static_assert(MIN > 1, "MIN SHOULD BE MORE THAN 1 OR NOT SPECIFIED");
	static_assert(MAX > MIN, "MAX SHOULD BE MORE THAN MIN OR NOT SPECIFIED");
};
#endif ///--->>> LV

} //namespace med
