/*!
@file
TODO: define.

@copyright Denis Priyomov 2016
Distributed under the MIT License
(See accompanying file LICENSE or copy at https://opensource.org/licenses/MIT)
*/

#pragma once

#include <limits>
#include <iterator>

#include "allocator.hpp"
#include "field.hpp"


namespace med {


template <class FIELD, std::size_t MIN, std::size_t MAX, std::size_t INPLACE = MIN>
class multi_field_t
{
	static_assert(INPLACE <= MAX, "MAX SHOULD BE GREATER OR EQUAL TO INPLACE STORAGE");
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
		inplace = INPLACE,
	};

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
	};

public:
	using iterator = iter_type<field_value>;
	iterator begin()                                        { return iterator{m_fields}; }
	iterator end()                                          { return iterator{}; }
	using const_iterator = iter_type<field_value const>;
	const_iterator begin() const                            { return const_iterator{m_fields}; }
	const_iterator end() const                              { return const_iterator{}; }

	std::size_t count() const                               { return m_count; }
	//NOTE: clear won't return items allocated from external storage, use reset there
	void clear()                                            { m_count = 0; }
	bool is_set() const                                     { return m_count > 0 && m_fields[0].value.is_set(); }

	field_type const& ref_field(std::size_t index) const    { return const_cast<multi_field_t*>(this)->ref_field(index); }
	field_type& ref_field(std::size_t index)                { return m_fields[index].value; }

	field_type const* get_field(std::size_t index) const
	{
		const_iterator it = begin(), ite = end();
		while (index && it != ite) { --index; ++it; }
		return it.get() && it->is_set() ? it.get() : nullptr;
	}
	field_type* get_field(std::size_t index)
	{
		iterator it = begin(), ite = end();
		while (index && it != ite) { --index; ++it; }
		return it.get();
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

		return nullptr;
	}

	//uses inplace or external storage
	//NOTE: check for max is done during encode/decode
	template <class CTX>
	field_type* push_back(CTX& ctx)
	{
		//try inplace 1st then external
		field_type* pv = push_back();
		if (!pv)
		{
			if (auto* piv = get_allocator_ptr(ctx)->template allocate<field_value>())
			{
				++m_count;
				m_tail->next = piv;
				piv->next = nullptr;
				m_tail = piv;
				pv = &m_tail->value;
			}
		}
		return pv;
	}

private:
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
//                        typename T::value_type, remove_cref_t<decltype(*std::declval<T>().begin())>
//                >::value
//        >
//> : std::true_type { };
template <class T>
constexpr bool is_multi_field_v = is_multi_field<T>::value;

}	//end: namespace med
