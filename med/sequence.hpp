/*!
TODO: define.

@copyright Denis Priyomov 2016
Distributed under the MIT License
(See accompanying file LICENSE or copy at https://opensource.org/licenses/MIT)
*/

#pragma once

#include "ie.hpp"
#include "state.hpp"
#include "container.hpp"
#include "encode.hpp"
#include "decode.hpp"
#include "debug.hpp"

namespace med {

namespace sl {

template <class Enable, class... IES>
struct seq_dec_imp;

template <class... IES>
using seq_decoder = seq_dec_imp<void, IES...>;

//mandatory field
template <class IE, class... IES>
struct seq_dec_imp<std::enable_if_t<
		!is_optional_v<IE>
	>,
	IE, IES...>
{
	template <class TO, class FUNC, class UNEXP, class TAG>
	static inline bool decode(TO& to, FUNC&& func, UNEXP& unexp, TAG& vtag)
	{
		CODEC_TRACE("{%s}...", name<IE>());
		IE& ie = to;
		return med::decode(func, ie, unexp) && seq_decoder<IES...>::decode(to, func, unexp, vtag);
	}
};

//TODO: more flexible is to compare current size read and how many bits to rewind for RO IE.
template <class IE, class FUNC, class TAG>
std::enable_if_t<is_read_only_v<typename IE::tag_type>>
inline clear_tag(FUNC& func, TAG& vtag)
{
	func.pop_state();
	vtag.clear();
}

template <class IE, class FUNC, class TAG>
std::enable_if_t<!is_read_only_v<typename IE::tag_type>>
inline clear_tag(FUNC&, TAG& vtag)
{
	vtag.clear();
}

//optional field with a tag
template <class IE, class... IES>
struct seq_dec_imp<std::enable_if_t<
		is_optional_v<IE>
		&& !has_optional_type<IE>::value
		&& has_tag_type_v<IE>
	>,
	IE, IES...>
{
	template <class TO, class FUNC, class UNEXP, class TAG>
	static inline bool decode(TO& to, FUNC&& func, UNEXP& unexp, TAG& vtag)
	{
		//CODEC_TRACE("[%s]...", name<IE>());
		if (!vtag)
		{
			//save state before decoding a tag
			if (func.push_state())
			{
				if (auto const tag = decode_tag<typename IE::tag_type>(func))
				{
					vtag.set_encoded(tag.get_encoded());
					CODEC_TRACE("pop tag=%zx", tag.get_encoded());
				}
				else
				{
					return false;
				}
			}
			else
			{
				CODEC_TRACE("EoF at %s", name<IE>());
				return true; //end of buffer
			}
		}

		if (IE::tag_type::match(vtag.get_encoded())) //check tag decoded
		{
			CODEC_TRACE("T=%zx[%s]", vtag.get_encoded(), name<IE>());
			clear_tag<IE>(func, vtag); //clear current tag as decoded
			IE& ie = to;
			if (!med::decode(func, ie.ref_field(), unexp)) return false;
		}
		return seq_decoder<IES...>::decode(to, func, unexp, vtag);
	}
};

//optional field without a tag (optional by end of data)
template <class IE, class... IES>
struct seq_dec_imp<std::enable_if_t<
		is_optional_v<IE>
		&& !has_tag_type_v<IE>
		&& !has_optional_type<IE>::value
	>,
	IE, IES...>
{
	template <class TO, class FUNC, class UNEXP, class TAG>
	static inline bool decode(TO& to, FUNC&& func, UNEXP& unexp, TAG& vtag)
	{
		CODEC_TRACE("[%s]...", name<IE>());
		if (vtag)
		{
			CODEC_TRACE("discard tag=%zx", vtag.get_encoded());
			func.pop_state(); //restore state
			vtag.clear();
		}

		if (!func.eof())
		{
			IE& ie = to;
			return med::decode(func, ie.ref_field(), unexp)
				&& seq_decoder<IES...>::decode(to, func, unexp, vtag);
		}
		CODEC_TRACE("EOF at [%s]", name<IE>());
		return true; //end of buffer
	}
};

//optional field with a presence-functor
template <class IE, class... IES>
struct seq_dec_imp<std::enable_if_t<
		is_optional_v<IE>
		&& has_optional_type<IE>::value
	>,
	IE, IES...>
{
	template <class TO, class FUNC, class UNEXP, class TAG>
	static inline bool decode(TO& to, FUNC&& func, UNEXP& unexp, TAG& vtag)
	{
		CODEC_TRACE("[%s]...", name<IE>());
		if (typename IE::optional_type{}(to))
		{
			CODEC_TRACE("[%s]", name<IE>());
			IE& ie = to;

			if (vtag)
			{
				CODEC_TRACE("discard tag=%zx", vtag.get_encoded());
				func.pop_state(); //restore state
				vtag.clear();
			}

			if (!med::decode(func, ie.ref_field(), unexp)) return false;
		}
		return seq_decoder<IES...>::decode(to, func, unexp, vtag);
	}
};

template <>
struct seq_dec_imp<void>
{
	template <class TO, class FUNC, class UNEXP, class TAG>
	static bool decode(TO&, FUNC&&, UNEXP&, TAG&)               { return true; }
};


template <class Enable, class... IES>
struct seq_enc_imp;
template <class... IES>
using seq_encoder = seq_enc_imp<void, IES...>;

//mandatory field w/o setter
template <class IE, class... IES>
struct seq_enc_imp<std::enable_if_t<
		!is_optional_v<IE> &&
		!has_setter_type_v<IE>
	>,
	IE, IES...>
{
	template <class TO, class FUNC>
	static inline bool encode(TO const& to, FUNC&& func)
	{
		IE const& ie = to;
		CODEC_TRACE("%c{%s}", ie.ref_field().is_set()?'+':'-', class_name<IE>());
		if (ie.ref_field().is_set())
		{
			return med::encode(func, ie) && seq_encoder<IES...>::encode(to, func);
		}
		else
		{
			func(error::MISSING_IE, name<typename IE::field_type>(), 1, 0);
			return false;
		}
	}
};

//mandatory field w/ setter
template <class IE, class... IES>
struct seq_enc_imp<std::enable_if_t<
		!is_optional_v<IE> &&
		has_setter_type_v<IE>
	>,
	IE, IES...>
{
	template <class TO, class FUNC>
	static inline bool encode(TO const& to, FUNC&& func)
	{
		CODEC_TRACE("{%s} with setter", name<IE>());
		typename IE::setter_type{}(const_cast<TO&>(to)); //TODO: copy of IE to not const_cast?
		IE const& ie = to;
		if (ie.ref_field().is_set())
		{
			return med::encode(func, ie) && seq_encoder<IES...>::encode(to, func);
		}
		else
		{
			func(error::MISSING_IE, name<typename IE::field_type>(), 1, 0);
			return false;
		}
	}
};

//optional field
template <class IE, class... IES>
struct seq_enc_imp<std::enable_if_t<
		is_optional_v<IE>
	>,
	IE, IES...>
{
	template <class TO, class FUNC>
	static inline bool encode(TO const& to, FUNC&& func)
	{
		IE const& ie = to;
		CODEC_TRACE("%c[%s]", ie.ref_field().is_set()?'+':'-', name<IE>());
		if (ie.ref_field().is_set())
		{
			return med::encode(func, ie) && seq_encoder<IES...>::encode(to, func);
		}
		else
		{
			return seq_encoder<IES...>::encode(to, func);
		}
	}
};


template <>
struct seq_enc_imp<void>
{
	template <class TO, class FUNC>
	static inline bool encode(TO&, FUNC&& func)
	{
		return true;
	}
};

} //end: namespace sl

template <class ...IES>
struct sequence : container<IES...>
{
	template <class ENCODER>
	bool encode(ENCODER&& encoder) const
	{
		return sl::seq_encoder<IES...>::encode(this->m_ies, encoder);
	}

	template <class DECODER, class UNEXP>
	bool decode(DECODER&& decoder, UNEXP& unexp)
	{
		value<32> vtag; //TODO: any hint from sequence?
		return sl::seq_decoder<IES...>::decode(this->m_ies, decoder, unexp, vtag);
	}
};

} //end: namespace med
