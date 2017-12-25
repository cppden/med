/**
@file
proxy helper classes to simplify field access

@copyright Denis Priyomov 2016-2017
Distributed under the MIT License
(See accompanying file LICENSE or visit https://github.com/cppden/med)
*/

#pragma once

#include <type_traits>
#include "multi_field.hpp"

namespace med {


template <class, class Enable = void>
struct has_ref_field : std::false_type { };
template <class T>
struct has_ref_field<T, void_t<decltype(std::declval<T>().ref_field())>> : std::true_type { };

template <class IE>
std::enable_if_t<has_ref_field<IE>::value, typename IE::field_type&>
inline ref_field(IE& ie) { return ie.ref_field(); }
template <class IE>
std::enable_if_t<has_ref_field<IE>::value, typename IE::field_type const&>
inline ref_field(IE const& ie) { return ie.ref_field(); }
template <class IE>
std::enable_if_t<!has_ref_field<IE>::value, IE&>
inline ref_field(IE& ie) { return ie; }

//read-only access optional field returning a pointer or null if not set
template <class FIELD, class IE>
std::enable_if_t<!is_multi_field_v<IE> && is_optional_v<IE>, FIELD const*>
inline get_field(IE const& ie)
{
	return ie.is_set() ? &ie.ref_field() : nullptr;
}

//read-only access mandatory field returning a reference
template <class FIELD, class IE>
std::enable_if_t<!is_multi_field_v<IE> && !is_optional_v<IE>, FIELD const&>
inline get_field(IE const& ie)
{
	return ie.ref_field();
}

//read-only access of any multi-field
template <class FIELD, class IE>
std::enable_if_t<is_multi_field_v<IE>, IE const&>
inline get_field(IE const& ie)
{
	return ie;
}

}	//end: namespace med
