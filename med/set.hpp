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

template <class HEADER, class IE, class ENCODER>
constexpr void encode_header(ENCODER& encoder)
{
	if constexpr (std::is_base_of_v<PRIMITIVE, typename HEADER::ie_type>)
	{
		using tag_type = med::meta::unwrap_t<decltype(ENCODER::template get_tag_type<IE>())>;
		//static_assert(HEADER::traits::bits == tag_type::traits::bits);
		if constexpr (tag_type::is_const) //encode only if tag fixed
		{
			return encode(encoder, tag_type{});
		}
	}
	else
	{
		static_assert(std::is_base_of_v<CONTAINER, typename HEADER::ie_type>, "unexpected aggregate?");
	}
}

template <class T, class DECODER>
constexpr void pop_state(DECODER& decoder)
{
	if constexpr (std::is_base_of_v<CONTAINER, typename T::ie_type>)
	{
		decoder(POP_STATE{});
	}
}

struct set_name
{
	template <class IE, typename TAG, class CODEC>
	static constexpr bool check(TAG const& tag, CODEC&)
	{
		using tag_type = meta::unwrap_t<decltype(CODEC::template get_tag_type<IE>())>;
		return tag_type::match(tag);
	}

	template <class IE, typename TAG, class CODEC>
	static constexpr char const* apply(TAG const&, CODEC&)
	{
		return name<IE>();
	}

	template <typename TAG, class CODEC>
	static constexpr char const* apply(TAG const&, CODEC&)
	{
		return nullptr;
	}
};

template <class HEADER>
struct set_enc
{
	using header_type = HEADER;
	template <class PREV_IE, class IE, class TO, class ENCODER>
	static constexpr void apply(TO const& to, ENCODER& encoder)
	{
		call_if<not std::is_void_v<PREV_IE>
				&& is_callable_with_v<ENCODER, NEXT_CONTAINER_ELEMENT>
			>::call(encoder, NEXT_CONTAINER_ELEMENT{}, to);

		if constexpr (is_multi_field_v<IE>)
		{
			IE const& ie = static_cast<IE const&>(to);
			CODEC_TRACE("[%s]*%zu", name<IE>(), ie.count());

			check_arity(encoder, ie);
			//TODO: ugly indeed :(
			constexpr bool do_hdr = is_callable_with_v<ENCODER, HEADER_CONTAINER>;
			if constexpr (do_hdr)
			{
				encode_header<header_type, IE>(encoder);
				encoder(HEADER_CONTAINER{}, ie);
			}

			call_if<is_callable_with_v<ENCODER, ENTRY_CONTAINER>>::call(encoder, ENTRY_CONTAINER{}, ie);
			bool first = true;
			for (auto& field : ie)
			{
				//field was pushed but not set... do we need a new error?
				if (not field.is_set()) { MED_THROW_EXCEPTION(missing_ie, name<IE>(), ie.count(), ie.count()-1) }
				if (not first) { call_if<is_callable_with_v<ENCODER, NEXT_CONTAINER_ELEMENT>>::call(encoder, NEXT_CONTAINER_ELEMENT{}, to); }
				if constexpr (not do_hdr) { encode_header<header_type, IE>(encoder); }
				med::encode(encoder, field);
				first = false;
			}
			call_if<is_callable_with_v<ENCODER, EXIT_CONTAINER>>::call(encoder, EXIT_CONTAINER{}, ie);
		}
		else //single-instance field
		{
			IE const& ie = static_cast<IE const&>(to);
			if (ie.ref_field().is_set())
			{
				CODEC_TRACE("[%s]", name<IE>());
				encode_header<header_type, IE>(encoder);
				call_if<is_callable_with_v<ENCODER, HEADER_CONTAINER>>::call(encoder, HEADER_CONTAINER{}, to);
				med::encode(encoder, ie.ref_field());
			}
			else if constexpr (!is_optional_v<IE>)
			{
				MED_THROW_EXCEPTION(missing_ie, name<IE>(), 1, 0)
			}
		}
	}

	template <class PREV_IE, class TO, class ENCODER>
	static constexpr void apply(TO const&, ENCODER&) {}
};

struct set_dec
{
	template <class IE, class TO, class DECODER, class UNEXP, class HEADER>
	static bool check(TO&, DECODER&, UNEXP&, HEADER const& header)
	{
		using tag_type = med::meta::unwrap_t<decltype(DECODER::template get_tag_type<IE>())>;
		return tag_type::match( get_tag(header) );
	}

