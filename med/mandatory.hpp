/**
@file
set of templates to define mandatory fields within containers

@copyright Denis Priyomov 2016-2017
Distributed under the MIT License
(See accompanying file LICENSE or visit https://github.com/cppden/med)
*/

#pragma once

#include "field.hpp"
#include "count.hpp"
#include "tag.hpp"
#include "length.hpp"

namespace med {

template<
	class TAG_FLD_LEN,
	class FLD_LEN_NUM = void,
	class FLD_NUM     = void,
	class MIN_MAX     = min<1>,
	class MAX_MIN     = max<1>
>
struct mandatory
{
	static_assert(std::is_void<TAG_FLD_LEN>(), "MALFORMED MANDATORY");
};

//M<FIELD>
template <AField FIELD>
struct mandatory<
	FIELD,
	void,
	void,
	min<1>,
	max<1>
> : field_t<FIELD>
{
	using field_type = FIELD;
};

//M<FIELD, SETTER>
template <AField FIELD, class SETTER> requires ASetter<FIELD, SETTER>
struct mandatory<
	FIELD,
	SETTER,
	void,
	min<1>,
	max<1>
> : field_t<FIELD>
{
	using setter_type = SETTER;
};


//M<FIELD, arity<NUM>>
template <AField FIELD, std::size_t NUM>
struct mandatory<
	FIELD,
	arity<NUM>,
	void,
	min<1>,
	max<1>
> : multi_field<FIELD, NUM, max<NUM>>
{
	static_assert(NUM > 1, "ARITY SHOULD BE MORE THAN 1 OR NOT SPECIFIED");
};

//M<FIELD, min<MIN>, max<MAX>, CNT_GETTER>
template <AField FIELD, std::size_t MIN, std::size_t MAX, ACountGetter COUNTER>
struct mandatory<
	FIELD,
	min<MIN>,
	max<MAX>,
	COUNTER,
	max<1>
> : multi_field<FIELD, MIN, max<MAX>>
{
	using count_getter = COUNTER;
	static_assert(MIN > 1, "MIN SHOULD BE MORE THAN 1 OR NOT SPECIFIED");
	static_assert(MAX > MIN, "MAX SHOULD BE MORE THAN MIN");
};

//M<FIELD, min<MIN>, Pmax<MAX>, CNT_GETTER>
template <AField FIELD, std::size_t MIN, std::size_t MAX, ACountGetter COUNTER>
struct mandatory<
	FIELD,
	min<MIN>,
	pmax<MAX>,
	COUNTER,
	max<1>
> : multi_field<FIELD, MIN, pmax<MAX>>
{
	using count_getter = COUNTER;
	static_assert(MIN > 1, "MIN SHOULD BE MORE THAN 1 OR NOT SPECIFIED");
	static_assert(MAX > MIN, "MAX SHOULD BE MORE THAN MIN");
};

//M<CNT, FIELD, min<MIN>, max<MAX>>
template <ACounter COUNTER, AField FIELD, std::size_t MIN, std::size_t MAX>
struct mandatory<
	COUNTER,
	FIELD,
	min<MIN>,
	max<MAX>,
	max<1>
> : multi_field<FIELD, MIN, max<MAX>>, COUNTER
{
	static_assert(MIN > 1, "MIN SHOULD BE MORE THAN 1 OR NOT SPECIFIED");
	static_assert(MAX > MIN, "MAX SHOULD BE MORE THAN MIN");
};

//M<CNT, FIELD, min<MIN>, Pmax<MAX>>
template <ACounter COUNTER, AField FIELD, std::size_t MIN, std::size_t MAX>
struct mandatory<
	COUNTER,
	FIELD,
	min<MIN>,
	pmax<MAX>,
	max<1>
> : multi_field<FIELD, MIN, pmax<MAX>>, COUNTER
{
	static_assert(MIN > 1, "MIN SHOULD BE MORE THAN 1 OR NOT SPECIFIED");
	static_assert(MAX > MIN, "MAX SHOULD BE MORE THAN MIN");
};

//M<FIELD, min<MIN>, max<MAX>>
template <AField FIELD, std::size_t MIN, std::size_t MAX>
struct mandatory<
	FIELD,
	min<MIN>,
	max<MAX>,
	min<1>,
	max<1>
> : multi_field<FIELD, MIN, max<MAX>>
{
	static_assert(MIN > 1, "MIN SHOULD BE MORE THAN 1 OR NOT SPECIFIED");
	static_assert(MAX > MIN, "MAX SHOULD BE MORE THAN MIN");
};

//M<FIELD, min<MIN>, Pmax<MAX>>
template <AField FIELD, std::size_t MIN, std::size_t MAX>
struct mandatory<
	FIELD,
	min<MIN>,
	pmax<MAX>,
	min<1>,
	max<1>
> : multi_field<FIELD, MIN, pmax<MAX>>
{
	static_assert(MIN > 1, "MIN SHOULD BE MORE THAN 1 OR NOT SPECIFIED");
	static_assert(MAX > MIN, "MAX SHOULD BE MORE THAN MIN");
};

//M<FIELD, max<MAX>>
template <AField FIELD, std::size_t MAX>
struct mandatory<
	FIELD,
	max<MAX>,
	void,
	min<1>,
	max<1>
> : multi_field<FIELD, 1, max<MAX>>
{
	static_assert(MAX > 1, "MAX SHOULD BE MORE THAN 1 OR NOT SPECIFIED");
};

//M<FIELD, Pmax<MAX>>
template <AField FIELD, std::size_t MAX>
struct mandatory<
	FIELD,
	pmax<MAX>,
	void,
	min<1>,
	max<1>
> : multi_field<FIELD, 1, pmax<MAX>>
{
	static_assert(MAX > 1, "MAX SHOULD BE MORE THAN 1 OR NOT SPECIFIED");
};

//M<FIELD, max<MAX>, COUNTER>
template <AField FIELD, std::size_t MAX, ACountGetter COUNTER>
struct mandatory<
	FIELD,
	max<MAX>,
	COUNTER,
	min<1>,
	max<1>
> : multi_field<FIELD, 1, max<MAX>>
{
	using count_getter = COUNTER;
	static_assert(MAX > 1, "MAX SHOULD BE MORE THAN 1 OR NOT SPECIFIED");
};

//M<FIELD, Pmax<MAX>, COUNTER>
template <AField FIELD, std::size_t MAX, ACountGetter COUNTER>
struct mandatory<
	FIELD,
	pmax<MAX>,
	COUNTER,
	min<1>,
	max<1>
> : multi_field<FIELD, 1, pmax<MAX>>
{
	using count_getter = COUNTER;
	static_assert(MAX > 1, "MAX SHOULD BE MORE THAN 1 OR NOT SPECIFIED");
};

//M<CNT, FIELD, max<MAX>>
template <ACounter COUNTER, AField FIELD, std::size_t MAX>
struct mandatory<
	COUNTER,
	FIELD,
	max<MAX>,
	min<1>,
	max<1>
> : multi_field<FIELD, 1, max<MAX>>, COUNTER
{
	static_assert(MAX > 1, "MAX SHOULD BE MORE THAN 1 OR NOT SPECIFIED");
};

//M<CNT, FIELD, Pmax<MAX>>
template <ACounter COUNTER, AField FIELD, std::size_t MAX>
struct mandatory<
	COUNTER,
	FIELD,
	pmax<MAX>,
	min<1>,
	max<1>
> : multi_field<FIELD, 1, pmax<MAX>>, COUNTER
{
	static_assert(MAX > 1, "MAX SHOULD BE MORE THAN 1 OR NOT SPECIFIED");
};

//M<TAG, FIELD>
template <ATag TAG, AField FIELD>
struct mandatory<
	TAG,
	FIELD,
	void,
	min<1>,
	max<1>
> : field_t<FIELD, add_tag<TAG>>
{
};

//M<TAG, FIELD, arity<NUM>
template <ATag TAG, AField FIELD, std::size_t NUM>
struct mandatory<
	TAG,
	FIELD,
	arity<NUM>,
	min<1>,
	max<1>
> : multi_field<FIELD, NUM, max<NUM>, void, add_tag<TAG>>
{
	static_assert(NUM > 1, "ARITY SHOULD BE MORE THAN 1 OR NOT SPECIFIED");
};

//M<TAG, FIELD, max<MAX>>
template <ATag TAG, AField FIELD, std::size_t MAX>
struct mandatory<
	TAG,
	FIELD,
	max<MAX>,
	min<1>,
	max<1>
> : multi_field<FIELD, 1, max<MAX>, void, add_tag<TAG>>
{
	static_assert(MAX > 1, "MAX SHOULD BE MORE THAN 1 OR NOT SPECIFIED");
};

//M<TAG, FIELD, Pmax<MAX>>
template <ATag TAG, AField FIELD, std::size_t MAX>
struct mandatory<
	TAG,
	FIELD,
	pmax<MAX>,
	min<1>,
	max<1>
> : multi_field<FIELD, 1, pmax<MAX>, void, add_tag<TAG>>
{
	static_assert(MAX > 1, "MAX SHOULD BE MORE THAN 1 OR NOT SPECIFIED");
};

//M<TAG, FIELD, min<MIN>, max<MAX>>
template <ATag TAG, AField FIELD, std::size_t MIN, std::size_t MAX>
struct mandatory<
	TAG,
	FIELD,
	min<MIN>,
	max<MAX>,
	max<1>
> : multi_field<FIELD, MIN, max<MAX>, void, add_tag<TAG>>
{
	static_assert(MIN > 1, "MIN SHOULD BE MORE THAN 1 OR NOT SPECIFIED");
	static_assert(MAX > MIN, "MAX SHOULD BE MORE THAN MIN");
};

//M<TAG, FIELD, min<MIN>, Pmax<MAX>>
template <ATag TAG, AField FIELD, std::size_t MIN, std::size_t MAX>
struct mandatory<
	TAG,
	FIELD,
	min<MIN>,
	pmax<MAX>,
	max<1>
> : multi_field<FIELD, MIN, pmax<MAX>, void, add_tag<TAG>>
{
	static_assert(MIN > 1, "MIN SHOULD BE MORE THAN 1 OR NOT SPECIFIED");
	static_assert(MAX > MIN, "MAX SHOULD BE MORE THAN MIN");
};

//M<LEN, FIELD>
template <ALength LEN, AField FIELD>
struct mandatory<
	LEN,
	FIELD,
	void,
	min<1>,
	max<1>
> : field_t<FIELD, add_len<typename LEN::length_type>>
{
	//bool operator==(mandatory const& rhs) const noexcept { return static_cast<FIELD const*>(this)->operator==(rhs); }
};

//M<LEN, FIELD, arity<NUM>>
template <ALength LEN, AField FIELD, std::size_t NUM>
struct mandatory<
	LEN,
	FIELD,
	arity<NUM>,
	min<1>,
	max<1>
> : multi_field<FIELD, NUM, max<NUM>, void, add_len<typename LEN::length_type>>
{
	static_assert(NUM > 1, "ARITY SHOULD BE MORE THAN 1 OR NOT SPECIFIED");
};

//M<TAG, LEN, FIELD>
template <ATag TAG, ALength LEN, AField FIELD>
struct mandatory<
	TAG,
	LEN,
	FIELD,
	min<1>,
	max<1>
> : field_t<FIELD, add_tag<TAG>, add_len<typename LEN::length_type>>
{
};

//M<TAG, LEN, FIELD, arity<NUM>>
template <ATag TAG, ALength LEN, AField FIELD, std::size_t NUM>
struct mandatory<
	TAG,
	LEN,
	FIELD,
	arity<NUM>,
	max<1>
> : multi_field<FIELD, NUM, max<NUM>, void, add_tag<TAG>, add_len<typename LEN::length_type>>
{
	static_assert(NUM > 1, "ARITY SHOULD BE MORE THAN 1 OR NOT SPECIFIED");
};

//M<LEN, FIELD, max<MAX>>
template <ALength LEN, AField FIELD, std::size_t MAX>
struct mandatory<
	LEN,
	FIELD,
	max<MAX>,
	min<1>,
	max<1>
> : multi_field<FIELD, 1, max<MAX>, void, add_len<typename LEN::length_type>>
{
	static_assert(MAX > 1, "MAX SHOULD BE MORE THAN 1 OR NOT SPECIFIED");
};

//M<LEN, FIELD, Pmax<MAX>>
template <ALength LEN, AField FIELD, std::size_t MAX>
struct mandatory<
	LEN,
	FIELD,
	pmax<MAX>,
	min<1>,
	max<1>
> : multi_field<FIELD, 1, pmax<MAX>, void, add_len<typename LEN::length_type>>
{
	static_assert(MAX > 1, "MAX SHOULD BE MORE THAN 1 OR NOT SPECIFIED");
};

//M<TAG, LEN, FIELD, max<MAX>>
template <ATag TAG, ALength LEN, AField FIELD, std::size_t MAX>
struct mandatory<
	TAG,
	LEN,
	FIELD,
	max<MAX>,
	max<1>
> : multi_field<FIELD, 1, max<MAX>, void, add_tag<TAG>, add_len<typename LEN::length_type>>
{
	static_assert(MAX > 1, "MAX SHOULD BE MORE THAN 1 OR NOT SPECIFIED");
};

//M<TAG, LEN, FIELD, Pmax<MAX>>
template <ATag TAG, ALength LEN, AField FIELD, std::size_t MAX>
struct mandatory<
	TAG,
	LEN,
	FIELD,
	pmax<MAX>,
	max<1>
> : multi_field<FIELD, 1, pmax<MAX>, void, add_tag<TAG>, add_len<typename LEN::length_type>>
{
	static_assert(MAX > 1, "MAX SHOULD BE MORE THAN 1 OR NOT SPECIFIED");
};

//M<TAG, LEN, FIELD, min<MIN>, max<MAX>>
template <ATag TAG, ALength LEN, AField FIELD, std::size_t MIN, std::size_t MAX>
struct mandatory<
	TAG,
	LEN,
	FIELD,
	min<MIN>,
	max<MAX>
> : multi_field<FIELD, MIN, max<MAX>, void, add_tag<TAG>, add_len<typename LEN::length_type>>
{
	static_assert(MIN > 1, "MIN SHOULD BE MORE THAN 1 OR NOT SPECIFIED");
	static_assert(MAX > MIN, "MAX SHOULD BE MORE THAN MIN");
};

//M<TAG, LEN, FIELD, min<MIN>, Pmax<MAX>>
template <ATag TAG, ALength LEN, AField FIELD, std::size_t MIN, std::size_t MAX>
struct mandatory<
	TAG,
	LEN,
	FIELD,
	min<MIN>,
	pmax<MAX>
> : multi_field<FIELD, MIN, pmax<MAX>, void, add_tag<TAG>, add_len<typename LEN::length_type>>
{
	static_assert(MIN > 1, "MIN SHOULD BE MORE THAN 1 OR NOT SPECIFIED");
	static_assert(MAX > MIN, "MAX SHOULD BE MORE THAN MIN");
};

} //namespace med
