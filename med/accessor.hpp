/**
@file
accessors for fields in container

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
struct has_ref_field<T, std::void_t<decltype(std::declval<T>().ref_field())>> : std::true_type { };

//read-write access for any field returning a reference to the field
template <class IE>
decltype(auto) ref_field(IE& ie) //-> decltype(ie.ref_field())
{
	if constexpr (has_ref_field<IE>::value)
	{
		return ie.ref_field();
	}
	else
	{
		return ie;
	}
}

//TODO: remove
template <class IE>
decltype(auto) rref_field(IE& ie)
{
	if constexpr (has_ref_field<IE>::value)
	{
		return rref_field(ie.ref_field());
	}
	else
	{
		return ie;
	}
}


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

namespace detail {

template <class T>
struct access
{
	template <class R>
	static auto as_mandatory(R&& ret)
	{
		static_assert(!std::is_same<T const*, R>(), "ACCESSING OPTIONAL NOT BY POINTER");
		return ret;
	}

	template <class R>
	static auto as_optional(R&& ret)
	{
		static_assert(std::is_same<T const*, R>(), "ACCESSING MANDATORY NOT BY REFERENCE");
		return ret;
	}
};

template <class C>
struct accessor
{
	explicit accessor(C& c) noexcept : m_that(c)  { }

	template <class T, class = std::enable_if_t<!std::is_pointer_v<T>>> operator T& ()
	{
		//static_assert(!std::is_pointer<T>(), "INVALID ACCESS BY NON-CONST POINTER");
		return m_that.template ref<T>();
	}
	template <class T, class = std::enable_if_t<std::is_const_v<T>>> operator T* () const
	{
		return access<T>::as_optional(m_that.template get<std::remove_const_t<T>>());
	}

private:
	C& m_that;
};

template <class C>
struct caccessor
{
	explicit caccessor(C const& c) noexcept : m_that(c) { }
	template <class T> operator T const* () const
	{
		return access<T>::as_optional(m_that.template get<T>());
	}

	template <class T> operator T const& () const
	{
		return access<T>::as_mandatory(m_that.template get<T>());
	}

private:
	C const& m_that;
};

}	//end: namespace detail

template <class C>
inline auto make_accessor(C& c)
{
	return detail::accessor<C>{ c };
}

template <class C>
inline auto make_accessor(C const& c)
{
	return detail::caccessor<C>{ c };
}

}	//end: namespace med
