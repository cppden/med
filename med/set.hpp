/**
@file
set IE container - tagged elements in any order

@copyright Denis Priyomov 2016-2017
Distributed under the MIT License
(See accompanying file LICENSE or visit https://github.com/cppden/med)
*/

#pragma once

#include "config.hpp"
#include "error.hpp"
#include "optional.hpp"
#include "container.hpp"
#include "encode.hpp"
#include "decode.hpp"
#include "name.hpp"
#include "tag.hpp"
#include "debug.hpp"
#include "util/unique.hpp"

namespace med {

namespace sl {

template <class HEADER, class IE, class FUNC>
constexpr MED_RESULT encode_header(FUNC& func)
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
	
	MED_RETURN_SUCCESS;
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
	static inline MED_RESULT encode(TO const& to, FUNC& func)
	{
		if constexpr (FUNC::encoder_type == codec_type::CONTAINER && not std::is_void_v<PREV_IE>)
		{
			MED_CHECK_FAIL(func(to, NEXT_CONTAINER_ELEMENT{}));
		}
		if constexpr (is_multi_field<IE>::value)
		{
			IE const& ie = static_cast<IE const&>(to);
			CODEC_TRACE("[%s]*%zu", name<IE>(), ie.count());

			MED_CHECK_FAIL(check_arity(func, ie));
			for (auto& field : ie)
			{
				//TODO: actually this field was pushed but not set... do we need a new error?
				if (field.is_set())
				{
					MED_CHECK_FAIL((encode_header<HEADER, IE>(func)));
					MED_CHECK_FAIL(med::encode(func, field));
				}
				else
				{
					return func(error::MISSING_IE, name<IE>(), ie.count(), ie.count()-1);
				}
			}
		}
		else //single-instance field
		{
			IE const& ie = static_cast<IE const&>(to);
			if (ie.ref_field().is_set())
			{
				CODEC_TRACE("[%s]", name<IE>());
				MED_CHECK_FAIL((encode_header<HEADER, IE>(func)));
				MED_CHECK_FAIL(med::encode(func, ie.ref_field()));
			}
			else if constexpr (!is_optional_v<IE>)
			{
				return func(error::MISSING_IE, name<IE>(), 1, 0);
			}
		}
		return set_for<IEs...>::template encode<IE, HEADER>(to, func);
	}

	template <class TO, class FUNC, class UNEXP, class HEADER>
	static inline MED_RESULT decode(TO& to, FUNC& func, UNEXP& unexp, HEADER const& header)
	{
		if constexpr (is_multi_field<IE>::value)
		{
			if (tag_type_t<IE>::match( get_tag(header) ))
			{
				//pop back the tag read since we have non-fixed tag inside
				if constexpr (not tag_type_t<IE>::is_const) { func(POP_STATE{}); }

				IE& ie = static_cast<IE&>(to);
				CODEC_TRACE("[%s]@%zu", name<IE>(), ie.count());
				if (ie.count() >= IE::max)
				{
					return func(error::EXTRA_IE, name<IE>(), IE::max, ie.count());
				}
				auto* field = ie.push_back(func);
				return MED_EXPR_AND(field) med::decode(func, *field, unexp);
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
				return func(error::EXTRA_IE, name<IE>(), 1);
			}
		}
		return set_for<IEs...>::decode(to, func, unexp, header);
	}

	template <class TO, class FUNC>
	static MED_RESULT check(TO const& to, FUNC& func)
	{
		if constexpr (is_multi_field<IE>::value)
		{
			IE const& ie = static_cast<IE const&>(to);
			MED_CHECK_FAIL(check_arity(func, ie));
		}
		else //single-instance field
		{
			IE const& ie = static_cast<IE const&>(to);
			if (not (is_optional_v<IE> || ie.ref_field().is_set()))
			{
				return func(error::MISSING_IE, name<IE>(), 1, 0);
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
	static constexpr MED_RESULT encode(TO&, FUNC&)       { MED_RETURN_SUCCESS; }

	template <class TO, class FUNC, class UNEXP, class HEADER>
	static MED_RESULT decode(TO& to, FUNC& func, UNEXP& unexp, HEADER const& header)
	{
		return unexp(func, to, header);
	}

	template <class TO, class FUNC>
	static constexpr MED_RESULT check(TO&, FUNC&)       { MED_RETURN_SUCCESS; }
};

}	//end: namespace sl

template <class HEADER, class ...IEs>
struct set : container<IEs...>
{
	static constexpr bool ordered = false; //used only in JSON, smth more useful?

	using header_type = HEADER;

	template <typename TAG>
	static constexpr char const* name_tag(TAG const& tag)
	{
		return sl::set_for<IEs...>::name_tag(tag);
	}

	template <class ENCODER>
	MED_RESULT encode(ENCODER& encoder) const
	{
		return sl::set_for<IEs...>::template encode<void, header_type>(this->m_ies, encoder);
	}

	template <class DECODER, class UNEXP>
	MED_RESULT decode(DECODER& decoder, UNEXP& unexp)
	{
		static_assert(util::are_unique(tag_value_get<IEs>::value...), "TAGS ARE NOT UNIQUE");

		while (decoder(PUSH_STATE{}))
		{
			header_type header;
#if (MED_EXCEPTIONS)
			//TODO: avoid try/catch
			try
			{
				med::decode(decoder, header, unexp);
			}
			catch (med::overflow const& ex)
			{
				decoder(POP_STATE{});
				decoder(error::SUCCESS);
				break;
			}
			sl::pop_state<header_type>(decoder);
			CODEC_TRACE("tag=%#zx", std::size_t(get_tag(header)));
			MED_CHECK_FAIL(sl::set_for<IEs...>::decode(this->m_ies, decoder, unexp, header));
#else
			if (med::decode(decoder, header, unexp))
			{
				sl::pop_state<header_type>(decoder);
				CODEC_TRACE("tag=%#zx", std::size_t(get_tag(header)));
				MED_CHECK_FAIL(sl::set_for<IEs...>::decode(this->m_ies, decoder, unexp, header));
			}
			else
			{
				decoder(POP_STATE{});
				decoder(error::SUCCESS);
				break;
			}
#endif //MED_EXCEPTIONS
		}

		return sl::set_for<IEs...>::check(this->m_ies, decoder);
	}
};

}	//end: namespace med
