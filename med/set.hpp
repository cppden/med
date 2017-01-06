/**
@file
set IE container - tagged elements in any order

@copyright Denis Priyomov 2016
Distributed under the MIT License
(See accompanying file LICENSE or copy at https://opensource.org/licenses/MIT)
*/

#pragma once

#include "ie.hpp"
#include "error.hpp"
#include "optional.hpp"
#include "container.hpp"
#include "encode.hpp"
#include "decode.hpp"
#include "name.hpp"
#include "tag.hpp"
#include "debug.hpp"

namespace med {

namespace sl {


template <class Enable, class... IES>
struct set_dec_imp;

template <class... IES>
using set_decoder = set_dec_imp<void, IES...>;

template <class IE, class... IES>
struct set_dec_imp<std::enable_if_t<!is_multi_field<IE>::value>, IE, IES...>
{
	template <class TO, class FUNC, class UNEXP, class HEADER>
	static inline bool decode(TO& to, FUNC& func, UNEXP& unexp, HEADER const& header)
	{
		if (tag_type_t<IE>::match( get_tag(header) ))
		{
			IE& ie = static_cast<IE&>(to);
			CODEC_TRACE("[%s] = %u", name<typename IE::field_type>(), ie.ref_field().is_set());
			if (!ie.ref_field().is_set())
			{
				return med::decode(func, ie.ref_field(), unexp);
			}
			else
			{
				func(error::EXTRA_IE, name<typename IE::field_type>(), 1);
				return false;
			}
		}
		else
		{
			return set_decoder<IES...>::decode(to, func, unexp, header);
		}
	}

	template <class TO, class FUNC>
	static bool check(TO const& to, FUNC& func)
	{
		IE const& ie = static_cast<IE const&>(to);
		if (is_optional_v<IE> || ie.ref_field().is_set())
		{
			return set_decoder<IES...>::check(to, func);
		}
		else
		{
			func(error::MISSING_IE, name<typename IE::field_type>(), 1, 0);
			return false;
		}
	}
};

template <class IE, class... IES>
struct set_dec_imp<std::enable_if_t<is_multi_field<IE>::value>, IE, IES...>
{
	template <class TO, class FUNC, class UNEXP, class HEADER>
	static inline bool decode(TO& to, FUNC& func, UNEXP& unexp, HEADER const& header)
	{
		if (tag_type_t<IE>::match( get_tag(header) ))
		{
			IE& ie = static_cast<IE&>(to);
			CODEC_TRACE("[%s]@%zu", name<typename IE::field_type>(), ie.count());
			if (ie.count() >= IE::max)
			{
				func(error::EXTRA_IE, name<typename IE::field_type>(), IE::max, ie.count());
				return false;
			}
			auto* pfield = ie.push_back(func);
			return pfield && med::decode(func, *pfield, unexp);
		}
		else
		{
			return set_decoder<IES...>::decode(to, func, unexp, header);
		}
	}

	template <class TO, class FUNC>
	static bool check(TO const& to, FUNC& func)
	{
		IE const& ie = static_cast<IE const&>(to);
		return check_arity(func, ie)
			&& set_decoder<IES...>::check(to, func);
	}
};

template <>
struct set_dec_imp<void>
{
	template <class TO, class FUNC, class UNEXP, class HEADER>
	static bool decode(TO& to, FUNC& func, UNEXP& unexp, HEADER const& header)
	{
		return unexp(func, to, header);
	}

