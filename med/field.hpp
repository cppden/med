/**
@file
single-instance field and its aggregates
multi-instance field (aka sequence-of) and its aggregates

@copyright Denis Priyomov 2016-2017
Distributed under the MIT License
(See accompanying file LICENSE or visit https://github.com/cppden/med)
*/

#pragma once

#include <algorithm>
#include <iterator>

#include "ie_type.hpp"
#include "name.hpp"
#include "exception.hpp"
#include "debug.hpp"
#include "meta/typelist.hpp"
#include "allocator.hpp"
#include "concepts.hpp"


namespace med {

template <AField FIELD, class... META_INFO>
struct field_t : FIELD
{
	using field_type = FIELD;

	//NOTE: since field may have meta_info already we have to override it directly
	// w/o inheritance to avoid ambiguity
	//NOTE: since it's a wrapper the added MI goes 1st
	using meta_info = meta::list_append_t<get_meta_info_t<META_INFO>..., get_meta_info_t<FIELD>>;
};

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

template <AField FIELD, std::size_t MIN, class CMAX, class META_INFO = void, class... FIELD_META_INFO>
class multi_field : public detail::define_meta_info<META_INFO>
{
	static_assert(MIN > 0, "MIN SHOULD BE GREATER ZERO");
	static_assert(CMAX::value >= MIN, "MAX SHOULD BE GREATER OR EQUAL TO MIN");

public:
	using ie_type = typename FIELD::ie_type;
	using field_type = field_t<FIELD, FIELD_META_INFO...>;

	struct field_value
	{
		field_type   value;
		field_value* next;
	};

	static constexpr std::size_t min = MIN;
	static constexpr std::size_t max = CMAX::value;
	static constexpr std::size_t inplace = detail::get_inplace<MIN, CMAX>::value;

	multi_field(multi_field const&) = delete;
	multi_field& operator= (multi_field const&) = delete;
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
		template <AField, std::size_t, class, class, class...> friend class multi_field;
		value_type* m_curr;
	};

public:
	using iterator = iter_type<field_value>;
	iterator begin()                                        { return iterator{m_head}; }
	iterator end()                                          { return iterator{}; }
	using const_iterator = iter_type<field_value const>;
	const_iterator begin() const                            { return const_iterator{m_head}; }
	const_iterator end() const                              { return const_iterator{}; }

	std::size_t count() const                               { return m_count; }
	bool empty() const                                      { return nullptr == m_head; }
	//NOTE: clear won't return items allocated from external storage, use reset there
	void clear()
	{
		for (auto& v : *this) { v.clear(); }
		m_head = m_tail = nullptr;
		m_count = 0;
	}
	bool is_set() const                                     { return not empty() && m_head->value.is_set(); }

	field_type* first()                                     { return empty() ? nullptr : &m_head->value; }
	field_type* last()                                      { return empty() ? nullptr : &m_tail->value; }
	field_type const* first() const                         { return const_cast<multi_field*>(this)->first(); }
	field_type const* last() const                          { return const_cast<multi_field*>(this)->last(); }

	//uses inplace storage only
	field_type* push_back()                                 { return append(get_free_inplace()); }

	//uses inplace or external storage
	//NOTE: check for max is done during encode/decode
	template <class CTX> field_type* push_back(CTX& ctx)
	{
		auto* pf = get_free_inplace(); //try inplace 1st then external
		return append(pf ? pf : create<field_value>(get_allocator(ctx)));
	}

	//won't recover space if external storage was used
	void pop_back()
	{
		if (auto* prev = m_head)
		{
			//clear the last
			--m_count;
			m_tail->value.clear();

			//locate previous before last
			for (std::size_t i = 1; i < count(); ++i)
			{
				CODEC_TRACE("%s: %s[%zu]=%p", __FUNCTION__, name<field_type>(), i, (void*)prev->next);
				prev = prev->next;
			}

			if (count())
			{
				prev->next->value.clear();
				m_tail = (count()) ? prev : nullptr;
			}
			else
			{
				m_head = m_tail = nullptr;
			}

			prev->next = nullptr;
		}
	}

	iterator erase(iterator pos)
	{
		for (auto it = begin(), prev = it; it; ++it)
		{
			if (it == pos)
			{
				--m_count;
				it.m_curr->value.clear();
				if (it == prev) //1st
				{
					m_head = m_head->next;
					return begin();
				}
				else
				{
					prev.m_curr->next = it.m_curr->next;
					return ++prev;
				}
			}
			prev = it;
		}
		return end();
	}

	bool operator==(multi_field const& rhs) const noexcept
	{
		return count() == rhs.count() && std::equal(begin(), end(), rhs.begin());
	}

private:
	//find unset inplace slot
	field_value* get_free_inplace()
	{
		if (count() < inplace)
		{
			for (auto& f : m_fields)
			{
				CODEC_TRACE("%s: %s=%p[%c]", __FUNCTION__, name<field_type>(), (void*)&f, f.value.is_set()?'+':'-');
				if (!f.value.is_set()) return &f;
			}
		}
		return nullptr;
	}

	field_type* append(field_value* pf)
	{
		if (!pf) { MED_THROW_EXCEPTION(out_of_memory, name<field_type>(), sizeof(field_type)) }

		CODEC_TRACE("%s(%s=%p) count=%zu", __FUNCTION__, name<field_type>(), (void*)pf, count());
		if (m_count++) { m_tail->next = pf; }
		else { m_head = m_tail = pf; }
		pf->next = nullptr;
		m_tail = pf;
		return &m_tail->value;
	}

	field_value  m_fields[inplace];
	std::size_t  m_count {0};
	field_value* m_head {nullptr};
	field_value* m_tail {nullptr};
};

}	//end: namespace med
