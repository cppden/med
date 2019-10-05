/**
@file
choice class as union

@copyright Denis Priyomov 2016-2017
Distributed under the MIT License
(See accompanying file LICENSE or visit https://github.com/cppden/med)
*/

#pragma once

#include <new>
#include <brigand.hpp>

#include "exception.hpp"
#include "accessor.hpp"
#include "tag.hpp"
#include "length.hpp"
#include "encode.hpp"
#include "decode.hpp"
#include "meta/unique.hpp"
#include "meta/typelist.hpp"
#include "meta/foreach.hpp"

namespace med {

namespace sl {

struct choice_if
{
	template <class IE, class T, class... Ts>
	static bool check(T const& v, Ts&&...)
	{
		//CODEC_TRACE("%s(%s): %zu ? %zu", __FUNCTION__, name<typename IE::field_type>(), v.index(), v.template index<typename IE::field_type>());
		return v.index() == v.template index<typename IE::field_type>();
	}
};

struct choice_len : choice_if
{
	template <class IE, class TO, class ENCODER>
	static std::size_t apply(TO const& to, ENCODER& encoder)
	{
		if constexpr (TO::plain_header)
		{
			return field_length(to.template as<IE>(), encoder);
		}
		else
		{
			return field_length(to.header(), encoder)
				//skip TAG as 1st metainfo which is encoded in header
				+ sl::ie_length<meta::list_rest_t<get_meta_info_t<IE>>>(to.template as<IE>(), encoder);
		}
	}

	template <class TO, class ENCODER>
	static constexpr std::size_t apply(TO const&, ENCODER&)
	{
		return 0;
	}
};

struct choice_copy : choice_if
{
	template <class IE, class FROM, class TO, class... ARGS>
	static void apply(FROM const& from, TO& to, ARGS&&... args)
	{
		using type = typename IE::field_type;
		//CODEC_TRACE("copy: %s", name<IE>());
		to.template ref<type>().copy(from.template as<IE>(), std::forward<ARGS>(args)...);
		if constexpr (not (TO::plain_header or FROM::plain_header))
		{
			to.header().copy(from.header(), std::forward<ARGS>(args)...);
		}
	}

	template <class FROM, class TO, class... ARGS>
	static constexpr void apply(FROM const&, TO&, ARGS&&...) { }
};

template <class CODEC>
struct choice_name
{
	template <class IE, typename TAG, class... Ts>
	static constexpr bool check(TAG const& tag, Ts&&...)
	{
		using type = meta::unwrap_t<decltype(CODEC::template get_tag_type<IE>())>;
		return type::match( tag );
	}

	template <class IE, typename TAG, class... Ts>
	static constexpr char const* apply(TAG const&, Ts&&...)
	{
		return name<typename IE::field_type>();
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
	static void apply(TO const& to, ENCODER& encoder)
	{
		CODEC_TRACE("CASE[%s]", name<typename IE::field_type>());
		if constexpr (TO::plain_header)
		{
			med::encode(encoder, to.template as<IE>());
		}
		else
		{
			using type = meta::unwrap_t<decltype(ENCODER::template get_tag_type<IE>())>;
			type const tag{};
#if 0 //TODO: how to not modify? problem to copy with length placeholder...
			auto hdr = to.header();
			hdr.set_tag(tag.get());
			med::encode(encoder, hdr);
#else
			const_cast<TO&>(to).header().set_tag(tag.get());
			med::encode(encoder, to.header());
#endif
			//skip 1st TAG meta-info as it's encoded in header
			sl::ie_encode<meta::list_rest_t<get_meta_info_t<IE>>>(encoder, to.template as<IE>());
		}
	}

	template <class TO, class ENCODER>
	static void apply(TO const& to, ENCODER&)
	{
		MED_THROW_EXCEPTION(unknown_tag, name<TO>(), to.index())
	}
};

struct choice_dec
{
	template <class IE, class TO, class HEADER, class DECODER, class UNEXP>
	bool check(TO&, HEADER const& header, DECODER&, UNEXP&)
	{
		using type = meta::unwrap_t<decltype(DECODER::template get_tag_type<IE>())>;
		return type::match( get_tag(header) );
	}

	template <class IE, class TO, class HEADER, class DECODER, class UNEXP>
	static void apply(TO& to, HEADER const&, DECODER& decoder, UNEXP& unexp)
	{
		CODEC_TRACE("CASE[%s]", name<typename IE::field_type>());
		auto& ie = static_cast<IE&>(to.template ref<typename IE::field_type>());
		//skip 1st TAG meta-info as it's decoded in header
		sl::ie_decode<meta::list_rest_t<get_meta_info_t<IE>>>(decoder, ie, unexp);
	}

	template <class TO, class HEADER, class DECODER, class UNEXP>
	void apply(TO& to, HEADER const& header, DECODER& decoder, UNEXP& unexp)
	{
		unexp(decoder, to, header);
	}
};

} //end: namespace sl

namespace detail {

struct dummy_header
{
	static constexpr bool is_set()          { return true; }
	static constexpr void clear()           { }
};

template <class HEADER, class = void>
struct make_header : dummy_header
{
	static constexpr bool plain_header = true; //plain header e.g. a tag

	auto& header() const                    { return *this; }
};

template <class HEADER>
struct make_header<HEADER, std::enable_if_t<detail::has_get_tag<HEADER>::value>>
{
	static constexpr bool plain_header = false; //compound header e.g. a sequence