	template <class IE, class TO, class DECODER, class UNEXP, class HEADER>
	static constexpr void apply(TO& to, DECODER& decoder, UNEXP& unexp, HEADER const&)
	{
		//pop back the tag read since we have non-fixed tag inside
		using tag_type = med::meta::unwrap_t<decltype(DECODER::template get_tag_type<IE>())>;
		if constexpr (not tag_type::is_const) { decoder(POP_STATE{}); }

		IE& ie = static_cast<IE&>(to);
		if constexpr (is_multi_field_v<IE>)
		{
			CODEC_TRACE("[%s]@%zu", name<IE>(), ie.count());
			//TODO: looks strange since it doesn't match encode's calls
			//call_if<is_callable_with_v<DECODER, HEADER_CONTAINER>>::call(decoder, HEADER_CONTAINER{}, ie);
			call_if<is_callable_with_v<DECODER, ENTRY_CONTAINER>>::call(decoder, ENTRY_CONTAINER{}, ie);

			if constexpr (is_callable_with_v<DECODER, NEXT_CONTAINER_ELEMENT>)
			{
				while (decoder(PUSH_STATE{}, ie))
				{
					if (auto const num = ie.count())
					{
						decoder(NEXT_CONTAINER_ELEMENT{}, ie);
						if (num >= IE::max)
						{
							MED_THROW_EXCEPTION(extra_ie, name<IE>(), IE::max, num)
						}
					}
					auto* field = ie.push_back(decoder);
					med::decode(decoder, *field, unexp);
				}
			}
			else
			{
				if (ie.count() >= IE::max)
				{
					MED_THROW_EXCEPTION(extra_ie, name<IE>(), IE::max, ie.count())
				}
				auto* field = ie.push_back(decoder);
				med::decode(decoder, *field, unexp);
			}

			call_if<is_callable_with_v<DECODER, EXIT_CONTAINER>>::call(decoder, EXIT_CONTAINER{}, ie);
		}
		else //single-instance field
		{
			CODEC_TRACE("[%s] = %u", name<IE>(), ie.ref_field().is_set());
			if (not ie.ref_field().is_set())
			{
				return med::decode(decoder, ie.ref_field(), unexp);
			}
			MED_THROW_EXCEPTION(extra_ie, name<IE>(), 2, 1)
		}
	}

	template <class TO, class DECODER, class UNEXP, class HEADER>
	//static constexpr void apply(TO&, DECODER&, UNEXP&, HEADER const&)
	static constexpr void apply(TO& to, DECODER& decoder, UNEXP& unexp, HEADER const& header)
	{
		unexp(decoder, to, header);
	}
};

struct set_check
{
	template <class IE, class TO, class DECODER>
	static constexpr void apply(TO const& to, DECODER& decoder)
	{
		IE const& ie = static_cast<IE const&>(to);
		if constexpr (is_multi_field_v<IE>)
		{
			check_arity(decoder, ie);
		}
		else //single-instance field
		{
			if (not (is_optional_v<IE> || ie.ref_field().is_set()))
			{
				MED_THROW_EXCEPTION(missing_ie, name<IE>(), 1, 0)
			}
		}
	}

	template <class TO, class DECODER>
	static constexpr void apply(TO const&, DECODER&) {}
};

}	//end: namespace sl

template <class TRAITS, class HEADER, class ...IEs>
struct base_set : container<TRAITS, IEs...>
{
	using header_type = HEADER;
	using ies_types = typename container<TRAITS, IEs...>::ies_types;

	template <typename TAG, class CODEC>
	static constexpr char const* name_tag(TAG const& tag, CODEC& codec)
	{
		return meta::for_if<ies_types>(sl::set_name{}, tag, codec);
	}

	template <class ENCODER>
	void encode(ENCODER& encoder) const
	{
		meta::foreach_prev<ies_types>(sl::set_enc<header_type>{}, this->m_ies, encoder);
	}

	template <class DECODER, class UNEXP>
	void decode(DECODER& decoder, UNEXP& unexp)
	{
		static_assert(std::is_void_v<meta::tag_unique_t<meta::tag_getter<DECODER>, ies_types>>
			, "SEE ERROR ON INCOMPLETE TYPE/UNDEFINED TEMPLATE HOLDING IEs WITH CLASHED TAGS");

		//TODO: how to join 2 branches w/o having unused bool
		bool first = true;
		while (decoder(PUSH_STATE{}, *this)) //NOTE: this usage doesn't address IE which corresponds to PUSH_STATE
		{
			if (not first)
			{
				call_if<is_callable_with_v<DECODER, NEXT_CONTAINER_ELEMENT>>::call(
					decoder, NEXT_CONTAINER_ELEMENT{}, *this);
			}
			header_type header;
			med::decode(decoder, header, unexp);
			sl::pop_state<header_type>(decoder);
			CODEC_TRACE("tag=%#zx", std::size_t(get_tag(header)));
			call_if<is_callable_with_v<DECODER, HEADER_CONTAINER>>::call(decoder, HEADER_CONTAINER{}, *this);

			//meta::foreach<ies_types>(sl::set_dec{}, this->m_ies, decoder, unexp, header);
			meta::for_if<ies_types>(sl::set_dec{}, this->m_ies, decoder, unexp, header);

			first = false;
		}
		meta::foreach<ies_types>(sl::set_check{}, this->m_ies, decoder);
	}
};

template <class HEADER, class ...IEs>
using set = base_set<base_traits, HEADER, IEs...>;

}	//end: namespace med
