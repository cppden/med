/**
@file
set of templates to define optional fields within containers

@copyright Denis Priyomov 2016-2017
Distributed under the MIT License
(See accompanying file LICENSE or visit https://github.com/cppden/med)
*/

#pragma once

#include "multi_field.hpp"
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
	static_assert(std::is_void<TAG_FLD>(), "MALFORMED OPTIONAL");
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
	//NOTE: since FIELD may have meta_info already we have to override it directly
	// w/o inheritance to avoid ambiguity
	using meta_info = make_meta_info_t<FIELD>;
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
> : multi_field<FIELD, 1, max<MAX>>, optional_t
{
	using meta_info = make_meta_info_t<FIELD>;
	static_assert(MAX > 1, "MAX SHOULD BE MORE THAN 1 OR NOT SPECIFIED");
};

template <class FIELD, std::size_t MAX>
struct optional<
	FIELD,
	pmax<MAX>,
	void,
	min<1>,
	max<1>,
	std::enable_if_t<is_field_v<FIELD>>
> : multi_field<FIELD, 1, pmax<MAX>>, optional_t
{
	using meta_info = make_meta_info_t<FIELD>;
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
	using meta_info = make_meta_info_t<FIELD>;
	using condition = CONDITION;
};

//single-instance conditional field w/ setter
template <class FIELD, class SETTER, class CONDITION>
struct optional<
	FIELD,
	SETTER,
	CONDITION,
	min<1>,
	max<1>,
	std::enable_if_t<is_field_v<FIELD> && is_setter_v<FIELD, SETTER> && is_condition_v<CONDITION>>
