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

namespace sl {

template <class Enable, class... IES>
struct seqof_dec_imp;

template <class... IES>
using seqof_decoder = seqof_dec_imp<void, IES...>;


template <>
struct seqof_dec_imp<void>
{
	template <class TO, class FUNC, class UNEXP, class TAG>
	static bool decode(TO&, FUNC&&, UNEXP&, TAG&)               { return true; }
};


template <class Enable, class... IES>
struct seqof_enc_imp;
template <class... IES>
using seqof_encoder = seqof_enc_imp<void, IES...>;

template <>
struct seqof_enc_imp<void>
{
	template <class TO, class FUNC>
	static inline bool encode(TO&, FUNC&& func)                 { return true; }
};

/*
template <class FUNC, class FIELD>
inline int encode_mult(FUNC& func, FIELD const& ie)
{
	//CODEC_TRACE("%s *%zu", name<FIELD>(), N);
	int count = 0;
	for (auto& v : ie)
	{
		CODEC_TRACE("[%s]@%d", name<FIELD>(), count);
		if (!sl::encode<FIELD>(func, v, typename FIELD::ie_type{})) return -1;
		++count;
	}
	return count;
}

template <class FIELD, class... IES>
struct set_dec_imp<std::enable_if_t<has_mult_field<FIELD>::value>, FIELD, IES...>
{
	template <class TO, class FUNC, class UNEXP, class HEADER>
	static inline bool decode(TO& to, FUNC& func, UNEXP& unexp, HEADER const& header)
	{
		if (tag_type_t<FIELD>::match( get_tag(header) ))
		{
			FIELD& ie = static_cast<FIELD&>(to);
			CODEC_TRACE("[%s]*", name<typename FIELD::field_type>());
			if (auto* field = func.allocate(ie))
			{
				return med::decode(func, *field, unexp);
			}
			return false;
		}
		else
		{
			return set_decoder<IES...>::decode(to, func, unexp, header);
		}
	}

	template <class TO, class FUNC>
	static bool check(TO const& to, FUNC& func)
	{
		FIELD const& ie = static_cast<FIELD const&>(to);
		if (check_arity<FIELD>(field_count(ie)))
		{
			return set_decoder<IES...>::check(to, func);
		}
		else
		{
			func(error::MISSING_IE, name<typename FIELD::field_type>(), FIELD::min, field_count(ie));
		}
		return false;
	}
};

template <class FIELD, class... IES>
struct set_enc_imp<std::enable_if_t<has_mult_field<FIELD>::value>, FIELD, IES...>
{
	template <class HEADER, class TO, class FUNC>
	static inline bool encode(TO const& to, FUNC& func)
	{
		CODEC_TRACE("[%s]*%zu", name<typename FIELD::field_type>(), FIELD::max);

		FIELD const& ie = static_cast<FIELD const&>(to);
		int const count = set_encode<HEADER, FIELD::max>(func, ie);
		if (count < 0) return false;

		if (check_arity<FIELD>(count))
		{
			return set_encoder<IES...>::template encode<HEADER>(to, func);
		}
		else
		{
			func(error::MISSING_IE, name<typename FIELD::field_type>(), FIELD::min, count);
		}
		return false;
	}
};

template <class FIELD, class... IES>
struct seq_dec_imp<std::enable_if_t<
		has_multi_field_v<FIELD>
		&& has_tag_type_v<FIELD>
	>,
	FIELD, IES...>
{
	template <class TO, class FUNC, class UNEXP, class TAG>
	static inline bool decode(TO& to, FUNC&& func, UNEXP& unexp, TAG& vtag)
	{
		FIELD& ie = to;

		if (!vtag)
		{
			if (func.push_state())
			{
				if (auto const tag = decode_tag<typename FIELD::tag_type>(func))
				{
					vtag.set_encoded(tag.get_encoded());
					CODEC_TRACE("pop tag=%zx", vtag.get_encoded());
				}
				else
				{
					return false;
				}
			}
		}

		while (FIELD::tag_type::match(vtag.get_encoded()))
		{
			CODEC_TRACE("T=%zx[%s]*", vtag.get_encoded(), name<FIELD>());

			if (auto* field = func.allocate(ie))
			{
				if (!med::decode(func, *field, unexp)) return false;
			}
			else
			{
				return false;
			}

			if (func.push_state())
			{
				if (auto const tag = decode_tag<typename FIELD::tag_type>(func))
				{
					vtag.set_encoded(tag.get_encoded());
					CODEC_TRACE("pop tag=%zx", vtag.get_encoded());
				}
				else
				{
					vtag.clear();
				}
			}
			else
			{
				vtag.clear();
				break;
			}
		}

		if (check_arity<FIELD>(field_count(ie)))
		{
			if (!vtag)
			{
				func.pop_state(); //restore state
				func(error::SUCCESS); //clear error
			}
			return seq_decoder<IES...>::decode(to, func, unexp, vtag);
		}
		else
		{
			func(error::MISSING_IE, name<typename FIELD::field_type>(), FIELD::min, field_count(ie));
			return false;
		}
	}
};

//multi-instance field without tag, counter or count-getter
template <class FIELD, class... IES>
struct seq_dec_imp<std::enable_if_t<
		has_multi_field_v<FIELD> &&
		!has_tag_type_v<FIELD> &&
		!has_count_getter_v<FIELD> &&
		!is_counter_v<FIELD>
	>,
	FIELD, IES...>
{
	template <class TO, class FUNC, class UNEXP, class TAG>
	static inline bool decode(TO& to, FUNC&& func, UNEXP& unexp, TAG& vtag)
	{
		FIELD& ie = to;

		if (vtag)
		{
			CODEC_TRACE("discard tag=%zx", vtag.get_encoded());
			func.pop_state(); //restore state
			vtag.clear();
		}

		CODEC_TRACE("[%s]*[%zu..%zu]", name<FIELD>(), FIELD::min, FIELD::max);

		std::size_t count = 0;
		while (!func.eof() && count < FIELD::max)
		{
			if (auto* field = func.allocate(ie))
			{
				if (!sl::decode<FIELD>(func, *field, typename FIELD::ie_type{}, unexp)) return false;
			}
			else
			{
				return false;
			}
			++count;
		}

		if (check_arity<FIELD>(count))
		{
			return seq_decoder<IES...>::decode(to, func, unexp, vtag);
		}
		else
		{
			func(error::MISSING_IE, name<typename FIELD::field_type>(), FIELD::min, count);
			return false;
		}
	}
};


//multi-instance field without tag with count-getter
template <class FIELD, class... IES>
struct seq_dec_imp<
	std::enable_if_t<
		has_multi_field_v<FIELD> &&
		!has_tag_type_v<FIELD> &&
		has_count_getter_v<FIELD>
	>,
	FIELD, IES...
>
{
	template <class TO, class FUNC, class UNEXP, class TAG>
	static inline bool decode(TO& to, FUNC&& func, UNEXP& unexp, TAG& vtag)
	{
		CODEC_TRACE("[%s]...", name<FIELD>());
		FIELD& ie = to;

		if (vtag)
		{
			CODEC_TRACE("discard tag=%zx", vtag.get_encoded());
			func.pop_state(); //restore state
			vtag.clear();
		}

		std::size_t count = typename FIELD::count_getter{}(to);

		CODEC_TRACE("[%s]*%zu", name<FIELD>(), count);
		while (count--)
		{
			if (auto* field = func.allocate(ie))
			{
				if (!med::decode(func, *field, unexp)) return false;
			}
			else
			{
				return false;
			}
		}

		if (check_arity<FIELD>(field_count(ie)))
		{
			return seq_decoder<IES...>::decode(to, func, unexp, vtag);
		}
		else
		{
			func(error::MISSING_IE, name<typename FIELD::field_type>(), FIELD::min, field_count(ie));
			return false;
		}
	}
};

//multi-instance field without tag with counter
template <class FIELD, class... IES>
struct seq_dec_imp<
	std::enable_if_t<
		has_multi_field_v<FIELD> &&
		!has_tag_type_v<FIELD> &&
		is_counter_v<FIELD>
	>,
	FIELD, IES...
>
{
	template <class TO, class FUNC, class UNEXP, class TAG>
	static inline bool decode(TO& to, FUNC&& func, UNEXP& unexp, TAG& vtag)
	{
		CODEC_TRACE("[%s]...", name<FIELD>());
		FIELD& ie = to;

		if (vtag)
		{
			CODEC_TRACE("discard tag=%zx", vtag.get_encoded());
			func.pop_state(); //restore state
			vtag.clear();
		}

		typename FIELD::counter_type counter_ie;
		if (!med::decode(func, counter_ie)) return false;

		auto count = counter_ie.get_encoded();
		if (check_arity<FIELD>(count))
		{
			while (count--)
			{
				CODEC_TRACE("[%s]*%zu", name<FIELD>(), count);

				if (auto* field = func.allocate(ie))
				{
					if (!med::decode(func, *field, unexp)) return false;
				}
				else
				{
					return false;
				}
			}

			return seq_decoder<IES...>::decode(to, func, unexp, vtag);
		}
		else
		{
			func(error::MISSING_IE, name<typename FIELD::field_type>(), FIELD::min, count);
			return false;
		}
	}
};

//multi-field w/o counter
template <class FIELD, class... IES>
struct seq_enc_imp<std::enable_if_t<
		has_multi_field_v<FIELD> &&
		!is_counter_v<FIELD>
	>,
	FIELD, IES...>
{
	template <class TO, class FUNC>
	static inline bool encode(TO const& to, FUNC&& func)
	{
		FIELD const& ie = to;
		int const count = med::encode_multi(func, ie);
		if (count < 0) return false;

		if (check_arity<FIELD>(count))
		{
			return seq_encoder<IES...>::encode(to, func);
		}
		else
		{
			func(error::MISSING_IE, name<typename FIELD::field_type>(), FIELD::min, count);
			return false;
		}
	}
};

//multi-field w/o counter
template <class FIELD, class... IES>
struct seq_enc_imp<std::enable_if_t<
		has_multi_field_v<FIELD> &&
		!is_counter_v<FIELD>
	>,
	FIELD, IES...>
{
	template <class TO, class FUNC>
	static inline bool encode(TO const& to, FUNC&& func)
	{
		FIELD const& ie = to;
		int const count = med::encode_multi(func, ie);
		if (count < 0) return false;

		if (check_arity<FIELD>(count))
		{
			return seq_encoder<IES...>::encode(to, func);
		}
		else
		{
			func(error::MISSING_IE, name<typename FIELD::field_type>(), FIELD::min, count);
			return false;
		}
	}
};

//mandatory multi-field w/ counter
template <class FIELD, class... IES>
struct seq_enc_imp<std::enable_if_t<
		has_multi_field_v<FIELD> &&
		is_counter_v<FIELD> &&
		!is_optional_v<FIELD>
	>,
	FIELD, IES...>
{
	template <class TO, class FUNC>
	static inline bool encode(TO const& to, FUNC&& func)
	{
		FIELD const& ie = to;
		std::size_t const count = field_count(ie);
		CODEC_TRACE("CV[%s]=%zu", name<FIELD>(), count);
		if (check_arity<FIELD>(count))
		{
			typename FIELD::counter_type counter_ie;
			counter_ie.set_encoded(count);

			return med::encode(func, counter_ie)
				&& med::encode_multi(func, ie) == (int)count
				&& seq_encoder<IES...>::encode(to, func);
		}
		else
		{
			func(error::MISSING_IE, name<typename FIELD::field_type>(), FIELD::min, count);
		}
		return false;
	}
};

//optional multi-field w/ counter
template <class FIELD, class... IES>
struct seq_enc_imp<std::enable_if_t<
		has_multi_field_v<FIELD> &&
		is_counter_v<FIELD> &&
		is_optional_v<FIELD>
	>,
	FIELD, IES...>
{
	template <class TO, class FUNC>
	static inline bool encode(TO const& to, FUNC&& func)
	{
		FIELD const& ie = to;
		std::size_t const count = field_count(ie);
		CODEC_TRACE("CV[%s]=%zu", name<FIELD>(), count);
		if (check_arity<FIELD>(count))
		{
			if (count > 0)
			{
				typename FIELD::counter_type counter_ie;
				counter_ie.set_encoded(count);

				return med::encode(func, counter_ie)
					&& med::encode_multi(func, ie) == (int)count
					&& seq_encoder<IES...>::encode(to, func);
			}
			else
			{
				return seq_encoder<IES...>::encode(to, func);
			}
		}
		else
		{
			func(error::MISSING_IE, name<typename FIELD::field_type>(), FIELD::min, count);
		}
		return false;
	}
};

*/

} //end: namespace sl

