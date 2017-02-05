/**
@file
proxy helper classes to simplify field access

@copyright Denis Priyomov 2016-2017
Distributed under the MIT License
(See accompanying file LICENSE or visit https://github.com/cppden/med)
*/

#pragma once

#include <type_traits>

//#include "optional.hpp"

namespace med {

template <class T, typename Enable = void>
struct has_arrow_operator : std::false_type {};

template <class T>
struct has_arrow_operator<T, std::enable_if_t<
	std::is_pointer<decltype(std::declval<T>().operator->())>::value>> : std::true_type {};

template <class T>
inline auto get_arrow(T* ptr) -> decltype(std::declval<T>().operator->())
{
	return (*ptr).operator->();
}

template <class T>
std::enable_if_t<!has_arrow_operator<T>::value, T*>
inline get_arrow(T* ptr)
{
	return ptr;
}

//needed to invoke overloaded ->
template <class T>
class field_proxy
{
public:
	explicit field_proxy(T* field)
		: m_field{field}
	{}

	decltype(auto) operator->() const   { return get_arrow(m_field); }
	T& operator*() const                { return *m_field; }
	operator T*() const                 { return m_field; }
	explicit operator bool() const      { return nullptr != m_field; }

private:
	T* m_field;
};

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
std::enable_if_t<!is_multi_field_v<IE> && is_optional_v<IE>, field_proxy<FIELD const>>
inline get_field(IE const& ie)
{
	return field_proxy<FIELD const>{ ie.is_set() ? &ie.ref_field() : nullptr };
};

//read-only access mandatory field returning a reference
template <class FIELD, class IE>
std::enable_if_t<!is_multi_field_v<IE> && !is_optional_v<IE>, FIELD const&>
inline get_field(IE const& ie)
{
	return ie.ref_field();
};

//read-only access of any multi-field
template <class FIELD, class IE>
std::enable_if_t<is_multi_field_v<IE>, IE const&>
inline get_field(IE const& ie)
{
	return ie;
};

}	//end: namespace med
