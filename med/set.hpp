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

struct set_name
{
	template <class IE, typename TAG, class CODEC>
	static constexpr bool check(TAG const& tag, CODEC&)
	{
		using mi = meta::produce_info_t<CODEC, IE>;
		using tag_t = typename meta::list_first_t<mi>::info_type;
		return tag_t::match(tag);
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

struct set_enc
{
	template <class PREV_IE, class IE, class TO, class ENCODER>
	static constexpr void apply(TO const& to, ENCODER& encoder)
	{
		using mi = meta::produce_info_t<ENCODER, IE>;
		IE const& ie = to;
		constexpr bool explicit_meta = explicit_meta_in<mi, get_field_type_t<IE>>();

		if constexpr (AMultiField<IE>)
		{
			CODEC_TRACE("[%s]*%zu: %s", name<IE>(), ie.count(), class_name<mi>());
			check_arity(encoder, ie);

			for (auto& field : ie)
			{
				//field was pushed but not set... do we need a new error?
				if (not field.is_set()) { MED_THROW_EXCEPTION(missing_ie, name<IE>(), ie.count(), ie.count()-1) }

				if constexpr (explicit_meta)
				{
					sl::ie_encode<meta::list_rest_t<mi>, void>(encoder, field);
				}
				else
				{
					sl::ie_encode<mi, void>(encoder, field);
				}
			}
		}
		else //single-instance field
		{
			if (ie.is_set())
			{
				CODEC_TRACE("[%s]%s: %s", name<IE>(), class_name<IE>(), class_name<mi>());
				if constexpr (explicit_meta)
				{
					sl::ie_encode<meta::list_rest_t<mi>, void>(encoder, ie);
				}
				else
				{
					sl::ie_encode<mi, void>(encoder, ie);
				}
			}
			else if constexpr (!AOptional<IE>)
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
	template <class IE, class TO, class DECODER, class HEADER, class... DEPS>
	static bool check(TO&, DECODER&, HEADER const& header, DEPS&...)
	{
		using mi = meta::produce_info_t<DECODER, IE>;
		using tag_t = typename meta::list_first_t<mi>::info_type;
		return tag_t::match( get_tag(header) );
	}

	template <class IE, class TO, class DECODER, class HEADER, class... DEPS>
	static constexpr void apply(TO& to, DECODER& decoder, HEADER const&, DEPS&... deps)
	{
		using mi = meta::produce_info_t<DECODER, IE>;
		//pop back the tag we've read as we have non-fixed tag inside
		using tag_t = typename meta::list_first_t<mi>::info_type;
		if constexpr (not tag_t::is_const) { decoder(POP_STATE{}); }

		IE& ie = to;
		if constexpr (AMultiField<IE>)
		{
			CODEC_TRACE("[%s]*%zu", name<IE>(), ie.count());
			if (ie.count() >= IE::max)
			{
				MED_THROW_EXCEPTION(extra_ie, name<IE>(), IE::max, ie.count())
			}
			auto* field = ie.push_back(decoder);
			sl::ie_decode<meta::list_rest_t<mi>, void>(decoder, *field, deps...);
		}
		else //single-instance field
		{
			CODEC_TRACE("%c[%s]", ie.is_set()?'+':'-', name<IE>());
			if (not ie.is_set())
			{
				return sl::ie_decode<meta::list_rest_t<mi>, void>(decoder, ie, deps...);
				//return med::decode(decoder, ie);
			}
			MED_THROW_EXCEPTION(extra_ie, name<IE>(), 2, 1)
		}
	}

	template <class TO, class DECODER, class HEADER, class... DEPS>
	static constexpr void apply(TO&, DECODER&, HEADER const& header, DEPS&...)
	{
		MED_THROW_EXCEPTION(unknown_tag, name<TO>(), get_tag(header))
	}
};

struct set_check
{
	template <class IE, class TO, class DECODER>
	static constexpr void apply(TO const& to, DECODER& decoder)
	{
		IE const& ie = to;
		if constexpr (AMultiField<IE>)
		{
			check_arity(decoder, ie);
		}
		else //single-instance field
		{
			if (not (AOptional<IE> || ie.is_set()))
			{
				MED_THROW_EXCEPTION(missing_ie, name<IE>(), 1, 0)
			}
		}
	}

	template <class TO, class DECODER>
	static constexpr void apply(TO const&, DECODER&) {}
};

}	//end: namespace sl

namespace detail {


template <class L> struct set_container;
template <template <class...> class L, AHasGetTag HEADER, class... IEs>
struct set_container<L<HEADER, IEs...>> : container<IEs...>
{
	using header_type = HEADER;
	static constexpr bool plain_header = false; //compound header e.g. a sequence
};
template <template <class...> class L, class IE1, class... IEs>
struct set_container<L<IE1, IEs...>> : container<IE1, IEs...>
{
	static constexpr bool plain_header = true; //plain header e.g. a tag
};

} //end: namespace detail

template <class... IEs>
struct set : detail::set_container<meta::typelist<IEs...>>
{
	using ies_types = typename detail::set_container<meta::typelist<IEs...>>::ies_types;

	template <typename TAG, class CODEC>
	static constexpr char const* name_tag(TAG const& tag, CODEC& codec)
	{
		return meta::for_if<ies_types>(sl::set_name{}, tag, codec);
	}

	template <class ENCODER>
	void encode(ENCODER& encoder) const
	{
		meta::foreach_prev<ies_types>(sl::set_enc{}, this->m_ies, encoder);
	}

	template <class DECODER, class... DEPS>
	void decode(DECODER& decoder, DEPS&... deps)
	{
		static_assert(std::is_void_v<meta::unique_t<tag_getter<DECODER>, ies_types>>
			, "SEE ERROR ON INCOMPLETE TYPE/UNDEFINED TEMPLATE HOLDING IEs WITH CLASHED TAGS");
		//?TODO: check all IEs have covariant tag

		if constexpr (set::plain_header)
		{
			using IE = meta::list_first_t<ies_types>; //use 1st IE since all have similar tag
			using mi = meta::produce_info_t<DECODER, IE>;
			using tag_t = typename meta::list_first_t<mi>::info_type;

			while (decoder(PUSH_STATE{}, *this))
			{
				value<std::size_t> header;
				header.set_encoded(sl::decode_tag<tag_t, false>(decoder));
				CODEC_TRACE("tag=%#zx", std::size_t(get_tag(header)));
				meta::for_if<ies_types>(sl::set_dec{}, this->m_ies, decoder, header, deps...);
			}
		}
		else //compound header
		{
			using header_type = typename detail::set_container<meta::typelist<IEs...>>::header_type;
			while (decoder(PUSH_STATE{}, *this))
			{
				header_type header;
				med::decode(decoder, header, deps...);
				decoder(POP_STATE{}); //restore back for IE to decode itself (?TODO: better to copy instead)
				CODEC_TRACE("tag=%#zx", std::size_t(get_tag(header)));
				meta::for_if<ies_types>(sl::set_dec{}, this->m_ies, decoder, header, deps...);
			}
		}
		meta::foreach<ies_types>(sl::set_check{}, this->m_ies, decoder);
	}
};

}	//end: namespace med
