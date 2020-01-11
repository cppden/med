/**
@file
single-instance field and its aggregates
multi-instance field (aka sequence-of) and its aggregates

@copyright Denis Priyomov 2016-2017
Distributed under the MIT License
(See accompanying file LICENSE or visit https://github.com/cppden/med)
*/

#pragma once

#include <iterator>
#include <limits>

#include "ie_type.hpp"
#include "name.hpp"
#include "exception.hpp"
#include "debug.hpp"
#include "meta/typelist.hpp"


namespace med {

template <class T, class = void> struct is_field : std::false_type {};
template <class T> struct is_field<T, std::enable_if_t<std::is_same_v<bool, decltype(std::declval<T>().is_set())>>> : std::true_type {};
template <class T> constexpr bool is_field_v = is_field<T>::value;

template <class, class = void> struct is_multi_field : std::false_type { };
template <class T> struct is_multi_field<T, std::void_t<typename T::field_value>> : std::true_type { };
template <class T> constexpr bool is_multi_field_v = is_multi_field<T>::value;

template <class FIELD, class... META_INFO>
struct field_t : FIELD
{
	using field_type = FIELD;
	//static_assert (is_field_v<FIELD>, "NOT A FIELD");

	//NOTE: since field may have meta_info already we have to override it directly
	// w/o inheritance to avoid ambiguity
	//NOTE: since it's a wrapper the added MI goes 1st
	using meta_info = make_meta_info_t<META_INFO..., FIELD>;
};


template <std::size_t N> struct arity : std::integral_constant<std::size_t, N> {};
template <std::size_t N> struct pmax : std::integral_constant<std::size_t, N> {};
using inf = pmax< std::numeric_limits<std::size_t>::max() >;


namespace detail {

template <std::size_t MIN, class CMAX>
struct get_inplace;

template <std::size_t MIN, std::size_t MAX>
struct get_inplace<MIN, max<MAX>> : std::integral_constant<std::size_t, MAX> {};

template <std::size_t MIN, std::size_t MAX>
struct get_inplace<MIN, pmax<MAX>> : std::integral_constant<std::size_t, MIN> {};

template <class META_INFO>
struct define_meta_info
{
	using meta_info = META_INFO;
};
template <>
struct define_meta_info<void> {};

} //end: namespace detail

template <class FIELD, std::size_t MIN, class CMAX, class META_INFO = void, class... FIELD_META_INFO>
class multi_field : public detail::define_meta_info<META_INFO>
{
	static_assert(MIN > 0, "MIN SHOULD BE GREATER ZERO");
	static_assert(CMAX::value >= MIN, "MAX SHOULD BE GREATER OR EQUAL TO MIN");
	static_assert(is_field_v<FIELD>, "FIELD IS REQUIRED");

public:
	using ie_type = typename FIELD::ie_type;
	using field_type = field_t<FIELD, FIELD_META_INFO...>;

	struct field_value
	{
		field_type   value;
		field_value* next;
	};

	enum bounds : std::size_t
	{
		min     = MIN,
		max     = CMAX::value,
		inplace = detail::get_inplace<MIN, CMAX>::value,
	};

	multi_field() = default;

private:
	template <class T>
	class iter_type
	{
	public:
		using iterator_category = std::forward_iterator_tag;
		using value_type = T;
		using difference_type = std::ptrdiff_t;
		using pointer = conditional_t<std::is_const_v<T>, field_type const*, field_type*>;
		using reference = conditional_t<std::is_const_v<T>, field_type const&, field_type&>;

		explicit iter_type(value_type* p = nullptr) : m_curr{p} { }
		iter_type& operator++()                     { m_curr = m_curr ? m_curr->next : nullptr; return *this; }
		iter_type operator++(int)                   { iter_type ret = *this; ++(*this); return ret;}
		bool operator==(iter_type const& rhs) const { return m_curr == rhs.m_curr; }
		bool operator!=(iter_type const& rhs) const { return !(*this == rhs); }
		reference operator*() const                 { return *get(); }
		pointer operator->() const                  { return get(); }
		pointer get() const                         { return m_curr ? &m_curr->value : nullptr; }
		explicit operator bool() const              { return nullptr != m_curr; }

	private:
		value_type* m_curr;
	};

public:
	using iterator = iter_type<field_value>;
	iterator begin()                                        { return iterator{empty() ? nullptr : m_fields}; }
	iterator end()                                          { return iterator{}; }
	using const_iterator = iter_type<field_value const>;
	const_iterator begin() const                            { return const_iterator{empty() ? nullptr : m_fields}; }
	const_iterator end() const                              { return const_iterator{}; }

	std::size_t count() const                               { return m_count; }
	bool empty() const                                      { return 0 == m_count; }
	//NOTE: clear won't return items allocated from external storage, use reset there
	void clear()                                            { m_count = 0; }
	bool is_set() const                                     { return not empty() && m_fields[0].value.is_set(); }

	field_type const* first() const                         { return empty() ? nullptr : &m_fields[0].value; }
	field_type const* last() const                          { return empty() ? nullptr : &m_tail->value; }

	//uses inplace storage only
	field_type* push_back()
	{
		if (count() < inplace)
		{
			return inplace_push_back();
		}
		MED_THROW_EXCEPTION(out_of_memory, name<field_type>(), sizeof(field_type))
	}

	//uses inplace or external storage
	//NOTE: check for max is done during encode/decode
	template <class CTX> field_type* push_back(CTX& ctx)
	{
		//try inplace 1st then external
		if (count() < inplace)
		{
			return inplace_push_back();
		}
		else
		{
			auto* piv = get_allocator_ptr(ctx)->template allocate<field_value>();
			++m_count;
			m_tail->next = piv;
			piv->next = nullptr;
			m_tail = piv;
			return &m_tail->value;
		}
	}

	//won't recover space if external storage was used
	void pop_back()
	{
		if (auto const num = count())
		{
			//clear the last
			m_tail->value.clear();
			//locate previous before last
			auto* prev = m_fields;
			for (std::size_t i = 2; i < num; ++i)
			{
				prev = prev->next;
			}
			//CODEC_TRACE("%s: %s[%zu]=%p", __FUNCTION__, name<field_type>(), num-1, (void*)prev->next);
			prev->next = nullptr;
			m_tail = (--m_count) ? prev : nullptr;
		}
	}

private:
	//checks are done by caller
	field_type* inplace_push_back()
	{
		auto* piv = m_fields + count();
		//CODEC_TRACE("%s: inplace %s[%zu]=%p", __FUNCTION__, name<field_type>(), count(), (void*)piv);
		if (m_count++) { m_tail->next = piv; }
		piv->next = nullptr;
		m_tail = piv;
		return &m_tail->value;
	}

	multi_field(multi_field const&) = delete;
	multi_field& operator= (multi_field const&) = delete;

	field_value  m_fields[inplace];
	std::size_t  m_count {0};
	field_value* m_tail {nullptr};
};

}	//end: namespace med
