/*!
TODO: define.

@copyright Denis Priyomov 2016
Distributed under the MIT License
(See accompanying file LICENSE or copy at https://opensource.org/licenses/MIT)
*/

#pragma once

#include "ie.hpp"
#include "state.hpp"
#include "allocator.hpp"
#include "container.hpp"
#include "encode.hpp"
#include "decode.hpp"
#include "debug.hpp"

namespace med {

namespace detail {

template <class FIELD, std::size_t MIN, std::size_t MAX, std::size_t INPLACE>
class seq_of : public IE<CONTAINER>
{
public:
	using value_type = FIELD;
	static_assert(is_field_v<value_type>, "FIELD IS REQUIRED");

	enum bounds : std::size_t
	{
		min     = MIN,
		max     = MAX,
		inplace = INPLACE,
	};

private:
	struct value_t
	{
		value_type value;
		value_t*   next;
	};

	template <class T>
	struct iter_base : std::iterator<
		std::forward_iterator_tag,
		T,
		std::ptrdiff_t,
		std::conditional_t<std::is_const<T>::value, value_type const*, value_type*>,
		std::conditional_t<std::is_const<T>::value, value_type const&, value_type&>
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
	using iterator = iter_type<value_t>;
	iterator begin()                                        { return iterator{m_items}; }
	iterator end()                                          { return iterator{}; }
	using const_iterator = iter_type<value_t const>;
	const_iterator begin() const                            { return const_iterator{m_items}; }
	const_iterator end() const                              { return const_iterator{}; }

	std::size_t count() const                               { return m_count; }
	std::size_t calc_length() const
	{
		CODEC_TRACE("calc_length[%s]*", name<FIELD>());
		std::size_t len = 0;
		for (auto& v : *this) { len += med::get_length<FIELD>(v); }
		return len;
	}

	//NOTE: clear won't return items allocated from external storage, use reset there
	void clear()                                            { m_count = 0; }
	bool is_set() const                                     { return m_count > 0 && m_items[0].value.is_set(); }

//	value_type const* get(std::size_t index) const    { return const_cast<mult_field_t*>(this)->ref_field(index); }
//	field_type* get(std::size_t index)                { return m_items[index].value; }

