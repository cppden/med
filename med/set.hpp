/*!
@file
TODO: define.

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
			CODEC_TRACE("[%s]*", name<typename IE::field_type>());
			if (auto* field = ie.push_back(func))
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
		IE const& ie = static_cast<IE const&>(to);
		if (check_arity<IE>(field_count(ie)))
		{
			return set_decoder<IES...>::check(to, func);
		}
		else
		{
			func(error::MISSING_IE, name<typename IE::field_type>(), IE::min, field_count(ie));
		}
		return false;
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

template <class HEADER, class FUNC, class IE>
constexpr int set_invoke_encode(FUNC&, IE&, int count) { return count; }

template <class HEADER, class FUNC, class IE, std::size_t INDEX, std::size_t... Is>
inline int set_invoke_encode(FUNC& func, IE& ie, int count)
{
	//TODO: use iterators
	if (ie.ref_field(INDEX).is_set())
	{
		if (!encode_header<HEADER, IE>(func) || !encode(func, ie.ref_field(INDEX))) return -1;
		return set_invoke_encode<HEADER, FUNC, IE, Is...>(func, ie, INDEX+1);
	}
	return INDEX;
}

template<class HEADER, class FUNC, class IE, std::size_t... Is>
inline int set_repeat_encode_impl(FUNC& func, IE& ie, std::index_sequence<Is...>)
{
	return set_invoke_encode<HEADER, FUNC, IE, Is...>(func, ie, 0);
}

template <class HEADER, std::size_t N, class FUNC, class IE>
inline int set_encode(FUNC& func, IE& ie)
{
	return set_repeat_encode_impl<HEADER>(func, ie, std::make_index_sequence<N>{});
}


template <class IE, class... IES>
struct set_enc_imp<std::enable_if_t<is_multi_field<IE>::value>, IE, IES...>
{
	template <class HEADER, class TO, class FUNC>
	static inline bool encode(TO const& to, FUNC& func)
	{
		CODEC_TRACE("[%s]*%zu", name<typename IE::field_type>(), IE::max);

		IE const& ie = static_cast<IE const&>(to);
		int const count = set_encode<HEADER, IE::max>(func, ie);
		if (count < 0) return false;

		if (check_arity<IE>(count))
		{
			return set_encoder<IES...>::template encode<HEADER>(to, func);
		}
		else
		{
			func(error::MISSING_IE, name<typename IE::field_type>(), IE::min, count);
		}
		return false;
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
	func.pop_state();
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
		while (decoder.push_state())
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
				decoder.pop_state();
				decoder(error::SUCCESS);
				break;
			}
		}

		return sl::set_decoder<IES...>::check(this->m_ies, decoder);
	}
};

}	//end: namespace med
