#pragma once

/**
@file
ASN.1

@copyright Denis Priyomov 2018
Distributed under the MIT License
(See accompanying file LICENSE or visit https://github.com/cppden/med)
*/
#include "ids.hpp"
#include "value.hpp"
#include "bit_string.hpp"
#include "sequence.hpp"
#include "set.hpp"

namespace med::asn {

template <typename T, class... ASN_TRAITS> //TRAITs is asn::traits<T,C>
struct value_t : value<T>, add_meta_info<mi<mik::TAG, ASN_TRAITS>...>{};

template <class... ASN_TRAITS>
using boolean_t = value_t<bool, ASN_TRAITS...>;
using boolean = boolean_t<traits<tg_value::BOOLEAN>>;

template <class... ASN_TRAITS>
struct null_t : empty<>, add_meta_info<mi<mik::TAG, ASN_TRAITS>...> {};
using null = null_t<traits<tg_value::NULL_TYPE>>;

//NOTE! ASN assumes the integer is signed! don't use unsigned ever
using integer = value_t<int, traits<tg_value::INTEGER>>;

template <class... ASN_TRAITS>
using enumerated_t = value_t<int, ASN_TRAITS...>;
using enumerated = enumerated_t<traits<tg_value::ENUMERATED>>;

template <class... ASN_TRAITS>
using real_t = value_t<double, ASN_TRAITS...>;
using real = real_t<traits<tg_value::REAL>>;

template <class... ASN_TRAITS>
struct bit_string_t : med::bit_string<>, add_meta_info<mi<mik::TAG, ASN_TRAITS>...> {};
using bit_string = bit_string_t<traits<tg_value::BIT_STRING>>;

template <class... ASN_TRAITS>
struct octet_string_t : med::octet_string<octets_var_extern>, add_meta_info<mi<mik::TAG, ASN_TRAITS>...> {};
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

template <class META_INFO, class IE, class CMAX = inf>
using sequence_of_t = multi_field<IE, 1, CMAX, META_INFO>;
template <class IE, class CMAX = inf>
using sequence_of = sequence_of_t<
	meta::typelist<mi<mik::TAG, traits<tg_value::SEQUENCE>>>,
	IE, CMAX
>;

template <class META_INFO, class ...IES>
struct set_t : med::set<IES...>
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
template <class META_INFO, class IE, class CMAX = inf>
using set_of_t = multi_field<IE, 1, CMAX, META_INFO>;
template <class IE, class CMAX = inf>
using set_of = set_of_t<
	meta::typelist<mi<mik::TAG, traits<tg_value::SET>>>,
	IE, CMAX
>;

template <class META_INFO, class ...IES>
struct choice_t : med::choice<IES...>
{
	using meta_info = META_INFO;
};
template <class ...IES>
using choice = choice_t<
	meta::typelist<>,
	IES...
>;

using subid_t = uintmax_t;
//TODO: join CMAX and TRAITS and filter out if 1st specified
//Relative OID has at least 1 component
template <class CMAX, class... ASN_TRAITS>
using relative_oid_t = multi_field<
	value<subid_t>, 1, CMAX,
	meta::typelist<mi<mik::TAG, ASN_TRAITS>...>
>;
template <class CMAX = inf>
using relative_oid = relative_oid_t<CMAX, traits<tg_value::RELATIVE_OID>>;

namespace detail {

//another example of marvelous ASN.1 (what?!1...)
static constexpr subid_t OID_ROOT_FACTOR = 40;

//extract root (1st component) from the 1st subidentifier
constexpr uint8_t oid_root(subid_t v)
{
	return (v < 2 * OID_ROOT_FACTOR) ? (v / OID_ROOT_FACTOR) : 2;
}

//extract subroot (2nd component) from the 1st subidentifier
constexpr subid_t oid_subroot(subid_t v)
{
	return v - OID_ROOT_FACTOR * oid_root(v);
}

//construct subidentifier from two 1st components
constexpr subid_t oid_make_1st(uint8_t root, subid_t subroot)
{
	if ((root < 2 && subroot >= OID_ROOT_FACTOR)
		|| root > 2)
	{
		MED_THROW_EXCEPTION(invalid_value, __FUNCTION__, subroot);
	}
	return root * OID_ROOT_FACTOR + subroot;
}

} //end: namespace detail

//OID has at least 2 components
template <class CMAX, class... ASN_TRAITS>
struct object_identifier_t : relative_oid_t<CMAX, ASN_TRAITS...>
{
	//returns 1st two components
	std::pair<uint8_t, subid_t> root() const
	{
		subid_t const v = this->empty() ? 0 : this->first()->get();
		return std::pair{detail::oid_root(v), detail::oid_subroot(v)};
	}
	void root(subid_t root_value, subid_t subroot_value)
	{
		if (this->empty()) { this->push_back(); }
		this->first()->set(detail::oid_make_1st(root_value, subroot_value));
	}
};

template <class CMAX = inf>
using object_identifier = object_identifier_t<CMAX, traits<tg_value::OBJECT_IDENTIFIER>>;


template <class, class = void> struct is_oid : std::false_type { };
template <class T> struct is_oid<T, std::enable_if_t<
		is_multi_field_v<T> and !has_meta_info_v<typename T::field_type>>> : std::true_type { };
template <class T> constexpr bool is_oid_v = is_oid<T>::value;

template <class, class = void> struct is_seqof : std::false_type { };
template <class T> struct is_seqof<T, std::enable_if_t<
		is_multi_field_v<T> and has_meta_info_v<typename T::field_type>>> : std::true_type { };
template <class T> constexpr bool is_seqof_v = is_seqof<T>::value;

} //end: namespace med::asn

