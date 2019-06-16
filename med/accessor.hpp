/**
@file
accessors for fields in container

@copyright Denis Priyomov 2016-2017
Distributed under the MIT License
(See accompanying file LICENSE or visit https://github.com/cppden/med)
*/

#pragma once

#include <type_traits>

#include "field_proxy.hpp"

namespace med {

namespace detail {

template <class T>
struct access
{
	template <class R>
	static auto& as_mandatory(R&& ret)
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

template <class C>
struct index_caccessor
{
	index_caccessor(C const& c, std::size_t i) noexcept : m_that{c}, m_index{i}  { }
	template <class T> operator T const* () const
	{
		return m_that.template get<T>(m_index);
	}

private:
	C const&          m_that;
	std::size_t const m_index;
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

template <class C>
inline auto make_accessor(C const& c, std::size_t index)
{
	return detail::index_caccessor<C>{ c, index };
}

}	//end: namespace med