namespace detail {

template <class FIELD, std::size_t MIN, std::size_t MAX, std::size_t INPLACE>
class seq_of : public IE<CONTAINER>
{
public:
	//using ie_type = typename FIELD::ie_type;
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

//	field_type const& ref_field(std::size_t index) const    { return const_cast<mult_field_t*>(this)->ref_field(index); }
//	field_type& ref_field(std::size_t index)                { return m_items[index].value; }

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
	template <class CTX>
	auto push_back(CTX& ctx) -> std::enable_if_t<!std::is_pointer<CTX>::value, value_type*>
	{
		auto* allocator = get_allocator_ptr(ctx);
		if (count() < inplace)
		{
			return push_back();
		}
		else if (count() < max)
		{
			if (auto* piv = allocator->template allocate<value_t>())
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

	template <class ENCODER>
	bool encode(ENCODER&& encoder) const
	{
		//return sl::seqof_encoder<IES...>::encode(this->m_ies, std::forward<ENCODER>(encoder));
		return false;
	}

	template <class DECODER, class UNEXP>
	bool decode(DECODER&& decoder, UNEXP& unexp)
	{
		//return sl::seqof_decoder<IES...>::decode(this->m_ies, std::forward<DECODER>(decoder), unexp);
		return false;
	}

private:
	value_t      m_items[inplace];
	std::size_t  m_count {0};
	value_t*     m_tail {nullptr};
};

} //end: namespace detail

template <class FIELD, class MIN = void, class MAX = void, class INPLACE = void>
struct sequence_of;

template <class FIELD>
struct sequence_of<FIELD, void, void, void> : detail::seq_of<FIELD, 0, inf, 1> {};

template <class FIELD, std::size_t ARITY>
struct sequence_of<FIELD, arity<ARITY>, void, void> : detail::seq_of<FIELD, ARITY, ARITY, ARITY>
{
	static_assert(ARITY > 0, "ARITY SHOULD BE GREATER ZERO");
};

template <class FIELD, std::size_t MIN>
struct sequence_of<FIELD, min<MIN>, void, void> : detail::seq_of<FIELD, MIN, inf, MIN>
{
	static_assert(MIN > 0, "MIN SHOULD BE GREATER ZERO OR NOT SPECIFIED");
};

template <class FIELD, std::size_t MAX>
struct sequence_of<FIELD, max<MAX>, void, void> : detail::seq_of<FIELD, 0, MAX, 1>
{
	static_assert(MAX > 0, "MAX SHOULD BE GREATER ZERO");
};

template <class FIELD, std::size_t MIN, std::size_t MAX>
struct sequence_of<FIELD, min<MIN>, max<MAX>, void> : detail::seq_of<FIELD, MIN, MAX, MIN>
{
	static_assert(MIN > 0, "MIN SHOULD BE GREATER ZERO OR NOT SPECIFIED");
	static_assert(MIN <= MAX, "MIN SHOULD BE LESS OR EQUAL MAX");
};

template <class FIELD, std::size_t MAX, std::size_t MIN>
struct sequence_of<FIELD, max<MAX>, min<MIN>, void> : detail::seq_of<FIELD, MIN, MAX, MIN>
{
	static_assert(MIN > 0, "MIN SHOULD BE GREATER ZERO OR NOT SPECIFIED");
	static_assert(MIN <= MAX, "MIN SHOULD BE LESS OR EQUAL MAX");
};

template <class FIELD, std::size_t MIN, std::size_t MAX, std::size_t INPLACE>
struct sequence_of<FIELD, min<MIN>, max<MAX>, inplace<INPLACE>> : detail::seq_of<FIELD, MIN, MAX, INPLACE>
{
	static_assert(MIN > 0, "MIN SHOULD BE GREATER ZERO OR NOT SPECIFIED");
	static_assert(MIN <= MAX, "MIN SHOULD BE LESS OR EQUAL MAX");
	static_assert(INPLACE >= MIN, "INPLACE SHOULD BE GREATER OR EQUAL MIN");
	static_assert(INPLACE <= MAX, "MAX SHOULD BE GREATER OR EQUAL INPLACE");
};

template <class FIELD, std::size_t MAX, std::size_t MIN, std::size_t INPLACE>
struct sequence_of<FIELD, max<MAX>, min<MIN>, inplace<INPLACE>> : detail::seq_of<FIELD, MIN, MAX, INPLACE>
{
	static_assert(MIN > 0, "MIN SHOULD BE GREATER ZERO OR NOT SPECIFIED");
	static_assert(MIN <= MAX, "MIN SHOULD BE LESS OR EQUAL MAX");
	static_assert(INPLACE >= MIN, "INPLACE SHOULD BE GREATER OR EQUAL MIN");
	static_assert(INPLACE <= MAX, "MAX SHOULD BE GREATER OR EQUAL INPLACE");
};

} //end: namespace med
