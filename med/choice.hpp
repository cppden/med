/**
@file
choice class as union

@copyright Denis Priyomov 2016-2017
Distributed under the MIT License
(See accompanying file LICENSE or visit https://github.com/cppden/med)
*/

#pragma once

#include <new>

#include "exception.hpp"
#include "accessor.hpp"
#include "tag.hpp"
#include "length.hpp"
#include "value.hpp"
#include "encode.hpp"
#include "decode.hpp"
#include "meta/unique.hpp"
#include "meta/typelist.hpp"

namespace med {

namespace sl {

struct choice_if
{
	template <class IE, class T, class... Ts>
	static constexpr bool check(T const& v, Ts&&...)
	{
		//CODEC_TRACE("%s(%s): %zu ? %zu", __FUNCTION__, name<IE>(), v.index(), v.template index<get_field_type_t<IE>>());
		return v.index() == v.template index<get_field_type_t<IE>>();
	}
};

struct choice_len : choice_if
{
	template <class IE, class TO, class ENCODER>
	static constexpr std::size_t apply(TO const& to, ENCODER& encoder)
	{
		if constexpr (TO::plain_header)
		{
			return field_length(to.template as<IE>(), encoder);
		}
		else
		{
			return field_length(to.header(), encoder)
				//skip TAG as 1st metainfo which is encoded in header
				+ sl::ie_length<void, meta::list_rest_t<
					meta::produce_info_t<ENCODER, IE>>>(to.template as<IE>(), encoder);
		}
	}

	template <class TO, class ENCODER>
	static constexpr std::size_t apply(TO const&, ENCODER&)
	{
		return 0;
	}
};

struct choice_clear : choice_if
{
	template <class IE, class TO>
	static constexpr void apply(TO& to)
	{
		CODEC_TRACE("clear: %s", name<IE>());
		to.template ref<get_field_type_t<IE>>().clear();
	}

	template <class TO>
	static constexpr void apply(TO&) { }
};

struct choice_copy : choice_if
{
	template <class IE, class FROM, class TO, class... ARGS>
	static constexpr void apply(FROM const& from, TO& to, ARGS&&... args)
	{
		//CODEC_TRACE("copy: %s", name<IE>());
		to.template ref<get_field_type_t<IE>>().copy(from.template as<IE>(), std::forward<ARGS>(args)...);
		if constexpr (not (TO::plain_header or FROM::plain_header))
		{
			to.header().copy(from.header(), std::forward<ARGS>(args)...);
		}
	}

	template <class FROM, class TO, class... ARGS>
	static constexpr void apply(FROM const&, TO&, ARGS&&...) { }
};

struct choice_eq : choice_if
{
	template <class IE, class CHOICE>
	static constexpr bool apply(CHOICE const& lhs, CHOICE const& rhs)
	{
		if constexpr (!CHOICE::plain_header)
		{
			if (!(lhs.header() == rhs.header())) return false;
		}
		using field_type = get_field_type_t<IE>;
		return static_cast<field_type const&>(lhs.template as<IE>()) == static_cast<field_type const&>(rhs.template as<IE>());
		//to.template ref<>().clear();
	}

	template <class CHOICE>
	static constexpr bool apply(CHOICE const&, CHOICE const&) { return false; }
};

template <class CODEC>
struct choice_name
{
	template <class IE, typename TAG, class... Ts>
	static constexpr bool check(TAG const& tag, Ts&&...)
	{
		using tag_t = typename meta::list_first<meta::produce_info_t<CODEC, IE>>::type::info_type;
		//static_assert (type::mik == mik::TAG, "NOT A TAG");
		return tag_t::match( tag );
	}

	template <class IE, typename TAG, class... Ts>
	static constexpr char const* apply(TAG const&, Ts&&...)
	{
		return name<IE>();
	}

	template <typename TAG, class... Ts>
	static constexpr char const* apply(TAG const&, Ts&&...)
	{
		return nullptr;
	}
};

struct choice_enc : choice_if
{
	template <class IE, class TO, class ENCODER>
	static constexpr void apply(TO const& to, ENCODER& encoder)
	{
		using mi = meta::produce_info_t<ENCODER, IE>;
		CODEC_TRACE("CASE[%s] mi=%s %s", name<IE>(), class_name<mi>(), TO::plain_header?"plain":"compaund");
		if constexpr (TO::plain_header)
		{
			if constexpr (std::is_base_of_v<CONTAINER, typename IE::ie_type>)
			{
				using type = typename meta::list_first_t<mi>::info_type;
				using EXPOSED = get_field_type_t<meta::list_first_t<typename IE::ies_types>>;
				if constexpr(std::is_same_v<EXPOSED, type>)
				{
					CODEC_TRACE("exposed[%s]", name<EXPOSED>());
					//encoode 1st TAG meta-info via exposed
					sl::ie_encode<meta::typelist<>, void>(encoder, to.template as<EXPOSED>());
					//skip 1st TAG meta-info and encode it via exposed
					return sl::ie_encode<meta::list_rest_t<mi>, EXPOSED>(encoder, to.template as<IE>());
				}
			}
			med::encode(encoder, to.template as<IE>());
		}
		else
		{
			//tag of this IE isn't in header
			if constexpr (!explicit_meta_in<mi, typename TO::header_type>())
			{
				using tag_t = typename meta::list_first_t<mi>::info_type;
				tag_t const tag{};
				//TODO: how to not modify?
				const_cast<TO&>(to).header().set_tag(tag.get());
			}
			med::encode(encoder, to.header());
			//skip 1st TAG meta-info as it's encoded in header
			sl::ie_encode<meta::list_rest_t<mi>, void>(encoder, to.template as<IE>());
		}
	}

