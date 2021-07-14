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
		//CODEC_TRACE("%s(%s): %zu ? %zu", __FUNCTION__, name<IE>(), v.index(), v.template index<get_field_type_t<IE>>());
		return v.index() == v.template index<get_field_type_t<IE>>();
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
				+ sl::ie_length<meta::list_rest_t<
					meta::produce_info_t<ENCODER, IE>>>>(to.template as<IE>(), encoder);
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
	static void apply(TO& to)
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
	static void apply(FROM const& from, TO& to, ARGS&&... args)
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

template <class CODEC>
struct choice_name
{
	template <class IE, typename TAG, class... Ts>
	static constexpr bool check(TAG const& tag, Ts&&...)
	{
		using type = typename meta::list_first<meta::produce_info_t<CODEC, IE>>::type;
		//static_assert (type::mik == mik::TAG, "NOT A TAG");
		return type::match( tag );
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
	static void apply(TO const& to, ENCODER& encoder)
	{
		CODEC_TRACE("CASE[%s]", name<IE>());
		if constexpr (TO::plain_header)
		{
			med::encode(encoder, to.template as<IE>());
		}
		else
		{
			using mi = meta::produce_info_t<ENCODER, IE>;
			//tag of this IE isn't in header
			if constexpr (!explicit_meta_in<mi, typename TO::header_type>())
			{
				using tag_t = meta::list_first_t<mi>;
				tag_t const tag{};
#if 0 //TODO: how to not modify? problem to copy with length placeholder...
				auto hdr = to.header();
				hdr.set_tag(tag.get());
#else
				const_cast<TO&>(to).header().set_tag(tag.get());
#endif
			}
			med::encode(encoder, to.header());
			//skip 1st TAG meta-info as it's encoded in header
			sl::ie_encode<meta::list_rest_t<mi>>(encoder, to.template as<IE>());
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
		using mi = meta::produce_info_t<DECODER, IE>;
		using type = meta::list_first_t<mi>;
		//static_assert (type::mik = kind::TAG, "NOT A TAG");
		return type::match( get_tag(header) );
	}

	template <class IE, class TO, class HEADER, class DECODER, class UNEXP>
	static void apply(TO& to, HEADER const&, DECODER& decoder, UNEXP& unexp)
	{
		CODEC_TRACE("CASE[%s]", name<IE>());
		auto& ie = static_cast<IE&>(to.template ref<get_field_type_t<IE>>());
		//skip 1st TAG meta-info as it's decoded in header
		using mi = meta::produce_info_t<DECODER, IE>;
		sl::ie_decode<meta::list_rest_t<mi>>(decoder, ie, unexp);
	}

	template <class TO, class HEADER, class DECODER, class UNEXP>
	void apply(TO& to, HEADER const& header, DECODER& decoder, UNEXP& unexp)
	{
		unexp(decoder, to, header);
	}
};

} //end: namespace sl

namespace detail {

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

template <class L> struct list_aligned_union;
template <template <class...> class L, class... Ts> struct list_aligned_union<L<Ts...>> : std::aligned_union<0, Ts...> {};

struct dummy_header
{
	static constexpr bool is_set()          { return true; }
	static constexpr void clear()           { }
};

template <class HEADER, class = void>
struct choice_header : dummy_header
{
	static constexpr bool plain_header = true; //plain header e.g. a tag

	auto& header() const                    { return *this; }
};

template <class HEADER>
struct choice_header<HEADER, std::enable_if_t<has_get_tag<HEADER>::value>>
{
	static constexpr bool plain_header = false; //compound header e.g. a sequence

	using header_type = HEADER;
	header_type const& header() const       { return m_header; }
	header_type& header()                   { return m_header; }

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
		detail::has_get_tag< meta::list_first_t<meta::typelist<IEs...>> >::value,
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
	std::size_t index() const               { return m_index; }
	//return index of given type
	template <class T>
	static constexpr std::size_t index()    { return meta::list_index_of_v<T, fields_types>; }

	void clear()
	{
		meta::for_if<ies_types>(sl::choice_clear{}, *this);
		this->header().clear(); m_index = num_types;
	}
	bool is_set() const                     { return this->header().is_set() && index() != num_types; }
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
		constexpr auto idx = choice::template index<T>();
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
		type const* ie = choice::template index<T>() == index()
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
		if constexpr (choice::plain_header)
		{
			using IE = meta::list_first_t<ies_types>; //use 1st IE since all have similar tag
			using mi = meta::produce_info_t<DECODER, IE>;
			using tag_t = meta::list_first_t<mi>;
			value<std::size_t> tag;
			tag.set_encoded(sl::decode_tag<tag_t, true>(decoder));
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

	using storage_type = typename detail::list_aligned_union<ies_types>::type;

	template <class T> T& as()              { return *reinterpret_cast<T*>(&m_storage); }
	template <class T> T const& as() const  { return *reinterpret_cast<T const*>(&m_storage); }

	std::size_t  m_index {num_types}; //index of selected type in storage
	storage_type m_storage;
};

} //end: namespace med
