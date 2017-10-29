/**
@file
multi-instance field (aka sequence-of) and its aggregates

@copyright Denis Priyomov 2016-2017
Distributed under the MIT License
(See accompanying file LICENSE or visit https://github.com/cppden/med)
*/

#pragma once

#include <iterator>

#include "allocator.hpp"
#include "field.hpp"


namespace med {


template <class FIELD, std::size_t MIN, std::size_t MAX>
class multi_field_t
{
	static_assert(MIN > 0, "MIN SHOULD BE GREATER ZERO");
	static_assert(MAX >= MIN, "MAX SHOULD BE GREATER OR EQUAL TO MIN");
	//static_assert(MIN <= INPLACE && INPLACE <= MAX, "INPLACE SHOULD BE BETWEEN MIN AND MAX");
	static_assert(is_field_v<FIELD>, "FIELD IS REQUIRED");

public:
	using ie_type = typename FIELD::ie_type;
	using field_type = FIELD;

	struct field_value
	{
		field_type   value;
		field_value* next;
	};

	enum bounds : std::size_t
	{
		min     = MIN,
		max     = MAX,
		inplace = (MIN == MAX || sizeof(FIELD) < (16*1024/MAX)) ? MAX : MIN,
	};

	multi_field_t() = default;

private:
	template <class T>
	struct iter_base : std::iterator<
		std::forward_iterator_tag,
		T,
		std::ptrdiff_t,
		std::conditional_t<std::is_const<T>::value, field_type const*, field_type*>,
		std::conditional_t<std::is_const<T>::value, field_type const&, field_type&>
	> {};

	template <class T>
	class iter_type : public iter_base<T>
	{
		using value_type = typename iter_base<T>::value_type;
		using reference = typename iter_base<T>::reference;
		using pointer = typename iter_base<T>::pointer;
		value_type* m_curr;

	public:
		explicit iter_type(value_type* p = nullptr) : m_curr{p} { }
		iter_type& operator++()                     { m_curr = m_curr ? m_curr->next : nullptr; return *this; }
		iter_type operator++(int)                   { iter_type ret = *this; ++(*this); return ret;}
		bool operator==(iter_type const& rhs) const { return m_curr == rhs.m_curr; }
		bool operator!=(iter_type const& rhs) const { return !(*this == rhs); }
		reference operator*() const                 { return *get(); }
		pointer operator->() const                  { return get(); }
		pointer get() const                         { return m_curr ? &m_curr->value : nullptr; }
		explicit operator bool() const              { return nullptr != m_curr; }
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
	bool is_set() const                                     { return m_count > 0 && m_fields[0].value.is_set(); }

	field_type const* first() const                         { return &m_fields[0]; }
	field_type const* last() const                          { return &m_tail->value; }

//	field_type* at(std::size_t index)
//	{
//		iterator it = begin(), ite = end();
//		while (index && it != ite) { --index; ++it; }
//		return it.get();
//	}

	//ineffective read-only access
	field_type const* at(std::size_t index) const
	{
		if (index >= count()) return nullptr;
		if (index < inplace) return &m_fields[index].value;
		const_iterator it = begin(), ite = end();
		while (index && it != ite) { --index; ++it; }
		return (it && it->is_set()) ? it.get() : nullptr;
	}

	//uses inplace storage only
	field_type* push_back()
	{
		if (count() < inplace)
		{
			auto* piv = m_fields + count();
			if (m_count++) { m_tail->next = piv; }
			piv->next = nullptr;
			m_tail = piv;
			return &m_tail->value;
		}
#ifndef MED_NO_EXCEPTION
		throw exception(error::OUT_OF_MEMORY, "No in-place space for IE '%s': %zu bytes", name<field_type>(), sizeof(field_type));
#endif
		return nullptr;
	}

	//uses inplace or external storage
	//NOTE: check for max is done during encode/decode
	template <class CTX>
	field_type* push_back(CTX& ctx)
	{
		//try inplace 1st then external
		if (count() < inplace)
		{
			return push_back();
		}
		else
		{
			if (auto* piv = get_allocator_ptr(ctx)->template allocate<field_value>())
			{
				++m_count;
				m_tail->next = piv;
				piv->next = nullptr;
				m_tail = piv;
				return &m_tail->value;
			}
		}
		return nullptr;
	}

private:
	multi_field_t(multi_field_t const&) = delete;
	multi_field_t& operator= (multi_field_t const&) = delete;

	field_value  m_fields[inplace];
	std::size_t  m_count {0};
	field_value* m_tail {nullptr};
};


template <class TAG, class VAL, std::size_t MIN, std::size_t MAX>
struct multi_tag_value_t : multi_field_t<VAL, MIN, MAX>, tag_t<TAG>
{
	using ie_type = IE_TV;
};

template <class LEN, class VAL, std::size_t MIN, std::size_t MAX>
struct multi_length_value_t : multi_field_t<VAL, MIN, MAX>, LEN
{
	using ie_type = IE_LV;
};

template <class, class Enable = void>
struct is_multi_field : std::false_type { };
template <class T>
struct is_multi_field<T, void_t<typename T::field_value>> : std::true_type { };
//template <class T>
//struct is_multi_field<T,
//        std::enable_if_t<
//                std::is_same<
//                        typename T::iterator::reference, decltype(*std::declval<T>().begin())
//                >::value
//        >
//> : std::true_type { };
template <class T>
constexpr bool is_multi_field_v = is_multi_field<T>::value;

}	//end: namespace med
