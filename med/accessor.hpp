/**
@file
accessors for fields in container

@copyright Denis Priyomov 2016-2017
Distributed under the MIT License
(See accompanying file LICENSE or visit https://github.com/cppden/med)
*/

#pragma once

#include <type_traits>

#include "field.hpp"

namespace med {

//read-only access optional field returning a pointer or null if not set
template <class FIELD, class IE> requires (!AMultiField<IE> && AOptional<IE>)
FIELD const* get_field(IE const& ie)
{
	return ie.is_set() ? &ie : nullptr;
}

//read-only access mandatory field returning a reference
template <class FIELD, class IE> requires (!AMultiField<IE> && !AOptional<IE>)
FIELD const& get_field(IE const& ie)
{
	return ie;
}

//read-only access of any multi-field
template <class FIELD, AMultiField IE>
constexpr IE const& get_field(IE const& ie)
{
	return ie;
}

namespace sl {

template <class FIELD>
struct field_at
{
	template <class T>
	using type = std::is_same<FIELD, get_field_type_t<T>>;
	template <class T>
	static constexpr bool value = std::is_same_v<FIELD, get_field_type_t<T>>;
};

} //end: namespace sl

}	//end: namespace med
