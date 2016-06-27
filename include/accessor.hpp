/*!
@file
TODO: define.

@copyright Denis Priyomov 2016
Distributed under the MIT License
(See accompanying file LICENSE or copy at https://opensource.org/licenses/MIT)
*/

#pragma once

#include <type_traits>
#include <limits>

#include "field.hpp"

namespace med {

//template< class T, class U >
//struct const_as : std::conditional<
//	std::is_const<T>::value, std::add_const_t<U>, std::remove_const_t<U>
//>
//{
//};
//
//template< class T, class U >
//using const_as_t = typename const_as<T, U>::type;


namespace detail {

template <class T>
struct access
{
	template <class R>
	static auto& as_mandatory(R&& ret)
	{
		static_assert(!std::is_same<field_proxy<T const>, R>(), "ACCESSING OPTIONAL NOT BY POINTER");
		return ret;
	}

	template <class R>
	static auto as_optional(R&& ret)
	{
		static_assert(std::is_same<field_proxy<T const>, R>(), "ACCESSING MANDATORY NOT BY REFERENCE");
		return ret;
	}
};

template <std::size_t INDEX, class CONTAINER>
struct accessor
{
	explicit accessor(CONTAINER& c) noexcept : m_that(c)  { }
	template <class T> operator T& ()               { return m_that.template ref<T, INDEX>(); }

private:
	CONTAINER& m_that;
};

template <class CONTAINER>
struct accessor<std::numeric_limits<std::size_t>::max(), CONTAINER>
{
	explicit accessor(CONTAINER& c) noexcept : m_that(c)  { }
	template <class T> operator T& ()               { return m_that.template ref<T>(); }

private:
	CONTAINER& m_that;
};

template <std::size_t INDEX, class CONTAINER>
struct caccessor
{
	explicit caccessor(CONTAINER const& c) noexcept : m_that(c) { }
	template <class T> operator T const* () const
	{
		return access<T>::as_optional(m_that.template get<T, INDEX>());
	}

	template <class T> operator T const& () const
	{
		return access<T>::as_mandatory(m_that.template get<T, INDEX>());
	}

private:
	CONTAINER const& m_that;
};

template <class CONTAINER>
struct caccessor<std::numeric_limits<std::size_t>::max(), CONTAINER>
{
	explicit caccessor(CONTAINER const& c) noexcept : m_that(c) { }
	template <class T> operator T const* () const
	{
		return access<T>::as_optional(m_that.template get<T>());
	}

	template <class T> operator T const& () const
	{
		return access<T>::as_mandatory(m_that.template get<T>());
	}

private:
	CONTAINER const& m_that;
};

template <class CONTAINER>
struct index_accessor
{
	index_accessor(CONTAINER& c, std::size_t i) noexcept : m_that{c}, m_index{i}  { }
	template <class T> operator T& ()               { return m_that.template ref<T>(m_index); }

private:
	CONTAINER&        m_that;
	std::size_t const m_index;
};

template <class CONTAINER>
struct index_caccessor
{
	index_caccessor(CONTAINER const& c, std::size_t i) noexcept : m_that{c}, m_index{i}  { }
	template <class T> operator T const* () const   { return m_that.template get<T>(m_index); }

private:
	CONTAINER const&  m_that;
	std::size_t const m_index;
};


}	//end: namespace detail

template <class CONTAINER>
inline auto make_accessor(CONTAINER& c)
{
	return detail::accessor<std::numeric_limits<std::size_t>::max(), CONTAINER>{ c };
}

template <class CONTAINER>
inline auto make_accessor(CONTAINER const& c)
{
	return detail::caccessor<std::numeric_limits<std::size_t>::max(), CONTAINER>{ c };
}

template <std::size_t INDEX, class CONTAINER>
inline auto make_accessor(CONTAINER& c)
{
	return detail::accessor<INDEX, CONTAINER>{ c };
}

template <std::size_t INDEX, class CONTAINER>
inline auto make_accessor(CONTAINER const& c)
{
	return detail::caccessor<INDEX, CONTAINER>{ c };
}

template <class CONTAINER>
inline auto make_accessor(CONTAINER& c, std::size_t index)
{
	return detail::index_accessor<CONTAINER>{ c, index };
}

template <class CONTAINER>
inline auto make_accessor(CONTAINER const& c, std::size_t index)
{
	return detail::index_caccessor<CONTAINER>{ c, index };
}

}	//end: namespace med
