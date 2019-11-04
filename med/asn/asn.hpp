#pragma once

/**
@file
ASN.1

@copyright Denis Priyomov 2018
Distributed under the MIT License
(See accompanying file LICENSE or visit https://github.com/cppden/med)
*/
//#include <cstdtypes>
#define BRIGAND_NO_BOOST_SUPPORT
#include <brigand.hpp>

#include "ids.hpp"
#include "../value.hpp"
#include "../octet_string.hpp"
#include "../bit_string.hpp"
#include "../sequence.hpp"

namespace med::asn {


template <std::size_t TAG>
using tag_value = value<fixed<TAG, std::size_t>>;

template <typename T, class... TRAITs> //TRAITs is asn::traits<T,C>
struct value_t : value<T>
{
	using meta_info = meta::typelist<mi<mik::TAG, TRAITs>...>;
};


template <class... TRAITs>
using boolean_t = value_t<bool, TRAITs...>;
using boolean = boolean_t<traits<tg_value::BOOLEAN>>;

template <class... TRAITs>
struct null_t : med::empty<>
{
	using meta_info = meta::typelist<mi<mik::TAG, TRAITs>...>;
};
using null = null_t<traits<tg_value::NULL_TYPE>>;

using integer = value_t<int, traits<tg_value::INTEGER>>;

template <class... TRAITs>
using enumerated_t = value_t<int, TRAITs...>;
using enumerated = enumerated_t<traits<tg_value::ENUMERATED>>;

template <class... TRAITs>
using real_t = value_t<double, TRAITs...>;
using real = real_t<traits<tg_value::REAL>>;

template <class... TRAITs>
struct bit_string_t : med::bit_string<>
{
	using meta_info = meta::typelist<mi<mik::TAG, TRAITs>...>;
};
using bit_string = bit_string_t<traits<tg_value::BIT_STRING>>;

template <class... TRAITs>
struct octet_string_t : med::octet_string<octets_var_extern>
{
	using meta_info = meta::typelist<mi<mik::TAG, TRAITs>...>;
};
using octet_string = octet_string_t<traits<tg_value::OCTET_STRING>>;


template <class TRAIT_LIST, class ...IES>
struct sequence_t : med::sequence<IES...>
{
	using meta_info = TRAIT_LIST;
};
template <class ...IES>
using sequence = sequence_t<meta::typelist<traits<tg_value::SEQUENCE>>, IES...>;

} //end: namespace med::asn