	using header_type = HEADER;
	header_type const& header() const       { return m_header; }
	header_type& header()                   { return m_header; }

private:
	header_type  m_header;
};

template <class T>
struct selector
{
	explicit selector(T* that) noexcept
		: m_this(that)
	{}

	template <class U> operator U&()
	{
		static_assert(!std::is_const<T>(), "CONST CHOICE RETURNS A POINTER, NOT REFERENCE!");
		return m_this->template ref<U>();
	}

	template <class U> operator U const* ()
	{
		return m_this->template get<U>();
	}

	T* m_this;
};

} //end: namespace detail


template <class TRAITS, class... IEs>
class base_choice : public IE<CONTAINER>
		, public detail::make_header< meta::list_first_t<brigand::list<IEs...>> >
{
public:
	using traits = TRAITS;
	using ies_types = conditional_t<
		detail::has_get_tag< meta::list_first_t<brigand::list<IEs...>> >::value,
		meta::list_rest_t<brigand::list<IEs...>>,
		brigand::list<IEs...>>;
	using field_types = brigand::transform<ies_types, get_field_type<brigand::_1>>;
	static constexpr std::size_t num_types = brigand::size<ies_types>::value;

	template <typename TAG, class CODEC>
	static constexpr char const* name_tag(TAG const& tag, CODEC& codec)
	{ return meta::for_if<ies_types>(sl::choice_name<CODEC>{}, tag, codec); }

	template <class T>
	static constexpr bool has()             { return not std::is_void_v<meta::find<ies_types, sl::field_at<T>>>; }

	//current selected index (type)
	std::size_t index() const               { return m_index; }
	//return index of given type
	template <class T>
	static constexpr std::size_t index()    { return brigand::index_of<field_types, T>::value; }

	void clear()                            { this->header().clear(); m_index = num_types; }
	bool is_set() const                     { return this->header().is_set() && m_index != num_types; }
	template <class ENC>
	std::size_t calc_length(ENC& enc) const { return meta::for_if<ies_types>(this->header().is_set(), sl::choice_len{}, *this, enc); }

	auto select()                           { return detail::selector{this}; }
	auto select() const                     { return detail::selector{this}; }
	auto cselect() const                    { return detail::selector{this}; }

	template <class T> T& ref()
	{
		//TODO: how to prevent a copy when callee-side re-uses reference by mistake?
		static_assert(!std::is_const<T>(), "REFERENCE IS NOT FOR ACCESSING AS CONST");
		using type = meta::find<ies_types, sl::field_at<T>>;
		static_assert(!std::is_void<type>(), "NO SUCH TYPE IN CHOICE");
		constexpr auto idx = base_choice::template index<T>();
		auto* ie = reinterpret_cast<type*>(&m_storage);
		if (idx != index())
		{
			m_index = idx;
			new (&m_storage) type{};
		}
		return *ie;
	}

	template <class T> T const* get() const
	{
		using type = meta::find<ies_types, sl::field_at<T>>;
		static_assert(!std::is_void<type>(), "NO SUCH CASE IN CHOICE");
		type const* ie = base_choice::template index<T>() == index()
			? &this->template as<type>()
			: nullptr;
		return ie;
	}

	template <class FROM, class... ARGS>
	void copy(FROM const& from, ARGS&&... args)
	{ meta::for_if<ies_types>(sl::choice_copy{}, from, *this, std::forward<ARGS>(args)...); }

	template <class TO, class... ARGS>
	void copy_to(TO& to, ARGS&&... args) const
	{ meta::for_if<ies_types>(sl::choice_copy{}, *this, to, std::forward<ARGS>(args)...); }

	template <class ENCODER>
	void encode(ENCODER& encoder) const
	{ meta::for_if<ies_types>(sl::choice_enc{}, *this, encoder); }

	template <class DECODER, class UNEXP>
	void decode(DECODER& decoder, UNEXP& unexp)
	{
		static_assert(std::is_void_v<meta::unique_t<tag_getter<DECODER>, ies_types>>
			, "SEE ERROR ON INCOMPLETE TYPE/UNDEFINED TEMPLATE HOLDING IEs WITH CLASHED TAGS");

		clear();
		if constexpr (base_choice::plain_header)
		{
			using IE = meta::list_first_t<ies_types>; //use 1st IE since all have similar tag
			using type = meta::unwrap_t<decltype(DECODER::template get_tag_type<IE>())>;
			value<std::size_t> tag;
			tag.set(sl::decode_tag<type, true>(decoder));
			meta::for_if<ies_types>(sl::choice_dec{}, *this, tag, decoder, unexp);
		}
		else
		{
			med::decode(decoder, this->header(), unexp);
			meta::for_if<ies_types>(sl::choice_dec{}, *this, this->header(), decoder, unexp);
		}
	}

private:
	friend struct sl::choice_len;
	friend struct sl::choice_copy;
	friend struct sl::choice_enc;
	friend struct sl::choice_dec;

	using storage_type = meta::list_aligned_union_t<ies_types>;

	template <class T> T& as()              { return *reinterpret_cast<T*>(&m_storage); }
	template <class T> T const& as() const  { return *reinterpret_cast<T const*>(&m_storage); }

	std::size_t  m_index {num_types}; //index of selected type in storage
	storage_type m_storage;
};

template <class ...IEs>
using choice = base_choice<base_traits, IEs...>;

} //end: namespace med