	template <class TO, class ENCODER>
	static constexpr void apply(TO const& to, ENCODER&)
	{
		MED_THROW_EXCEPTION(unknown_tag, name<TO>(), to.index())
	}
};

struct choice_dec
{
	template <class IE, class TO, class HEADER, class DECODER, class... DEPS>
	constexpr bool check(TO&, HEADER const& header, DECODER&, DEPS&...)
	{
		using mi = meta::produce_info_t<DECODER, IE>;
		using tag_t = typename meta::list_first_t<mi>::info_type;
		//static_assert (type::mik = kind::TAG, "NOT A TAG");
		return tag_t::match( get_tag(header) );
	}

	template <class IE, class TO, class HEADER, class DECODER, class... DEPS>
	static constexpr void apply(TO& to, HEADER const& header, DECODER& decoder, DEPS&... deps)
	{
		CODEC_TRACE("CASE[%s]", name<IE>());
		auto& ie = static_cast<IE&>(to.template ref<get_field_type_t<IE>>());
		//skip 1st TAG meta-info as it's decoded in header
		using mi = meta::produce_info_t<DECODER, IE>;
		if constexpr (std::is_base_of_v<CONTAINER, typename IE::ie_type>)
		{
			using type = typename meta::list_first_t<mi>::info_type;
			using EXPOSED = get_field_type_t<meta::list_first_t<typename IE::ies_types>>;
			if constexpr(std::is_same_v<EXPOSED, type>)
			{
				CODEC_TRACE("exposed[%s] = %#zx", name<EXPOSED>(), size_t(header.get()));
				ie.template ref<type>().set(header.get());
				return sl::ie_decode<sl::decode_type_context<meta::list_rest_t<mi>, EXPOSED>>(decoder, ie, deps...);
			}
		}
		sl::ie_decode<sl::decode_type_context<meta::list_rest_t<mi>>>(decoder, ie, deps...);
	}

	template <class TO, class HEADER, class DECODER, class... DEPS>
	constexpr void apply(TO&, HEADER const& header, DECODER&, DEPS&...)
	{
		MED_THROW_EXCEPTION(unknown_tag, name<TO>(), get_tag(header))
	}
};

} //end: namespace sl

namespace detail {

template <class T>
struct selector
{
	explicit constexpr selector(T* that) noexcept
		: m_this(that)
	{}

	template <class U> constexpr operator U&()
	{
		static_assert(!std::is_const_v<T>, "CONST CHOICE RETURNS A POINTER, NOT REFERENCE!");
		return m_this->template ref<U>();
	}

	template <class U> constexpr operator U const* ()
	{
		return m_this->template get<U>();
	}

	T* m_this;
};

template <class L> struct list_aligned_union;
template <template <class...> class L, class... Ts> struct list_aligned_union<L<Ts...>> : std::aligned_union<0, Ts...> {};

template <class HEADER>
struct choice_header
{
	static constexpr bool plain_header = true; //plain header e.g. a tag

	static constexpr bool is_set()          { return true; }
	static constexpr void clear()           { }
	constexpr auto& header() const          { return *this; }
};

template <AHasGetTag HEADER>
struct choice_header<HEADER>
{
	static constexpr bool plain_header = false; //compound header e.g. a sequence

