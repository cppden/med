#pragma once

/**
@file
ASN.1

@copyright Denis Priyomov 2018
Distributed under the MIT License
(See accompanying file LICENSE or visit https://github.com/cppden/med)
*/

#include "ids.hpp"
#include "../value.hpp"
#include "../octet_string.hpp"
#include "../bit_string.hpp"
#include "../sequence.hpp"

namespace med::asn {

template <typename T, std::size_t TAG, tg_class CLASS = tg_class::UNIVERSAL, tg_type TYPE = tg_type::IMPLICIT>
using value_t = med::value<T, traits<TAG, CLASS, TYPE>>;

template <std::size_t TAG, tg_class CLASS = tg_class::UNIVERSAL, tg_type TYPE = tg_type::IMPLICIT>
using boolean_t = value_t<bool, TAG, CLASS, TYPE>;
using boolean = boolean_t<tg_value::BOOLEAN>;

template <std::size_t TAG, tg_class CLASS = tg_class::UNIVERSAL, tg_type TYPE = tg_type::IMPLICIT>
using null_t = med::empty<traits<TAG, CLASS, TYPE>>;
using null = null_t<tg_value::NULL_TYPE>;

using integer = value_t<int, tg_value::INTEGER>;

template <std::size_t TAG, tg_class CLASS = tg_class::UNIVERSAL, tg_type TYPE = tg_type::IMPLICIT>
using enumerated_t = value_t<int, TAG, CLASS, TYPE>;
using enumerated = enumerated_t<tg_value::ENUMERATED>;

template <std::size_t TAG, tg_class CLASS = tg_class::UNIVERSAL, tg_type TYPE = tg_type::IMPLICIT>
using real_t = value_t<double, TAG, CLASS, TYPE>;
using real = real_t<tg_value::REAL>;

template <std::size_t TAG, tg_class CLASS = tg_class::UNIVERSAL, tg_type TYPE = tg_type::IMPLICIT>
using bit_string_t = med::bit_string<traits<TAG, CLASS, TYPE>>;
using bit_string = bit_string_t<tg_value::BIT_STRING>;

template <std::size_t TAG, tg_class CLASS = tg_class::UNIVERSAL, tg_type TYPE = tg_type::IMPLICIT>
using octet_string_t = med::octet_string<octets_var_extern, traits<TAG, CLASS, TYPE>>;
using octet_string = octet_string_t<tg_value::OCTET_STRING>;


template <std::size_t TAG, tg_class CLASS, tg_type TYPE, class ...IES>
using sequence_t = med::base_sequence<traits<TAG, CLASS, TYPE>, IES...>;
template <class ...IES>
using sequence = sequence_t<tg_value::SEQUENCE, tg_class::UNIVERSAL, tg_type::IMPLICIT, IES...>;

} //end: namespace med::asn