	template <class TO, class FUNC>
	static constexpr bool check(TO&, FUNC&)       { return true; }
};


template <class Enable, class... IES>
struct set_enc_imp;

template <class... IES>
using set_encoder = set_enc_imp<void, IES...>;

template <class HEADER, class IE, class FUNC>
std::enable_if_t<std::is_base_of<PRIMITIVE, typename HEADER::ie_type>::value, bool>
inline encode_header(FUNC& func)
{
	HEADER header;
	header.set_encoded(tag_type_t<IE>::get_encoded());
	return encode(func, header);
}

template <class HEADER, class IE, class FUNC>
std::enable_if_t<std::is_base_of<CONTAINER, typename HEADER::ie_type>::value, bool>
constexpr encode_header(FUNC&)
{
	return true;
}


template <class TO, class FUNC, class HEADER, class IE, class... IES>
std::enable_if_t<is_optional_v<IE>, bool>
inline continue_encode(TO const& to, FUNC& func)
{
	return set_encoder<IES...>::template encode<HEADER>(to, func);
}

template <class TO, class FUNC, class HEADER, class IE, class... IES>
std::enable_if_t<!is_optional_v<IE>, bool>
inline continue_encode(TO const&, FUNC& func)
{
	func(error::MISSING_IE, name<typename IE::field_type>(), 1, 0);
	return false;
}


template <class IE, class... IES>
struct set_enc_imp<std::enable_if_t<!is_multi_field<IE>::value>, IE, IES...>
{
	template <class HEADER, class TO, class FUNC>
	static inline bool encode(TO const& to, FUNC& func)
	{
		IE const& ie = static_cast<IE const&>(to);
		if (ie.ref_field().is_set())
		{
			CODEC_TRACE("[%s]", name<IE>());
			return encode_header<HEADER, IE>(func) && med::encode(func, ie.ref_field())
				&& set_encoder<IES...>::template encode<HEADER>(to, func);
		}
		else
		{
			return continue_encode<TO, FUNC, HEADER, IE, IES...>(to, func);
		}
	}
};


template <class IE, class... IES>
struct set_enc_imp<std::enable_if_t<is_multi_field<IE>::value>, IE, IES...>
{
	template <class HEADER, class TO, class FUNC>
	static inline bool encode(TO const& to, FUNC& func)
	{
		IE const& ie = static_cast<IE const&>(to);
		CODEC_TRACE("[%s]*%zu", name<typename IE::field_type>(), ie.count());

		if (!check_arity(func, ie)) return false;
		for (auto& field : ie)
		{
			if (field.is_set())
			{
				if (!encode_header<HEADER, IE>(func) || !med::encode(func, field)) return false;
			}
			else
			{
				//TODO: actually this field was pushed but not set... do we need a new error?
				func(error::MISSING_IE, name<typename IE::field_type>(), ie.count(), ie.count()-1);
				return false;
			}
		}
		return set_encoder<IES...>::template encode<HEADER>(to, func);
	}
};

template <>
struct set_enc_imp<void>
{
	template <class HEADER, class TO, class FUNC>
	static constexpr bool encode(TO&, FUNC&)       { return true; }
};

template <class T, class FUNC>
std::enable_if_t<std::is_base_of<PRIMITIVE, typename T::ie_type>::value>
constexpr pop_state(FUNC&&)
{
}

template <class T, class FUNC>
std::enable_if_t<std::is_base_of<CONTAINER, typename T::ie_type>::value>
inline pop_state(FUNC&& func)
{
	func(POP_STATE{});
}

}	//end: namespace sl


template <class HEADER, class ...IES>
struct set : container<IES...>
{
	using header_type = HEADER;

	template <class ENCODER>
	bool encode(ENCODER& encoder) const
	{
		return sl::set_encoder<IES...>::template encode<header_type>(this->m_ies, encoder);
	}

	template <class DECODER, class UNEXP>
	bool decode(DECODER& decoder, UNEXP& unexp)
	{
		while (decoder(PUSH_STATE{}))
		{
			header_type header;
			if (med::decode(decoder, header, unexp))
			{
				sl::pop_state<header_type>(decoder);
				CODEC_TRACE("tag=%#zx", get_tag(header));
				if (!sl::set_decoder<IES...>::decode(this->m_ies, decoder, unexp, header))
				{
					return false;
				}
			}
			else
			{
				decoder(POP_STATE{});
				decoder(error::SUCCESS);
				break;
			}
		}

		return sl::set_decoder<IES...>::check(this->m_ies, decoder);
	}
};

}	//end: namespace med