	using header_type = HEADER;
	constexpr header_type const& header() const { return m_header; }
	constexpr header_type& header()             { return m_header; }

private:
	header_type  m_header;
};

} //end: namespace detail


template <class... IEs>
class choice : public IE<CONTAINER>
		, public detail::choice_header< meta::list_first_t<meta::typelist<IEs...>> >
{
public:
	using ies_types = conditional_t<
		AHasGetTag< meta::list_first_t<meta::typelist<IEs...>> >,
		meta::list_rest_t<meta::typelist<IEs...>>,
		meta::typelist<IEs...>>;
	using fields_types = meta::transform_t<ies_types, get_field_type>;
	static constexpr auto num_types = meta::list_size_v<ies_types>;

	template <typename TAG, class CODEC>
	static constexpr char const* name_tag(TAG const& tag, CODEC& codec)
	{ return meta::for_if<ies_types>(sl::choice_name<CODEC>{}, tag, codec); }

	template <class T>
	static constexpr bool has()             { return not std::is_void_v<meta::find<ies_types, sl::field_at<T>>>; }

	//current selected index (type)
	constexpr std::size_t index() const     { return m_index; }
	//return index of given type
	template <class T>
	static constexpr std::size_t index()    { return meta::list_index_of_v<T, fields_types>; }

	constexpr void clear()
	{
		meta::for_if<ies_types>(sl::choice_clear{}, *this);
		this->header().clear(); m_index = num_types;
	}

	constexpr bool is_set() const           { return this->header().is_set() && index() != num_types; }

	template <class ENC>
	constexpr std::size_t calc_length(ENC& enc) const
		{ return meta::for_if<ies_types>(this->header().is_set(), sl::choice_len{}, *this, enc); }

	constexpr auto select()                 { return detail::selector{this}; }
	constexpr auto select() const           { return detail::selector{this}; }
	constexpr auto cselect() const          { return detail::selector{this}; }

	template <class T> constexpr T& ref()
	{
		//TODO: how to prevent a copy when callee-side re-uses reference by mistake?
		static_assert(!std::is_const_v<T>, "REFERENCE IS NOT FOR ACCESSING AS CONST");
		using type = meta::find<ies_types, sl::field_at<T>>;
		static_assert(!std::is_void<type>(), "NO SUCH TYPE IN CHOICE");
		constexpr auto idx = choice::template index<T>();
		auto* ie = reinterpret_cast<type*>(&m_storage);
		if (idx != index())
		{
			m_index = idx;
			new (&m_storage) type{};
		}
		return *ie;
	}

	template <class T> constexpr T const* get() const
	{
		using type = meta::find<ies_types, sl::field_at<T>>;
		static_assert(!std::is_void<type>(), "NO SUCH CASE IN CHOICE");
		type const* ie = choice::template index<T>() == index()
			? &this->template as<type>()
			: nullptr;
		return ie;
	}

	template <class FROM, class... ARGS>
	constexpr void copy(FROM const& from, ARGS&&... args)
	{ meta::for_if<ies_types>(sl::choice_copy{}, from, *this, std::forward<ARGS>(args)...); }

	constexpr bool operator==(choice const& rhs) const
		{ return index() == rhs.index() && meta::for_if<ies_types>(sl::choice_eq{}, *this, rhs); }

	template <class TO, class... ARGS>
	constexpr void copy_to(TO& to, ARGS&&... args) const
	{ meta::for_if<ies_types>(sl::choice_copy{}, *this, to, std::forward<ARGS>(args)...); }

	template <class ENCODER>
	constexpr void encode(ENCODER& encoder) const
	{ meta::for_if<ies_types>(sl::choice_enc{}, *this, encoder); }

	template <class DECODER, class... DEPS>
	constexpr void decode(DECODER& decoder, DEPS&... deps)
	{
		static_assert(std::is_void_v<meta::unique_t<tag_getter<DECODER>, ies_types>>
			, "SEE ERROR ON INCOMPLETE TYPE/UNDEFINED TEMPLATE HOLDING IEs WITH CLASHED TAGS");

		clear();
		if constexpr (choice::plain_header)
		{
			using IE = meta::list_first_t<ies_types>; //use 1st IE since all have similar tag
			using mi = meta::produce_info_t<DECODER, IE>;
			using tag_t = typename meta::list_first_t<mi>::info_type;
			as_writable_t<tag_t> tag;
			tag.set_encoded(sl::decode_tag<tag_t, true>(decoder));
			meta::for_if<ies_types>(sl::choice_dec{}, *this, tag, decoder, deps...);
		}
		else
		{
			med::decode(decoder, this->header(), deps...);
			meta::for_if<ies_types>(sl::choice_dec{}, *this, this->header(), decoder, deps...);
		}
	}

private:
	friend struct sl::choice_len;
	friend struct sl::choice_copy;
	friend struct sl::choice_enc;
	friend struct sl::choice_dec;
	friend struct sl::choice_eq;

	using storage_type = typename detail::list_aligned_union<ies_types>::type;

	template <class T> constexpr T& as()                { return *reinterpret_cast<T*>(&m_storage); }
	template <class T> constexpr T const& as() const    { return *reinterpret_cast<T const*>(&m_storage); }

	std::size_t  m_index {num_types}; //index of selected type in storage
	storage_type m_storage;
};

} //end: namespace med
