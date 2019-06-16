/**
@file
set IE container - tagged elements in any order

@copyright Denis Priyomov 2016-2017
Distributed under the MIT License
(See accompanying file LICENSE or visit https://github.com/cppden/med)
*/

#pragma once

#include "config.hpp"
#include "exception.hpp"
#include "optional.hpp"
#include "container.hpp"
#include "encode.hpp"
#include "decode.hpp"
#include "name.hpp"
#include "tag.hpp"
#include "meta/unique.hpp"

namespace med {

namespace sl {

template <class HEADER, class IE, class FUNC>
constexpr void encode_header(FUNC& func)
{
	if constexpr (std::is_base_of_v<PRIMITIVE, typename HEADER::ie_type>)
	{
		using tag_type = tag_type_t<IE>;
		//static_assert(HEADER::traits::bits == tag_type::traits::bits);
		if constexpr (tag_type::is_const) //encode only if tag fixed
		{
			return encode(func, tag_type{});
		}
	}
	else
	{
		static_assert(std::is_base_of_v<CONTAINER, typename HEADER::ie_type>, "unexpected aggregate?");
	}
}

template <class T, class FUNC>
constexpr void pop_state(FUNC&& func)
{
	if constexpr (std::is_base_of_v<CONTAINER, typename T::ie_type>)
	{
		func(POP_STATE{});
	}
}

template <class... IEs>
struct set_for;

template <class IE, class... IEs>
struct set_for<IE, IEs...>
{
	template <typename TAG>
	static constexpr char const* name_tag(TAG const& tag)
	{
		if (tag_type_t<IE>::match(tag))
		{
			return name<IE>();
		}
		else
		{
			return set_for<IEs...>::name_tag(tag);
		}
	}

	template <class PREV_IE, class HEADER, class TO, class FUNC>
	static inline void encode(TO const& to, FUNC& func)
	{
		call_if<not std::is_void_v<PREV_IE>
				&& is_callable_with_v<FUNC, NEXT_CONTAINER_ELEMENT>
			>::call(func, NEXT_CONTAINER_ELEMENT{}, to);

		if constexpr (is_multi_field_v<IE>)
		{
			IE const& ie = static_cast<IE const&>(to);
			CODEC_TRACE("[%s]*%zu", name<IE>(), ie.count());

			check_arity(func, ie);
			//TODO: ugly indeed :(
			constexpr bool do_hdr = is_callable_with_v<FUNC, HEADER_CONTAINER>;
			if constexpr (do_hdr)
			{
				encode_header<HEADER, IE>(func);
				func(HEADER_CONTAINER{}, ie);
			}

			call_if<is_callable_with_v<FUNC, ENTRY_CONTAINER>>::call(func, ENTRY_CONTAINER{}, ie);
			bool first = true;
			for (auto& field : ie)
			{
				//field was pushed but not set... do we need a new error?
				if (not field.is_set()) { MED_THROW_EXCEPTION(missing_ie, name<IE>(), ie.count(), ie.count()-1); }
				if (not first) { call_if<is_callable_with_v<FUNC, NEXT_CONTAINER_ELEMENT>>::call(func, NEXT_CONTAINER_ELEMENT{}, to); }
				if constexpr (not do_hdr) { encode_header<HEADER, IE>(func); }
				med::encode(func, field);
				first = false;
			}
			call_if<is_callable_with_v<FUNC, EXIT_CONTAINER>>::call(func, EXIT_CONTAINER{}, ie);
		}
		else //single-instance field
		{
			IE const& ie = static_cast<IE const&>(to);
			if (ie.ref_field().is_set())
			{
				CODEC_TRACE("[%s]", name<IE>());
				encode_header<HEADER, IE>(func);
				call_if<is_callable_with_v<FUNC, HEADER_CONTAINER>>::call(func, HEADER_CONTAINER{}, to);
				med::encode(func, ie.ref_field());
			}
			else if constexpr (!is_optional_v<IE>)
			{
				MED_THROW_EXCEPTION(missing_ie, name<IE>(), 1, 0);
			}
		}
		return set_for<IEs...>::template encode<IE, HEADER>(to, func);
	}

