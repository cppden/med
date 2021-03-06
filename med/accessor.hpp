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
template <class FIELD, class IE>
inline std::enable_if_t<!is_multi_field_v<IE> && is_optional_v<IE>, FIELD const*> get_field(IE const& ie)
{
	return ie.is_set() ? &ie : nullptr;
}

//read-only access mandatory field returning a reference
template <class FIELD, class IE>
constexpr std::enable_if_t<!is_multi_field_v<IE> && !is_optional_v<IE>, FIELD const&> get_field(IE const& ie)
{
	return ie;
}

//read-only access of any multi-field
template <class FIELD, class IE>
constexpr std::enable_if_t<is_multi_field_v<IE>, IE const&> get_field(IE const& ie)
{
	return ie;
}

namespace sl {

template <class FIELD>
struct field_at
{
	template <class T>
	using type = std::is_same<FIELD, get_field_type_t<T>>;
};

} //end: namespace sl

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
		return m_that.template ref<T>();
	}
	template <class T, class = std::enable_if_t<std::is_const_v<T>>> operator T* () const
	{
		return access<T>::as_optional(m_that.template get<std::remove_const_t<T>>());
	}
	template <class T, class = std::enable_if_t<std::is_pointer_v<T> && !std::is_const_v<T>>> operator T* ()
	{
		static_assert(!std::is_pointer<T>(), "INVALID ACCESS OF MANDATORY BY POINTER");
		return T{};
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