	//uses inplace storage only
	value_type* push_back()
	{
		if (count() < inplace)
		{
			auto* piv = m_items + count();
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
	value_type* push_back(CTX& ctx)
	{
		//try inplace 1st then external
		value_type* pv = push_back();
		if (!pv)
		{
			if (auto* piv = get_allocator_ptr(ctx)->template allocate<value_t>())
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

//std::enable_if_t<is_optional_v<IE>, bool>
//constexpr check_arity(std::size_t count)    { return 0 == count || count >= IE::min; }
//std::enable_if_t<!is_optional_v<IE>, bool>
//constexpr check_arity(std::size_t count)    { return count >= IE::min; }

	template <class ENCODER>
	bool encode(ENCODER& encoder) const
	{
		CODEC_TRACE("[%s]*[%zu..%zu]", name<FIELD>(), min, max);

		if (count() <= max)
		{
			if (count() >= min)
			{
				for (auto& field : *this)
				{
					if (!med::encode(encoder, field)) return false;
				}
				return true;
			}
			else
			{
				encoder(error::EXTRA_IE, name<FIELD>(), min, count());
			}
		}
		else
		{
			encoder(error::EXTRA_IE, name<FIELD>(), max, count());
		}
		return false;
	}


	template <class DECODER, class UNEXP>
	bool decode(DECODER& decoder, UNEXP& unexp)
	{
		CODEC_TRACE("[%s]*[%zu..%zu]", name<FIELD>(), min, max);

		if (count() <= max)
		{
			if (count() >= min)
			{
				for (auto& field : *this)
				{
					if (!med::decode(decoder, field, unexp)) return false;
				}
				return true;
			}
			else
			{
				decoder(error::MISSING_IE, name<FIELD>(), min, count());
			}
		}
		else
		{
			encoder(error::EXTRA_IE, name<FIELD>(), max, count());
		}
		return false;

//		if (auto* field = push_back(decoder))
//		{
//			return med::decode(decoder, *field, unexp);
//		}
	}

private:
	value_t      m_items[inplace];
	std::size_t  m_count {0};
	value_t*     m_tail {nullptr};
};

} //end: namespace detail

template <class FIELD, class MIN = void, class MAX = void, class INPLACE = void>
struct sequence_of;

//template <class FIELD>
//struct sequence_of<FIELD, void, void, void> : detail::seq_of<FIELD, 1, inf, 1> {};

template <class FIELD, std::size_t ARITY>
struct sequence_of<FIELD, arity<ARITY>, void, void> : detail::seq_of<FIELD, ARITY, ARITY, ARITY>
{
	static_assert(ARITY > 1, "ARITY SHOULD BE GREATER ONE");
};

template <class FIELD, std::size_t MIN>
struct sequence_of<FIELD, min<MIN>, void, void> : detail::seq_of<FIELD, MIN, inf, MIN>
{
	static_assert(MIN > 1, "MIN SHOULD BE GREATER ONE OR NOT SPECIFIED");
};

template <class FIELD, std::size_t MAX>
struct sequence_of<FIELD, max<MAX>, void, void> : detail::seq_of<FIELD, 1, MAX, 1>
{
	static_assert(MAX > 1, "MAX SHOULD BE GREATER ONE");
};

template <class FIELD, std::size_t MIN, std::size_t MAX>
struct sequence_of<FIELD, min<MIN>, max<MAX>, void> : detail::seq_of<FIELD, MIN, MAX, MIN>
{
	static_assert(MIN > 1, "MIN SHOULD BE GREATER ONE OR NOT SPECIFIED");
	static_assert(MIN <= MAX, "MIN SHOULD BE LESS OR EQUAL MAX");
};

template <class FIELD, std::size_t MAX, std::size_t MIN>
struct sequence_of<FIELD, max<MAX>, min<MIN>, void> : detail::seq_of<FIELD, MIN, MAX, MIN>
{
	static_assert(MIN > 1, "MIN SHOULD BE GREATER ONE OR NOT SPECIFIED");
	static_assert(MIN <= MAX, "MIN SHOULD BE LESS OR EQUAL MAX");
};

template <class FIELD, std::size_t MIN, std::size_t MAX, std::size_t INPLACE>
struct sequence_of<FIELD, min<MIN>, max<MAX>, inplace<INPLACE>> : detail::seq_of<FIELD, MIN, MAX, INPLACE>
{
	static_assert(MIN > 1, "MIN SHOULD BE GREATER ONE OR NOT SPECIFIED");
	static_assert(MIN <= MAX, "MIN SHOULD BE LESS OR EQUAL MAX");
	static_assert(INPLACE >= MIN, "INPLACE SHOULD BE GREATER OR EQUAL MIN");
	static_assert(INPLACE <= MAX, "MAX SHOULD BE GREATER OR EQUAL INPLACE");
};

template <class FIELD, std::size_t MAX, std::size_t MIN, std::size_t INPLACE>
struct sequence_of<FIELD, max<MAX>, min<MIN>, inplace<INPLACE>> : detail::seq_of<FIELD, MIN, MAX, INPLACE>
{
	static_assert(MIN > 1, "MIN SHOULD BE GREATER ONE OR NOT SPECIFIED");
	static_assert(MIN <= MAX, "MIN SHOULD BE LESS OR EQUAL MAX");
	static_assert(INPLACE >= MIN, "INPLACE SHOULD BE GREATER OR EQUAL MIN");
	static_assert(INPLACE <= MAX, "MAX SHOULD BE GREATER OR EQUAL INPLACE");
};

} //end: namespace med
