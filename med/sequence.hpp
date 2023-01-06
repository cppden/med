/**
@file
sequence IE container - any IE in strict sequential order

@copyright Denis Priyomov 2016-2017
Distributed under the MIT License
(See accompanying file LICENSE or visit https://github.com/cppden/med)
*/

#pragma once

#include "config.hpp"
#include "state.hpp"
#include "container.hpp"
#include "encode.hpp"
#include "decode.hpp"
#include "debug.hpp"
#include "meta/typelist.hpp"
#include "meta/foreach.hpp"

namespace med {

namespace sl {

//TODO: more flexible is to compare current size read and how many bits to rewind for RO IE.
template <class IE, class FUNC>
constexpr void clear_tag(FUNC& func, auto& vtag)
{
	using type = get_meta_tag_t<meta::produce_info_t<FUNC, IE>>;
	if constexpr (is_peek_v<type>)
	{
		func(POP_STATE{});
	}
	vtag.clear();
}

constexpr void discard(auto& func, auto& vtag)
{
	if (vtag)
	{
		CODEC_TRACE("discard tag=%zx", vtag.get_encoded());
		func(POP_STATE{}); //restore state
		vtag.clear();
	}
}

template <class FUNC, class IE>
constexpr void encode_multi(FUNC& func, IE const& ie)
{
	using mi = meta::produce_info_t<FUNC, typename IE::field_type>; //assuming MI of multi_field == MI of field
	using ctx = type_context<mi>;

	CODEC_TRACE("%s *%zu", name<IE>(), ie.count());
	for (auto& field : ie)
	{
		CODEC_TRACE("[%s]%c", name<IE>(), field.is_set() ? '+':'-');
		if (field.is_set())
		{
			ie_encode<ctx>(func, field);
		}
		else
		{
			MED_THROW_EXCEPTION(missing_ie, name<IE>(), ie.count(), ie.count() - 1)
		}
	}
}

struct seq_dec
{
	template <class CTX, class PREV_IE, class IE, class TO, class DECODER>
	static constexpr void apply(TO& to, DECODER& decoder, auto& vtag, auto&... deps)
	{
		IE& ie = to;
		using mi = meta::produce_info_t<DECODER, IE>;
		using type = get_meta_tag_t<mi>;
		using EXP_TAG = typename CTX::explicit_tag_type;
		using EXP_LEN = typename CTX::explicit_length_type;
		using ctx = type_context<mi, EXP_TAG, EXP_LEN>;

		if constexpr (AMultiField<IE>)
		{
			CODEC_TRACE("[%s]*", name<IE>());
			if constexpr (not std::is_void_v<type>) //multi-field with tag
			{
				//multi-instance optional or mandatory field w/ tag w/o counter
				static_assert(!AHasCountGetter<IE> && !ACounter<IE> && !AHasCondition<IE>, "TO IMPLEMENT!");

				if (!vtag && decoder(PUSH_STATE{}, ie))
				{
					vtag.set_encoded(decode_tag<type, false>(decoder));
					CODEC_TRACE("pop tag=%zx", vtag.get_encoded());
				}

				while (type::match(vtag.get_encoded()))
				{
					CODEC_TRACE("->T=%zx[%s]*%zu", vtag.get_encoded(), name<IE>(), ie.count()+1);
					auto* field = ie.push_back(decoder);
					using ctx_next = type_context<meta::list_rest_t<mi>, EXP_TAG, EXP_LEN>;
					ie_decode<ctx_next>(decoder, *field, deps...);

					if (decoder(PUSH_STATE{}, ie)) //not at the end
					{
						vtag.set_encoded(decode_tag<type, false>(decoder));
						CODEC_TRACE("pop tag=%zx", vtag.get_encoded());
					}
					else //end is reached
					{
						CODEC_TRACE("<-T=%zx[%s]*", vtag.get_encoded(), name<IE>());
						vtag.clear();
						break;
					}
				}

				if (!vtag) { decoder(POP_STATE{}); } //restore state
				check_arity(decoder, ie);
			}
			else //multi-field w/o tag
			{
				using prev_tag_t = get_meta_tag_t<meta::produce_info_t<DECODER, PREV_IE>>;
				if constexpr (not std::is_void_v<prev_tag_t>) { discard(decoder, vtag); }

				//multi-field w/ count-getter or counter
				if constexpr (AHasCountGetter<IE> || ACounter<IE>)
				{
					auto count = [&]
					{
						if constexpr(AHasCountGetter<IE>)
						{
							return typename IE::count_getter{}(to);
						}
						else
						{
							typename IE::counter_type counter_ie;
							ie_decode<type_context<>>(decoder, counter_ie);
							return counter_ie.get_encoded();
						}
					}();

					CODEC_TRACE("[%s] CNT=%zu", name<IE>(), std::size_t(count));
					check_arity(decoder, ie, count);
					while (count--)
					{
						auto* field = ie.push_back(decoder);
						CODEC_TRACE("#%zu = %p", std::size_t(count), (void*)field);
						med::decode(decoder, *field, deps...);
					}
				}
				else if constexpr (AHasCondition<IE>) //conditional multi-field
				{
					if (typename IE::condition{}(to))
					{
						do
						{
							CODEC_TRACE("C[%s]#%zu", name<IE>(), ie.count());
							auto* field = ie.push_back(decoder);
							med::decode(decoder, *field, deps...);
						}
						while (typename IE::condition{}(to));

						check_arity(decoder, ie);
					}
					else
					{
						CODEC_TRACE("skipped C[%s]", name<IE>());
					}
				}
				else
				{
					CODEC_TRACE("[%s]*[%zu..%zu]: %s", name<IE>(), IE::min, IE::max, class_name<DECODER>());
					std::size_t count = 0;
					while (decoder(CHECK_STATE{}, ie) && count < IE::max)
					{
						auto* field = ie.push_back(decoder);
						ie_decode<ctx>(decoder, *field, deps...);
						++count;
					}

					check_arity(decoder, ie);
				}
			}
		}
		else //single-instance field
		{
			if constexpr (AOptional<IE>)
			{
				if constexpr (not std::is_void_v<type>) //optional field with tag
				{
					CODEC_TRACE("O<%s> w/TAG %s", name<IE>(), class_name<type>());
					//read a tag or use the tag read before
					if (!vtag)
					{
						//save state before decoding a tag
						if (decoder(PUSH_STATE{}, ie))
						{
							//don't save state as we just did it already
							vtag.set_encoded(decode_tag<type, false>(decoder));
							CODEC_TRACE("read tag=%zx", std::size_t(vtag.get_encoded()));
						}
						else
						{
							CODEC_TRACE("EoF at %s", name<IE>());
							return; //end of buffer
						}
					}

					if (type::match(vtag.get_encoded())) //check tag decoded
					{
						CODEC_TRACE("T=%zx[%s]", std::size_t(vtag.get_encoded()), name<IE>());
						clear_tag<IE>(decoder, vtag); //clear current tag as decoded
						using ctx_next = type_context<meta::list_rest_t<mi>, EXP_TAG, EXP_LEN>;
						ie_decode<ctx_next>(decoder, ie, deps...);
					}
				}
				else //optional w/o tag
				{
					CODEC_TRACE("O<%s> w/o TAG", name<IE>());
					//discard tag if needed
					using prev_tag_t = get_meta_tag_t<meta::produce_info_t<DECODER, PREV_IE>>;
					if constexpr (AOptional<PREV_IE> and not std::is_void_v<prev_tag_t>)
					{
						discard(decoder, vtag);
					}

					if constexpr (AHasCondition<IE>) //conditional field
					{
						bool const was_set = typename IE::condition{}(to);
						if (was_set)
						{
							CODEC_TRACE("C[%s]", name<IE>());
							ie_decode<ctx>(decoder, ie, deps...);
						}
						else
						{
							CODEC_TRACE("skipped C[%s]", name<IE>());
						}
					}
					else //optional field w/o tag (optional by end of data)
					{
						CODEC_TRACE("[%s]...", name<IE>());
						if (decoder(CHECK_STATE{}, ie))
						{
							ie_decode<ctx>(decoder, ie, deps...);
						}
						else
						{
							CODEC_TRACE("EOF at [%s]", name<IE>());
							return; //end of buffer
						}
					}
				}
			}
			else //mandatory field
			{
				CODEC_TRACE("M<%s> explicit=<%s:%s>...", name<IE>(), name<EXP_TAG>(), name<EXP_LEN>());

				//if switched from optional with tag then discard it since mandatory is read as whole
				using prev_tag_t = get_meta_tag_t<meta::produce_info_t<DECODER, PREV_IE>>;
				if constexpr (AOptional<PREV_IE> and not std::is_void_v<prev_tag_t>)
				{
					discard(decoder, vtag);
				}
				ie_decode<ctx>(decoder, ie, deps...);
			}
		}
	}
};

struct seq_enc
{
	template <class CTX, class PREV_IE, class IE>
	static constexpr void apply(auto const& to, auto& encoder)
	{
		IE const& ie = to;
		if constexpr (AMultiField<IE>)
		{
			if constexpr (ACounter<IE>)
			{
				//optional multi-field w/ counter w/o tag
				if constexpr (AOptional<IE>)
				{
					std::size_t const count = field_count(ie);
					CODEC_TRACE("CV[%s]=%zu", name<IE>(), ie.count());
					if (count > 0)
					{
						typename IE::counter_type counter_ie;
						counter_ie.set_encoded(ie.count());
						check_arity(encoder, ie);
						med::encode(encoder, counter_ie);
						encode_multi(encoder, ie);
					}
				}
				//mandatory multi-field w/ counter w/o tag
				else
				{
					CODEC_TRACE("CV{%s}=%zu", name<IE>(), ie.count());
					typename IE::counter_type counter_ie;
					counter_ie.set_encoded(ie.count());
					check_arity(encoder, ie);
					med::encode(encoder, counter_ie);
					encode_multi(encoder, ie);
				}
			}
			else //multi-field w/o counter
			{
				encode_multi(encoder, ie);
				check_arity(encoder, ie);
			}
		}
		else //single-instance field
		{
			if constexpr (AOptional<IE>) //optional field
			{
				if constexpr (AHasSetterType<IE>) //with setter
				{
					CODEC_TRACE("[%s] with setter", name<IE>());
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

					if (ie.is_set())
					{
						med::encode(encoder, ie);
					}
				}
				else //w/o setter
				{
					CODEC_TRACE("%c[%s]", ie.is_set()?'+':'-', name<IE>());
					if (ie.is_set())
					{
						med::encode(encoder, ie);
					}
				}
			}
			else //mandatory field
			{
				if constexpr (AHasSetterType<IE>) //with setter
				{
					CODEC_TRACE("{%s} with setter", name<IE>());
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
					if (ie.is_set())
					{
						med::encode(encoder, ie);
					}
					else
					{
						MED_THROW_EXCEPTION(missing_ie, name<IE>(), 1, 0)
					}
				}
				else //w/o setter
				{
					//CODEC_TRACE("%c{%s}", ie.is_set()?'+':'-', class_name<IE>());
					if (AHasSetLength<IE> || ie.is_set())
					{
						med::encode(encoder, ie);
					}
					else
					{
						MED_THROW_EXCEPTION(missing_ie, name<IE>(), 1, 0)
					}
				}
			}
		}
	}
};

}	//end: namespace sl


template <class ...IES>
struct sequence : container<IES...>
{
	using ies_types = typename container<IES...>::ies_types;

	template <class IE_LIST>
	void encode(auto& encoder) const
	{
		meta::foreach_prev<IE_LIST, void>(sl::seq_enc{}, this->m_ies, encoder);
	}
	void encode(auto& encoder) const { encode<ies_types>(encoder); }

	template <class IE_LIST, class TYPE_CTX = type_context<>>
	void decode(auto& decoder, auto&... deps)
	{
		value<std::size_t> vtag;
		meta::foreach_prev<IE_LIST, TYPE_CTX>(sl::seq_dec{}, this->m_ies, decoder, vtag, deps...);
	}
	void decode(auto& decoder, auto&... deps) { decode<ies_types>(decoder, deps...); }
};

} //end: namespace med
