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

template <class FUNC, class IE>
inline constexpr void encode_single(FUNC& func, IE const& ie)
{
	if (ie.is_set())
	{
		using mi = meta::produce_info_t<FUNC, IE>;
		constexpr bool explicit_meta = explicit_meta_in<mi, get_field_type_t<IE>>();

		CODEC_TRACE("[%s]%s: %s", name<IE>(), class_name<IE>(), class_name<mi>());
		if constexpr (explicit_meta)
		{
			using ctx = type_context<IE_SET, meta::list_rest_t<mi>>;
			sl::ie_encode<ctx>(func, ie);
		}
		else
		{
			using ctx = type_context<IE_SET, mi>;
			sl::ie_encode<ctx>(func, ie);
		}
	}
	else if constexpr (!AOptional<IE>)
	{
		MED_THROW_EXCEPTION(missing_ie, name<IE>(), 1, 0)
	}
}

struct set_name
{
	template <class IE, typename TAG, class CODEC>
	static constexpr bool check(TAG const& tag, CODEC&)
	{
		using mi = meta::produce_info_t<CODEC, IE>;
		using tag_t = get_info_t<meta::list_first_t<mi>>;
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
	template <class CTX, class PREV_IE, class IE, class TO, class ENCODER>
	static constexpr void apply(TO const& to, ENCODER& encoder)
	{
		IE const& ie = to;

		if constexpr (AMultiField<IE>)
		{
			using mi = meta::produce_info_t<ENCODER, IE>;
			constexpr bool explicit_meta = explicit_meta_in<mi, get_field_type_t<IE>>();

			CODEC_TRACE("[%s]*%zu: %s", name<IE>(), ie.count(), class_name<mi>());
			check_arity(encoder, ie);

			for (auto& field : ie)
			{
				//field was pushed but not set... do we need a new error?
				if (not field.is_set()) { MED_THROW_EXCEPTION(missing_ie, name<IE>(), ie.count(), ie.count()-1) }

				if constexpr (explicit_meta)
				{
					using ctx = type_context<IE_SET, meta::list_rest_t<mi>, get_info_t<meta::list_first_t<mi>>>;
					sl::ie_encode<ctx>(encoder, field);
				}
				else
				{
					using ctx = type_context<IE_SET, mi>;
					sl::ie_encode<ctx>(encoder, field);
				}
			}
		}
		else //single-instance field
		{
			if constexpr (AHasSetterType<IE>) //with setter
			{
				CODEC_TRACE("[%s] with setter from %s", name<IE>(), name<TO>());
				IE ie;
				ie.copy(static_cast<IE const&>(to), encoder);

				typename IE::setter_type setter;
				if constexpr (std::is_same_v<bool, decltype(setter(ie, to))>)
				{
					if (not setter(ie, to))
					{
						MED_THROW_EXCEPTION(invalid_value, name<IE>(), ie.get())
					}
				}
				else
				{
					setter(ie, to);
				}
				encode_single(encoder, ie);
			}
			else
			{
				encode_single(encoder, ie);
			}
		}
	}
};

struct set_dec
{
	template <class IE, class TO, class DECODER, class HEADER, class... DEPS>
	static bool check(TO&, DECODER&, HEADER const& header, DEPS&...)
	{
		using mi = meta::produce_info_t<DECODER, IE>;
		using tag_t = get_info_t<meta::list_first_t<mi>>;
		CODEC_TRACE("%zu tag match%c for %s(%s)", size_t(get_tag(header)), (tag_t::match(get_tag(header))? '+':'-'), name<tag_t>(), name<IE>());
		return tag_t::match( get_tag(header) );
	}

	template <class IE, class TO, class DECODER, class HEADER, class... DEPS>
	static constexpr void apply(TO& to, DECODER& decoder, HEADER const&, DEPS&... deps)
	{
		using mi = meta::produce_info_t<DECODER, IE>;
		//pop back the tag we've read as we have non-fixed tag inside
		using tag_t = get_info_t<meta::list_first_t<mi>>;
		if constexpr (!APredefinedValue<tag_t>) { decoder(POP_STATE{}); }

		IE& ie = to;
		if constexpr (AMultiField<IE>)
		{
			CODEC_TRACE("[%s]*%zu", name<IE>(), ie.count());
			if (ie.count() >= IE::max)
			{
				MED_THROW_EXCEPTION(extra_ie, name<IE>(), IE::max, ie.count())
			}
			auto* field = ie.push_back(decoder);
			sl::ie_decode<type_context<IE_SET, meta::list_rest_t<mi>>>(decoder, *field, deps...);
		}
		else //single-instance field
		{
			CODEC_TRACE("%c[%s]", ie.is_set()?'+':'-', name<IE>());
			if (not ie.is_set())
			{
				return sl::ie_decode<type_context<IE_SET, meta::list_rest_t<mi>>>(decoder, ie, deps...);
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

		if constexpr (AHasCondition<IE>) // conditional - quite an exotic case, since a med::set usually does not require conditional fields
		{
			bool const should_be_set = typename IE::condition{}(to);
			if (ie.is_set() != should_be_set)
			{
				CODEC_TRACE("%cC[%s] %s be set in %s", ie.is_set() ? '+' : '-', name<IE>(), should_be_set ? "MUST" : "must NOT", name<TO>());
				if (should_be_set) { MED_THROW_EXCEPTION(missing_ie, name<IE>(), 1, 0); }
				else               { MED_THROW_EXCEPTION(extra_ie,   name<IE>(), 0, 1); }
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
struct set_container<L<HEADER, IEs...>> : container<IE_SET, IEs...>
{
	using header_type = HEADER;
	static constexpr bool plain_header = false; //compound header e.g. a sequence
};
template <template <class...> class L, class IE1, class... IEs>
struct set_container<L<IE1, IEs...>> : container<IE_SET, IE1, IEs...>
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
		meta::foreach_prev<ies_types, void>(sl::set_enc{}, this->m_ies, encoder);
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
			using tag_t = get_info_t<meta::list_first_t<mi>>;

			while (decoder(PUSH_STATE{}, *this))
			{
				value<std::size_t> header;
				header.set_encoded(sl::decode_tag<tag_t>(decoder));
				CODEC_TRACE("tag=%#zX mi=%s firstIE=%s tag_t=%s", std::size_t(get_tag(header)), class_name<mi>(), name<IE>(), name<tag_t>());
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
				CODEC_TRACE("tag=%#zX hdr=%s", std::size_t(get_tag(header)), class_name<header_type>());
				meta::for_if<ies_types>(sl::set_dec{}, this->m_ies, decoder, header, deps...);
			}
		}
		meta::foreach<ies_types>(sl::set_check{}, this->m_ies, decoder);
	}
};

}	//end: namespace med