	template <class TO, class FUNC, class UNEXP, class HEADER>
	static inline void decode(TO& to, FUNC& func, UNEXP& unexp, HEADER const& header)
	{
		if constexpr (is_multi_field_v<IE>)
		{
			if (tag_type_t<IE>::match( get_tag(header) ))
			{
				//pop back the tag read since we have non-fixed tag inside
				if constexpr (not tag_type_t<IE>::is_const) { func(POP_STATE{}); }

				IE& ie = static_cast<IE&>(to);
				CODEC_TRACE("[%s]@%zu", name<IE>(), ie.count());
				//TODO: looks strange since it doesn't match encode's calls
				//call_if<is_callable_with_v<FUNC, HEADER_CONTAINER>>::call(func, HEADER_CONTAINER{}, ie);
				call_if<is_callable_with_v<FUNC, ENTRY_CONTAINER>>::call(func, ENTRY_CONTAINER{}, ie);

				if constexpr (is_callable_with_v<FUNC, NEXT_CONTAINER_ELEMENT>)
				{
					while (func(PUSH_STATE{}, ie))
					{
						if (auto const num = ie.count())
						{
							func(NEXT_CONTAINER_ELEMENT{}, ie);
							if (num >= IE::max)
							{
								MED_THROW_EXCEPTION(extra_ie, name<IE>(), IE::max, num);
							}
						}
						auto* field = ie.push_back(func);
						med::decode(func, *field, unexp);
					}
				}
				else
				{
					if (ie.count() >= IE::max)
					{
						MED_THROW_EXCEPTION(extra_ie, name<IE>(), IE::max, ie.count());
					}
					auto* field = ie.push_back(func);
					med::decode(func, *field, unexp);
				}

				call_if<is_callable_with_v<FUNC, EXIT_CONTAINER>>::call(func, EXIT_CONTAINER{}, ie);
				return;
			}
		}
		else //single-instance field
		{
			if (tag_type_t<IE>::match( get_tag(header) ))
			{
				//pop back the tag read since we have non-fixed tag inside
				if constexpr (not tag_type_t<IE>::is_const) { func(POP_STATE{}); }

				IE& ie = static_cast<IE&>(to);
				CODEC_TRACE("[%s] = %u", name<IE>(), ie.ref_field().is_set());
				if (not ie.ref_field().is_set())
				{
					return med::decode(func, ie.ref_field(), unexp);
				}
				MED_THROW_EXCEPTION(extra_ie, name<IE>(), 2, 1);
			}
			else
			{
				CODEC_TRACE("not [%s] = %#zx", name<IE>(), std::size_t(tag_type_t<IE>::get()));
			}
		}
		return set_for<IEs...>::decode(to, func, unexp, header);
	}

	template <class TO, class FUNC>
	static void check(TO const& to, FUNC& func)
	{
		if constexpr (is_multi_field_v<IE>)
		{
			IE const& ie = static_cast<IE const&>(to);
			check_arity(func, ie);
		}
		else //single-instance field
		{
			IE const& ie = static_cast<IE const&>(to);
			if (not (is_optional_v<IE> || ie.ref_field().is_set()))
			{
				MED_THROW_EXCEPTION(missing_ie, name<IE>(), 1, 0);
			}
		}
		return set_for<IEs...>::check(to, func);
	}
};

template <>
struct set_for<>
{
	template <typename TAG>
	static constexpr char const* name_tag(TAG const&)    { return nullptr; }

	template <class PREV_IE, class HEADER, class TO, class FUNC>
	static constexpr void encode(TO&, FUNC&)             { }

	template <class TO, class FUNC, class UNEXP, class HEADER>
	static void decode(TO& to, FUNC& func, UNEXP& unexp, HEADER const& header)
	{
		return unexp(func, to, header);
	}

	template <class TO, class FUNC>
	static constexpr void check(TO&, FUNC&)              { }
};

}	//end: namespace sl

template <class TRAITS, class HEADER, class ...IEs>
struct base_set : container<TRAITS, IEs...>
{
	using header_type = HEADER;
	using ies_types = typename container<TRAITS, IEs...>::ies_types;

	template <typename TAG>
	static constexpr char const* name_tag(TAG const& tag)
	{
		return sl::set_for<IEs...>::name_tag(tag);
	}

	template <class ENCODER>
	void encode(ENCODER& encoder) const
	{
		return sl::set_for<IEs...>::template encode<void, header_type>(this->m_ies, encoder);
	}

	template <class FUNC, class UNEXP>
	void decode(FUNC& func, UNEXP& unexp)
	{
		static_assert(std::is_void_v<meta::tag_unique_t<ies_types>>, "SEE ERROR ON INCOMPLETE TYPE/UNDEFINED TEMPLATE HOLDING IEs WITH CLASHED TAGS");

		//TODO: how to join 2 branches w/o having unused bool
		bool first = true;
		while (func(PUSH_STATE{}, *this)) //NOTE: this usage doesn't address IE which corresponds to PUSH_STATE
		{
			if (not first) { call_if<is_callable_with_v<FUNC, NEXT_CONTAINER_ELEMENT>>::call(func, NEXT_CONTAINER_ELEMENT{}, *this); }
			header_type header;
			med::decode(func, header, unexp);
			sl::pop_state<header_type>(func);
			CODEC_TRACE("tag=%#zx", std::size_t(get_tag(header)));
			call_if<is_callable_with_v<FUNC, HEADER_CONTAINER>>::call(func, HEADER_CONTAINER{}, *this);
			sl::set_for<IEs...>::decode(this->m_ies, func, unexp, header);
			first = false;
		}
		return sl::set_for<IEs...>::check(this->m_ies, func);
	}
};

template <class HEADER, class ...IEs>
using set = base_set<base_traits, HEADER, IEs...>;

}	//end: namespace med