> : field_t<FIELD>, optional_t
{
	using meta_info = make_meta_info_t<FIELD>;
	using setter_type = SETTER;
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
> : multi_field<FIELD, 1, max<MAX>>, optional_t
{
	using meta_info = make_meta_info_t<FIELD>;
	using condition = CONDITION;
	static_assert(MAX > 1, "MAX SHOULD BE MORE THAN 1 OR NOT SPECIFIED");
};

template <class FIELD, std::size_t MAX, class CONDITION>
struct optional<
	FIELD,
	pmax<MAX>,
	CONDITION,
	min<1>,
	max<1>,
	std::enable_if_t<is_field_v<FIELD> && is_condition_v<CONDITION>>
> : multi_field<FIELD, 1, pmax<MAX>>, optional_t
{
	using meta_info = make_meta_info_t<FIELD>;
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
> : multi_field<FIELD, 1, max<MAX>>, optional_t
{
	using meta_info = make_meta_info_t<FIELD>;
	using condition = CONDITION;
	static_assert(MAX > 1, "MAX SHOULD BE MORE THAN 1 OR NOT SPECIFIED");
};

template <class FIELD, class CONDITION, std::size_t MAX>
struct optional<
	FIELD,
	CONDITION,
	pmax<MAX>,
	min<1>,
	max<1>,
	std::enable_if_t<is_field_v<FIELD> && is_condition_v<CONDITION>>
> : multi_field<FIELD, 1, pmax<MAX>>, optional_t
{
	using meta_info = make_meta_info_t<FIELD>;
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
> : multi_field<FIELD, 1, max<MAX>>, COUNTER, optional_t
{
	using meta_info = make_meta_info_t<FIELD>;
	static_assert(MAX > 1, "MAX SHOULD BE MORE THAN 1 OR NOT SPECIFIED");
};

template <class COUNTER, class FIELD, std::size_t MAX>
struct optional<
	COUNTER,
	FIELD,
	pmax<MAX>,
	min<1>,
	max<1>,
	std::enable_if_t<is_field_v<FIELD> && is_counter_v<COUNTER>>
> : multi_field<FIELD, 1, pmax<MAX>>, COUNTER, optional_t
{
	using meta_info = make_meta_info_t<FIELD>;
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
> : multi_field<FIELD, 1, max<MAX>>, optional_t
{
	using meta_info = make_meta_info_t<FIELD>;
	using count_getter = COUNTER;
	static_assert(MAX > 1, "MAX SHOULD BE MORE THAN 1 OR NOT SPECIFIED");
};

template <class FIELD, std::size_t MAX, class COUNTER>
struct optional<
	FIELD,
	pmax<MAX>,
	COUNTER,
	min<1>,
	max<1>,
	std::enable_if_t<is_field_v<FIELD> && is_count_getter_v<COUNTER>>
> : multi_field<FIELD, 1, pmax<MAX>>, optional_t
{
	using meta_info = make_meta_info_t<FIELD>;
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
> : multi_field<FIELD, MIN, max<MAX>>, optional_t
{
	using meta_info = make_meta_info_t<FIELD>;
	using count_getter = COUNTER;
	static_assert(MIN > 1, "MIN SHOULD BE MORE THAN 1 OR NOT SPECIFIED");
	static_assert(MAX > MIN, "MAX SHOULD BE MORE THAN MIN OR NOT SPECIFIED");
};

template <class FIELD, std::size_t MIN, std::size_t MAX, class COUNTER>
struct optional<
	FIELD,
	min<MIN>,
	pmax<MAX>,
	COUNTER,
	max<1>,
	std::enable_if_t<is_field_v<FIELD> && is_count_getter_v<COUNTER>>
> : multi_field<FIELD, MIN, pmax<MAX>>, optional_t
{
	using meta_info = make_meta_info_t<FIELD>;
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
> : multi_field<FIELD, MIN, max<MAX>>, COUNTER, optional_t
{
	using meta_info = make_meta_info_t<FIELD>;
	static_assert(MIN > 1, "MIN SHOULD BE MORE THAN 1 OR NOT SPECIFIED");
	static_assert(MAX > MIN, "MAX SHOULD BE MORE THAN MIN OR NOT SPECIFIED");
};

template <class COUNTER, class FIELD, std::size_t MIN, std::size_t MAX>
struct optional<
	COUNTER,
	FIELD,
	min<MIN>,
	pmax<MAX>,
	max<1>,
	std::enable_if_t<is_field_v<FIELD> && is_counter_v<COUNTER>>
> : multi_field<FIELD, MIN, pmax<MAX>>, COUNTER, optional_t
{
	using meta_info = make_meta_info_t<FIELD>;
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
	using meta_info = make_meta_info_t<mi<mik::TAG, tag_t<TAG>>, FIELD>;
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
> : multi_tag_value<TAG, FIELD, 1, max<MAX>>, optional_t
{
	using meta_info = make_meta_info_t<mi<mik::TAG, tag_t<TAG>>, FIELD>;
	static_assert(MAX > 1, "MAX SHOULD BE MORE THAN 1 OR NOT SPECIFIED");
};

template <class TAG, class FIELD, std::size_t MAX>
struct optional<
	TAG,
	FIELD,
	pmax<MAX>,
	min<1>,
	max<1>,
	std::enable_if_t<is_tag_v<TAG> && is_field_v<FIELD>>
> : multi_tag_value<TAG, FIELD, 1, pmax<MAX>>, optional_t
{
	using meta_info = make_meta_info_t<mi<mik::TAG, tag_t<TAG>>, FIELD>;
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
> : multi_tag_value<TAG, FIELD, NUM, max<NUM>>, optional_t
{
	using meta_info = make_meta_info_t<mi<mik::TAG, tag_t<TAG>>, FIELD>;
	static_assert(NUM > 1, "ARITY SHOULD BE MORE THAN 1 OR NOT SPECIFIED");
};

//multi-instance field
template <class FIELD, std::size_t MIN, std::size_t MAX>
struct optional<
	FIELD,
	min<MIN>,
	max<MAX>,
	min<1>,
	max<1>,
	std::enable_if_t<is_field_v<FIELD>>
> : multi_field<FIELD, MIN, max<MAX>>, optional_t
{
	using meta_info = make_meta_info_t<FIELD>;
	static_assert(MIN > 1, "MIN SHOULD BE MORE THAN 1 OR NOT SPECIFIED");
	static_assert(MAX > MIN, "MAX SHOULD BE MORE THAN MIN OR NOT SPECIFIED");
};

template <class FIELD, std::size_t MIN, std::size_t MAX>
struct optional<
	FIELD,
	min<MIN>,
	pmax<MAX>,
	min<1>,
	max<1>,
	std::enable_if_t<is_field_v<FIELD>>
> : multi_field<FIELD, MIN, pmax<MAX>>, optional_t
{
	using meta_info = make_meta_info_t<FIELD>;
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
	std::enable_if_t<is_field_v<FIELD>>
> : multi_field<FIELD, NUM, max<NUM>>, optional_t
{
	using meta_info = make_meta_info_t<FIELD>;
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
	using meta_info = make_meta_info_t<mi<mik::TAG, tag_t<TAG>>, mi<mik::LEN, LEN>, FIELD>;
};

template <class TAG, class LEN, class FIELD, std::size_t NUM>
struct optional<
	TAG,
	LEN,
	FIELD,
	arity<NUM>,
	max<1>,
	std::enable_if_t<is_tag_v<TAG> && is_length_v<LEN> && is_field_v<FIELD>>
> : multi_tag_value<TAG, length_value_t<LEN, FIELD>, NUM, max<NUM>>, optional_t
{
	using meta_info = make_meta_info_t<mi<mik::TAG, tag_t<TAG>>, mi<mik::LEN, LEN>, FIELD>;
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
> : multi_tag_value<TAG, length_value_t<LEN, FIELD>, 1, max<MAX>>, optional_t
{
	using meta_info = make_meta_info_t<mi<mik::TAG, tag_t<TAG>>, mi<mik::LEN, LEN>, FIELD>;
	static_assert(MAX > 1, "MAX SHOULD BE MORE THAN 1 OR NOT SPECIFIED");
};

template <class TAG, class LEN, class FIELD, std::size_t MAX>
struct optional<
	TAG,
	LEN,
	FIELD,
	pmax<MAX>,
	max<1>,
	std::enable_if_t<is_tag_v<TAG> && is_length_v<LEN> && is_field_v<FIELD>>
> : multi_tag_value<TAG, length_value_t<LEN, FIELD>, 1, pmax<MAX>>, optional_t
{
	using meta_info = make_meta_info_t<mi<mik::TAG, tag_t<TAG>>, mi<mik::LEN, LEN>, FIELD>;
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
> : multi_tag_value<TAG, length_value_t<LEN, FIELD>, MIN, max<MAX>>, optional_t
{
	using meta_info = make_meta_info_t<mi<mik::TAG, tag_t<TAG>>, mi<mik::LEN, LEN>, FIELD>;
	static_assert(MIN > 1, "MIN SHOULD BE MORE THAN 1 OR NOT SPECIFIED");
	static_assert(MAX > MIN, "MAX SHOULD BE MORE THAN MIN OR NOT SPECIFIED");
};

template <class TAG, class LEN, class FIELD, std::size_t MIN, std::size_t MAX>
struct optional<
	TAG,
	LEN,
	FIELD,
	min<MIN>,
	pmax<MAX>,
	std::enable_if_t<is_tag_v<TAG> && is_length_v<LEN> && is_field_v<FIELD>>
> : multi_tag_value<TAG, length_value_t<LEN, FIELD>, MIN, pmax<MAX>>, optional_t
{
	using meta_info = make_meta_info_t<mi<mik::TAG, tag_t<TAG>>, mi<mik::LEN, LEN>, FIELD>;
	static_assert(MIN > 1, "MIN SHOULD BE MORE THAN 1 OR NOT SPECIFIED");
	static_assert(MAX > MIN, "MAX SHOULD BE MORE THAN MIN OR NOT SPECIFIED");
};
#endif ///--->>> TLV


#if 1 ///<<<--- LV
template <class LEN, class FIELD>
struct optional<
	LEN,
	FIELD,
	void,
	min<1>,
	max<1>,
	std::enable_if_t<is_field_v<FIELD> && is_length_v<LEN>>
> : length_value_t<LEN, FIELD>, optional_t
{
	using meta_info = make_meta_info_t<mi<mik::LEN, LEN>, FIELD>;
};

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
	using meta_info = make_meta_info_t<mi<mik::LEN, LEN>, FIELD>;
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
> : multi_length_value<LEN, FIELD, 1, max<MAX>>, optional_t
{
	using meta_info = make_meta_info_t<mi<mik::LEN, LEN>, FIELD>;
	static_assert(MAX > 1, "MAX SHOULD BE MORE THAN 1 OR NOT SPECIFIED");
};

template <class LEN, class FIELD, std::size_t MAX>
struct optional<
	LEN,
	FIELD,
	pmax<MAX>,
	min<1>,
	max<1>,
	std::enable_if_t<is_length_v<LEN> && is_field_v<FIELD>>
> : multi_length_value<LEN, FIELD, 1, pmax<MAX>>, optional_t
{
	using meta_info = make_meta_info_t<mi<mik::LEN, LEN>, FIELD>;
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
> : multi_length_value<LEN, FIELD, MIN, max<MAX>>, optional_t
{
	using meta_info = make_meta_info_t<mi<mik::LEN, LEN>, FIELD>;
	static_assert(MIN > 1, "MIN SHOULD BE MORE THAN 1 OR NOT SPECIFIED");
	static_assert(MAX > MIN, "MAX SHOULD BE MORE THAN MIN OR NOT SPECIFIED");
};

template <class LEN, class FIELD,  std::size_t MIN, std::size_t MAX>
struct optional<
	LEN,
	FIELD,
	min<MIN>,
	pmax<MAX>,
	max<1>,
	std::enable_if_t<is_length_v<LEN> && is_field_v<FIELD>>
> : multi_length_value<LEN, FIELD, MIN, pmax<MAX>>, optional_t
{
	using meta_info = make_meta_info_t<mi<mik::LEN, LEN>, FIELD>;
	static_assert(MIN > 1, "MIN SHOULD BE MORE THAN 1 OR NOT SPECIFIED");
	static_assert(MAX > MIN, "MAX SHOULD BE MORE THAN MIN OR NOT SPECIFIED");
};
#endif ///--->>> LV

} //namespace med
