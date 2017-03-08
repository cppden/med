/**
@file
sequence IE container - any IE in strict sequential order

@copyright Denis Priyomov 2016-2017
Distributed under the MIT License
(See accompanying file LICENSE or visit https://github.com/cppden/med)
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

//single-instance mandatory field
template <class IE, class... IES>
struct seq_dec_imp<std::enable_if_t<
		!is_multi_field_v<IE> &&
		!is_optional_v<IE>
	>,
	IE, IES...>
{
	template <class TO, class FUNC, class UNEXP, class TAG>
	static inline MED_RESULT decode(TO& to, FUNC&& func, UNEXP& unexp, TAG& vtag)
	{
		CODEC_TRACE("{%s}...", name<IE>());
		IE& ie = to;
		return med::decode(func, ie, unexp) MED_AND seq_decoder<IES...>::decode(to, func, unexp, vtag);
	}
};

//TODO: more flexible is to compare current size read and how many bits to rewind for RO IE.
template <class IE, class FUNC, class TAG>
std::enable_if_t<is_peek_v<typename IE::tag_type>>
inline clear_tag(FUNC& func, TAG& vtag)
{
	func(POP_STATE{});
	vtag.clear();
}

template <class IE, class FUNC, class TAG>
std::enable_if_t<!is_peek_v<typename IE::tag_type>>
inline clear_tag(FUNC&, TAG& vtag)
{
	vtag.clear();
}

template <class FUNC, class TAG>
inline void discard(FUNC& func, TAG& vtag)
{
	if (vtag)
	{
		CODEC_TRACE("discard tag=%zx", vtag.get_encoded());
		func(POP_STATE{}); //restore state
		vtag.clear();
	}
}

//single-instance optional field w/ tag
template <class IE, class... IES>
struct seq_dec_imp<std::enable_if_t<
		!is_multi_field_v<IE> &&
		is_optional_v<IE> &&
		!has_condition_v<IE> &&
		has_tag_type_v<IE>
	>,
	IE, IES...>
{
	template <class TO, class FUNC, class UNEXP, class TAG>
	static inline MED_RESULT decode(TO& to, FUNC&& func, UNEXP& unexp, TAG& vtag)
	{
		//CODEC_TRACE("[%s]...", name<IE>());
		if (!vtag)
		{
			//save state before decoding a tag
			if (!func(PUSH_STATE{}))
			{
				CODEC_TRACE("EoF at %s", name<IE>());
				MED_RETURN_SUCCESS; //end of buffer
			}

			//convert const to writable
			using TAG_IE = typename IE::tag_type::writable;
			TAG_IE tag;
			MED_CHECK_FAIL(func(tag, typename TAG_IE::ie_type{}));
			vtag.set_encoded(tag.get_encoded());
			CODEC_TRACE("pop tag=%zx", tag.get_encoded());
		}

		if (IE::tag_type::match(vtag.get_encoded())) //check tag decoded
		{
			CODEC_TRACE("T=%zx[%s]", vtag.get_encoded(), name<IE>());
			clear_tag<IE>(func, vtag); //clear current tag as decoded
			IE& ie = to;
			MED_CHECK_FAIL(med::decode(func, ie.ref_field(), unexp));
		}
		return seq_decoder<IES...>::decode(to, func, unexp, vtag);
	}
};

//single-instance optional field w/o tag (optional by end of data)
template <class IE, class... IES>
struct seq_dec_imp<std::enable_if_t<
		!is_multi_field_v<IE> &&
		is_optional_v<IE> &&
		!has_tag_type_v<IE> &&
		!has_condition_v<IE>
	>,
	IE, IES...>
{
	template <class TO, class FUNC, class UNEXP, class TAG>
	static inline MED_RESULT decode(TO& to, FUNC&& func, UNEXP& unexp, TAG& vtag)
	{
		CODEC_TRACE("[%s]...", name<IE>());
		discard(func, vtag);

		if (!func(CHECK_STATE{}))
		{
			CODEC_TRACE("EOF at [%s]", name<IE>());
			MED_RETURN_SUCCESS; //end of buffer
		}
		IE& ie = to;
		return med::decode(func, ie.ref_field(), unexp)
			MED_AND seq_decoder<IES...>::decode(to, func, unexp, vtag);
	}
};

//single-instance conditional field
template <class IE, class... IES>
struct seq_dec_imp<std::enable_if_t<
		!is_multi_field_v<IE> &&
		is_optional_v<IE> &&
		has_condition_v<IE>
	>,
	IE, IES...>
{
	template <class TO, class FUNC, class UNEXP, class TAG>
	static inline MED_RESULT decode(TO& to, FUNC&& func, UNEXP& unexp, TAG& vtag)
	{
		if (typename IE::condition{}(to))
		{
			discard(func, vtag);

			CODEC_TRACE("C[%s]", name<IE>());
			IE& ie = to;
			MED_CHECK_FAIL(med::decode(func, ie.ref_field(), unexp));
		}
		else
		{
			CODEC_TRACE("skipped C[%s]", name<IE>());
		}
		return seq_decoder<IES...>::decode(to, func, unexp, vtag);
	}
};

//multi-instance conditional field
template <class IE, class... IES>
struct seq_dec_imp<std::enable_if_t<
		is_multi_field_v<IE> &&
		is_optional_v<IE> &&
		has_condition_v<IE>
	>,
	IE, IES...>
{
	template <class TO, class FUNC, class UNEXP, class TAG>
	static inline MED_RESULT decode(TO& to, FUNC&& func, UNEXP& unexp, TAG& vtag)
	{
		if (typename IE::condition{}(to))
		{
			discard(func, vtag);
			IE& ie = to;
			do
			{
				CODEC_TRACE("C[%s]#%zu", name<IE>(), ie.count());
				auto* field = ie.push_back(func);
				MED_CHECK_FAIL(field || !med::decode(func, *field, unexp));
			}
			while (typename IE::condition{}(to));

			MED_CHECK_FAIL(check_arity(func, ie));
		}
		else
		{
			CODEC_TRACE("skipped C[%s]", name<IE>());
		}
		return seq_decoder<IES...>::decode(to, func, unexp, vtag);
	}
};

//multi-instance optional or mandatory field w/ tag w/o counter
template <class IE, class... IES>
struct seq_dec_imp<std::enable_if_t<
		is_multi_field_v<IE> &&
		has_tag_type_v<IE> &&
		!has_count_getter_v<IE> &&
		!is_counter_v<IE> &&
		!has_condition_v<IE>
	>,
	IE, IES...>
{
	template <class TO, class FUNC, class UNEXP, class TAG>
	static inline MED_RESULT decode(TO& to, FUNC&& func, UNEXP& unexp, TAG& vtag)
	{
		IE& ie = to;

		if (!vtag && func(PUSH_STATE{}))
		{
			//convert const to writable
			using TAG_IE = typename IE::tag_type::writable;
			TAG_IE tag;
			MED_CHECK_FAIL(func(tag, typename TAG_IE::ie_type{}));
			vtag.set_encoded(tag.get_encoded());
			CODEC_TRACE("pop tag=%zx", vtag.get_encoded());
		}

		while (IE::tag_type::match(vtag.get_encoded()))
		{
			CODEC_TRACE("T=%zx[%s]*", vtag.get_encoded(), name<IE>());
			auto* field = ie.push_back(func);
			MED_CHECK_FAIL(MED_EXPR_AND(field) med::decode(func, *field, unexp));

			if (func(PUSH_STATE{}))
			{
				//convert const to writable
				using TAG_IE = typename IE::tag_type::writable;
				TAG_IE tag;
#ifdef MED_NO_EXCEPTION
				if (func(tag, typename TAG_IE::ie_type{}))
				{
					vtag.set_encoded(tag.get_encoded());
					CODEC_TRACE("pop tag=%zx", vtag.get_encoded());
				}
				else
				{
					vtag.clear();
				}
#else //!MED_NO_EXCEPTION
				//TODO: avoid try/catch
				try
				{
					func(tag, typename TAG_IE::ie_type{});
					vtag.set_encoded(tag.get_encoded());
					CODEC_TRACE("pop tag=%zx", vtag.get_encoded());
				}
				catch (med::exception const& ex)
				{
					vtag.clear();
				}
#endif //MED_NO_EXCEPTION
			}
			else
			{
				vtag.clear();
				break;
			}
		}

		if (!vtag)
		{
			func(POP_STATE{}); //restore state
			func(error::SUCCESS); //clear error
		}
		return check_arity(func, ie)
			MED_AND seq_decoder<IES...>::decode(to, func, unexp, vtag);
	}
};

//multi-instance field w/o tag, counter or count-getter
template <class IE, class... IES>
struct seq_dec_imp<std::enable_if_t<
		is_multi_field_v<IE> &&
		!has_tag_type_v<IE> &&
		!has_count_getter_v<IE> &&
		!is_counter_v<IE> &&
		!has_condition_v<IE>
	>,
	IE, IES...>
{
	template <class TO, class FUNC, class UNEXP, class TAG>
	static inline MED_RESULT decode(TO& to, FUNC&& func, UNEXP& unexp, TAG& vtag)
	{
		discard(func, vtag);

		IE& ie = to;
		CODEC_TRACE("[%s]*[%zu..%zu]", name<IE>(), IE::min, IE::max);

		std::size_t count = 0;
		while (func(CHECK_STATE{}) && count < IE::max)
		{
			auto* field = ie.push_back(func);
			MED_CHECK_FAIL(MED_EXPR_AND(field) sl::decode_ie<IE>(func, *field, typename IE::ie_type{}, unexp));
			++count;
		}

		return check_arity(func, ie, count)
			MED_AND seq_decoder<IES...>::decode(to, func, unexp, vtag);
	}
};


//multi-instance field w/o tag w/ count-getter
template <class IE, class... IES>
struct seq_dec_imp<
	std::enable_if_t<
		is_multi_field_v<IE> &&
		!has_tag_type_v<IE> &&
		has_count_getter_v<IE>
	>,
	IE, IES...
>
{
	template <class TO, class FUNC, class UNEXP, class TAG>
	static inline MED_RESULT decode(TO& to, FUNC&& func, UNEXP& unexp, TAG& vtag)
	{
		discard(func, vtag);
		CODEC_TRACE("[%s]...", name<IE>());

		IE& ie = to;

		std::size_t count = typename IE::count_getter{}(to);
		CODEC_TRACE("[%s]*%zu", name<IE>(), count);

		MED_CHECK_FAIL(check_arity(func, ie, count));

		while (count--)
		{
			auto* field = ie.push_back(func);
			MED_CHECK_FAIL(MED_EXPR_AND(field) med::decode(func, *field, unexp));
		}

		return seq_decoder<IES...>::decode(to, func, unexp, vtag);
	}
};

//multi-instance field w/o tag w/ counter
template <class IE, class... IES>
struct seq_dec_imp<
	std::enable_if_t<
		is_multi_field_v<IE> &&
		!has_tag_type_v<IE> &&
		is_counter_v<IE>
	>,
	IE, IES...
>
{
	template <class TO, class FUNC, class UNEXP, class TAG>
	static inline MED_RESULT decode(TO& to, FUNC&& func, UNEXP& unexp, TAG& vtag)
	{
		discard(func, vtag);
		CODEC_TRACE("[%s]...", name<IE>());

		IE& ie = to;
		typename IE::counter_type counter_ie;
		MED_CHECK_FAIL(med::decode(func, counter_ie));
		auto count = counter_ie.get_encoded();
		MED_CHECK_FAIL(check_arity(func, ie, count));
		while (count--)
		{
			CODEC_TRACE("[%s]*%zu", name<IE>(), count);

			auto* field = ie.push_back(func);
			MED_CHECK_FAIL(MED_EXPR_AND(field) med::decode(func, *field, unexp));
		}

		return seq_decoder<IES...>::decode(to, func, unexp, vtag);
	}
};


template <>
struct seq_dec_imp<void>
{
	template <class TO, class FUNC, class UNEXP, class TAG>
	static MED_RESULT decode(TO&, FUNC&&, UNEXP&, TAG&)  { MED_RETURN_SUCCESS; }
};


template <class Enable, class... IES>
struct seq_enc_imp;
template <class... IES>
using seq_encoder = seq_enc_imp<void, IES...>;

//single-instance mandatory field w/o setter
template <class IE, class... IES>
struct seq_enc_imp<std::enable_if_t<
		!is_multi_field_v<IE> &&
		!is_optional_v<IE> &&
		!has_setter_type_v<IE>
	>,
	IE, IES...>
{
	template <class TO, class FUNC>
	static inline MED_RESULT encode(TO const& to, FUNC&& func)
	{
		IE const& ie = to;
		CODEC_TRACE("%c{%s}", ie.ref_field().is_set()?'+':'-', class_name<IE>());
		if (ie.ref_field().is_set())
		{
			return med::encode(func, ie) MED_AND seq_encoder<IES...>::encode(to, func);
		}
		return func(error::MISSING_IE, name<typename IE::field_type>(), 1, 0);
	}
};

//single-instance mandatory field w/ setter
template <class IE, class... IES>
struct seq_enc_imp<std::enable_if_t<
		!is_multi_field_v<IE> &&
		!is_optional_v<IE> &&
		has_setter_type_v<IE>
	>,
	IE, IES...>
{
	template <class TO, class FUNC>
	static inline MED_RESULT encode(TO const& to, FUNC&& func)
	{
		CODEC_TRACE("{%s} with setter", name<IE>());
		typename IE::setter_type{}(const_cast<TO&>(to)); //TODO: copy of IE to not const_cast?
		IE const& ie = to;
		if (ie.ref_field().is_set())
		{
			return med::encode(func, ie) MED_AND seq_encoder<IES...>::encode(to, func);
		}
		return func(error::MISSING_IE, name<typename IE::field_type>(), 1, 0);
	}
};

template <class IE, class... IES>
struct seq_enc_imp<std::enable_if_t<
		!is_multi_field_v<IE> &&
		is_optional_v<IE>
	>,
	IE, IES...>
{
	template <class TO, class FUNC>
	static inline MED_RESULT encode(TO const& to, FUNC&& func)
	{
		IE const& ie = to;
		CODEC_TRACE("%c[%s]", ie.ref_field().is_set()?'+':'-', name<IE>());
		if (ie.ref_field().is_set())
		{
			return med::encode(func, ie) MED_AND seq_encoder<IES...>::encode(to, func);
		}
		return seq_encoder<IES...>::encode(to, func);
	}
};

template <class FUNC, class IE>
inline MED_RESULT encode_multi(FUNC& func, IE const& ie)
{
	CODEC_TRACE("%s *%zu", name<IE>(), ie.count());
	for (auto& field : ie)
	{
		CODEC_TRACE("[%s]%c", name<IE>(), field.is_set() ? '+':'-');
		if (field.is_set())
		{
			MED_CHECK_FAIL(sl::encode_ie<IE>(func, field, typename IE::ie_type{}));
		}
		else
		{
			return func(error::MISSING_IE, name<typename IE::field_type>(), ie.count(), ie.count() - 1);
		}
	}
	MED_RETURN_SUCCESS;
}


//multi-field w/o counter
template <class IE, class... IES>
struct seq_enc_imp<std::enable_if_t<
		is_multi_field_v<IE> &&
		!is_counter_v<IE>
	>,
	IE, IES...>
{
	template <class TO, class FUNC>
	static inline MED_RESULT encode(TO const& to, FUNC&& func)
	{
		IE const& ie = to;
		return encode_multi(func, ie)
			MED_AND check_arity(func, ie)
			MED_AND seq_encoder<IES...>::encode(to, func);
	}
};

//mandatory multi-field w/ counter w/o tag
template <class IE, class... IES>
struct seq_enc_imp<std::enable_if_t<
		is_multi_field_v<IE> &&
		is_counter_v<IE> &&
		!is_optional_v<IE> &&
		!has_tag_type_v<IE>
	>,
	IE, IES...>
{
	template <class TO, class FUNC>
	static inline MED_RESULT encode(TO const& to, FUNC&& func)
	{
		IE const& ie = to;
		CODEC_TRACE("CV[%s]=%zu", name<IE>(), ie.count());
		typename IE::counter_type counter_ie;
		counter_ie.set_encoded(ie.count());
		return check_arity(func, ie)
			MED_AND med::encode(func, counter_ie)
			MED_AND encode_multi(func, ie)
			MED_AND seq_encoder<IES...>::encode(to, func);
	}
};

//optional multi-field w/ counter w/o tag
template <class IE, class... IES>
struct seq_enc_imp<std::enable_if_t<
		is_multi_field_v<IE> &&
		is_counter_v<IE> &&
		is_optional_v<IE> &&
		!has_tag_type_v<IE>
	>,
	IE, IES...>
{
	template <class TO, class FUNC>
	static inline MED_RESULT encode(TO const& to, FUNC&& func)
	{
		IE const& ie = to;
		std::size_t const count = field_count(ie);
		CODEC_TRACE("CV[%s]=%zu", name<IE>(), ie.count());
		if (count > 0)
		{
			typename IE::counter_type counter_ie;
			counter_ie.set_encoded(ie.count());

			return check_arity(func, ie)
				MED_AND med::encode(func, counter_ie)
				MED_AND encode_multi(func, ie)
				MED_AND seq_encoder<IES...>::encode(to, func);
		}
		return seq_encoder<IES...>::encode(to, func);
	}
};

template <>
struct seq_enc_imp<void>
{
	template <class TO, class FUNC>
	static inline MED_RESULT encode(TO&, FUNC&&)
	{
		MED_RETURN_SUCCESS;
	}
};

}	//end: namespace sl

template <class ...IES>
struct sequence : container<IES...>
{
	template <class ENCODER>
	MED_RESULT encode(ENCODER&& encoder) const
	{
		return sl::seq_encoder<IES...>::encode(this->m_ies, encoder);
	}

	template <class DECODER, class UNEXP>
	MED_RESULT decode(DECODER&& decoder, UNEXP& unexp)
	{
		value<std::size_t> vtag; //TODO: any clue from sequence?
		return sl::seq_decoder<IES...>::decode(this->m_ies, decoder, unexp, vtag);
	}
};


} //end: namespace med

