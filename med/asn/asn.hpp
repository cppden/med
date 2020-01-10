#pragma once

/**
@file
ASN.1

@copyright Denis Priyomov 2018
Distributed under the MIT License
(See accompanying file LICENSE or visit https://github.com/cppden/med)
*/
//#include <cstdtypes>

#include "ids.hpp"
#include "../value.hpp"
#include "../octet_string.hpp"
#include "../bit_string.hpp"
#include "../sequence.hpp"
#include "../set.hpp"

namespace med::asn {

template <typename T, class... ASN_TRAITS> //TRAITs is asn::traits<T,C>
struct value_t : value<T>
{
	using meta_info = meta::typelist<mi<mik::TAG, ASN_TRAITS>...>;
};


template <class... ASN_TRAITS>
using boolean_t = value_t<bool, ASN_TRAITS...>;
using boolean = boolean_t<traits<tg_value::BOOLEAN>>;

template <class... ASN_TRAITS>
struct null_t : med::empty<>
{
	using meta_info = meta::typelist<mi<mik::TAG, ASN_TRAITS>...>;
};
using null = null_t<traits<tg_value::NULL_TYPE>>;

using integer = value_t<int, traits<tg_value::INTEGER>>;

template <class... ASN_TRAITS>
using enumerated_t = value_t<int, ASN_TRAITS...>;
using enumerated = enumerated_t<traits<tg_value::ENUMERATED>>;

template <class... ASN_TRAITS>
using real_t = value_t<double, ASN_TRAITS...>;
using real = real_t<traits<tg_value::REAL>>;

template <class... ASN_TRAITS>
struct bit_string_t : med::bit_string<>
{
	using meta_info = meta::typelist<mi<mik::TAG, ASN_TRAITS>...>;
};
using bit_string = bit_string_t<traits<tg_value::BIT_STRING>>;

template <class... ASN_TRAITS>
struct octet_string_t : med::octet_string<octets_var_extern>
{
	using meta_info = meta::typelist<mi<mik::TAG, ASN_TRAITS>...>;
};
using octet_string = octet_string_t<traits<tg_value::OCTET_STRING>>;

template <class META_INFO, class ...IES>
struct sequence_t : med::sequence<IES...>
{
	using meta_info = META_INFO;
};
template <class ...IES>
using sequence = sequence_t<
	meta::typelist<mi<mik::TAG, traits<tg_value::SEQUENCE>>>,
	IES...
>;

template <class META_INFO, class IE, class CMAX = med::inf>
using sequence_of_t = med::multi_field<IE, 1, CMAX, META_INFO>;
template <class IE, class CMAX = med::inf>
using sequence_of = sequence_of_t<
	meta::typelist<mi<mik::TAG, traits<tg_value::SEQUENCE>>>,
	IE, CMAX
>;

//TODO: set should handle constructed tags
template <class META_INFO, class ...IES>
struct set_t : med::set<med::value<uint8_t>, IES...>
{
	using meta_info = META_INFO;
};
template <class ...IES>
using set = set_t<
	meta::typelist<mi<mik::TAG, traits<tg_value::SET>>>,
	IES...
>;

/*
NOTE: due to exessively bloat ASN.1 specs there are sequence-of and set-of.
Both are about repeated *single* type (multi_field in terms of MED).
And in all encoding rules except for DER/COER these ASN-types are encoded the same
(but different default ASN-classes in BER).
In DER/COER the elements of set-of are ordered increasingly by value prior to encoding.
*/
template <class META_INFO, class IE, class CMAX = med::inf>
using set_of_t = med::multi_field<IE, 1, CMAX, META_INFO>;
template <class IE, class CMAX = med::inf>
using set_of = set_of_t<
	meta::typelist<mi<mik::TAG, traits<tg_value::SET>>>,
	IE, CMAX
>;


} //end: namespace med::asn

