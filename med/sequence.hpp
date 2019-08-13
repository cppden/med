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
template <class IE, class FUNC, class TAG>
inline void clear_tag(FUNC& func, TAG& vtag)
{
	using tag_type = med::meta::unwrap_t<decltype(FUNC::template get_tag_type<IE>())>;
	if constexpr (is_peek_v<tag_type>)
	{
		func(POP_STATE{});
	}
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

template <class FUNC, class IE>
inline void encode_multi(FUNC& func, IE const& ie)
{
	CODEC_TRACE("%s *%zu", name<IE>(), ie.count());
	for (auto& field : ie)
	{
		CODEC_TRACE("[%s]%c", name<IE>(), field.is_set() ? '+':'-');
		if (field.is_set())
		{
			sl::encode_ie<IE>(func, field, typename IE::ie_type{});
			if constexpr (is_callable_with_v<FUNC, NEXT_CONTAINER_ELEMENT>)
			{
				if (ie.last() != &field) { func(NEXT_CONTAINER_ELEMENT{}, ie); }
			}
		}
		else
		{
			MED_THROW_EXCEPTION(missing_ie, name<IE>(), ie.count(), ie.count() - 1)
		}
	}
}

struct seq_dec
{
	template <class PREV_IE, class IE, class TO, class DECODER, class UNEXP, class TAG>
	static constexpr void apply(TO& to, DECODER& decoder, UNEXP& unexp, TAG& vtag)
	{
		IE& ie = to;
		if constexpr (is_multi_field_v<IE>)
		{
			CODEC_TRACE("[%s]*", name<IE>());
			using tag_type = med::meta::unwrap_t<decltype(DECODER::template get_tag_type<IE>())>;
			if constexpr (not std::is_void_v<tag_type>) //multi-field with tag
			{
				//multi-instance optional or mandatory field w/ tag w/o counter
				static_assert(!has_count_getter_v<IE> && !is_counter_v<IE> && !has_condition_v<IE>, "TO IMPLEMENT!");

				if (!vtag && decoder(PUSH_STATE{}, ie))
				{
					vtag.set_encoded(decode_tag<tag_type, false>(decoder));
					CODEC_TRACE("pop tag=%zx", vtag.get_encoded());
				}

				while (tag_type::match(vtag.get_encoded()))
				{
					CODEC_TRACE("->T=%zx[%s]*%zu", vtag.get_encoded(), name<IE>(), ie.count());
					auto* field = ie.push_back(decoder);
					med::decode(decoder, *field, unexp);

					if (decoder(PUSH_STATE{}, ie)) //not at the end
					{
						vtag.set_encoded(decode_tag<tag_type, false>(decoder));
						CODEC_TRACE("pop tag=%zx", vtag.get_encoded());
					}
					else //end is reached
					{
						CODEC_TRACE("<-T=%zx[%s]*", vtag.get_encoded(), name<IE>());
						vtag.clear();
						break;
					}
				}

				if (!vtag)
				{
					decoder(POP_STATE{}); //restore state
				}
				check_arity(decoder, ie);
			}
			else //multi-field w/o tag
			{
				using prev_tag_type = meta::unwrap_t<decltype(DECODER::template get_tag_type<PREV_IE>())>;
				if constexpr (not std::is_void_v<prev_tag_type>)
				{
					discard(decoder, vtag);
				}

				if constexpr (has_count_getter_v<IE>) //multi-field w/ count-getter
				{
					std::size_t count = typename IE::count_getter{}(to);
					CODEC_TRACE("[%s]*%zu", name<IE>(), count);
					check_arity(decoder, ie, count);
					while (count--)
					{
						auto* field = ie.push_back(decoder);
						med::decode(decoder, *field, unexp);
					}
				}
				else if constexpr (is_counter_v<IE>) //multi-field w/ counter
				{
					typename IE::counter_type counter_ie;
					med::decode(decoder, counter_ie);
					auto count = counter_ie.get_encoded();
					CODEC_TRACE("[%s] CNT=%zu", name<IE>(), std::size_t(count));
					check_arity(decoder, ie, count);
					while (count--)
					{
						auto* field = ie.push_back(decoder);
						CODEC_TRACE("#%zu = %p", std::size_t(count), (void*)field);
						med::decode(decoder, *field, unexp);
					}
				}
				else if constexpr (has_condition_v<IE>) //conditional multi-field
				{
					if (typename IE::condition{}(to))
					{
						do
						{
							CODEC_TRACE("C[%s]#%zu", name<IE>(), ie.count());
							auto* field = ie.push_back(decoder);
							med::decode(decoder, *field, unexp);
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
						if constexpr (is_callable_with_v<DECODER, NEXT_CONTAINER_ELEMENT>)
						{
							if (count > 0) { decoder(NEXT_CONTAINER_ELEMENT{}, ie); }
						}
						sl::decode_ie<IE>(decoder, *field, typename IE::ie_type{}, unexp);
						++count;
					}

					check_arity(decoder, ie, count);
				}
			}
		}
		else //single-instance field
		{
			if constexpr (is_optional_v<IE>)
			{
				using tag_type = med::meta::unwrap_t<decltype(DECODER::template get_tag_type<IE>())>;
				if constexpr (not std::is_void_v<tag_type>) //optional field with tag
				{
					CODEC_TRACE("O<%s> w/TAG", name<IE>());
					//read a tag or use the tag read before
					if (!vtag)
					{
						//save state before decoding a tag
						if (decoder(PUSH_STATE{}, ie))
						{
							//don't save state as we just did it already
							vtag.set_encoded(decode_tag<tag_type, false>(decoder));
							CODEC_TRACE("pop tag=%zx", std::size_t(vtag.get_encoded()));
						}
						else
						{
							CODEC_TRACE("EoF at %s", name<IE>());
							return; //end of buffer
						}
					}

					if (tag_type::match(vtag.get_encoded())) //check tag decoded
					{
						CODEC_TRACE("T=%zx[%s]", std::size_t(vtag.get_encoded()), name<IE>());
						clear_tag<IE>(decoder, vtag); //clear current tag as decoded
						med::decode(decoder, ie.ref_field(), unexp);
					}
				}
				else //optional w/o tag
				{
					CODEC_TRACE("O<%s> w/o TAG", name<IE>());
					//discard tag if needed
					using prev_tag_type = meta::unwrap_t<decltype(DECODER::template get_tag_type<PREV_IE>())>;
					if constexpr (is_optional_v<PREV_IE> and not std::is_void_v<prev_tag_type>)
					{
						discard(decoder, vtag);
					}

					if constexpr (has_condition_v<IE>) //conditional field
					{
						bool const was_set = typename IE::condition{}(to);
						if (was_set || has_default_value_v<IE>)
						{
							CODEC_TRACE("C[%s]", name<IE>());
							med::decode(decoder, ie, unexp);
							if constexpr (has_default_value_v<IE>)
							{
								if (not was_set) { ie.ref_field().clear(); } //discard since it's a default
							}
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
							med::decode(decoder, ie, unexp);
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
				CODEC_TRACE("M<%s>...", name<IE>());

				//if switched from optional with tag then discard it since mandatory is read as whole
				using prev_tag_type = meta::unwrap_t<decltype(DECODER::template get_tag_type<PREV_IE>())>;
				if constexpr (is_optional_v<PREV_IE> and not std::is_void_v<prev_tag_type>)
				{
					discard(decoder, vtag);
				}
				med::decode(decoder, ie, unexp);
			}
		}
	}

	template <class PREV_IE, class TO, class DECODER, class UNEXP, class TAG>
	static constexpr void apply(TO&, DECODER&, UNEXP&, TAG&) {}
};

struct seq_enc
{
	template <class PREV_IE, class IE, class TO, class ENCODER>
	static constexpr void apply(TO const& to, ENCODER& encoder)
	{
		if constexpr (is_multi_field_v<IE>)
		{
			if constexpr (is_counter_v<IE>)
			{
				//static_assert(!ENCODER::template has_tag_type<IE>(), "missed case");
				//optional multi-field w/ counter w/o tag
				if constexpr (is_optional_v<IE>)
				{
					IE const& ie = to;
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
					IE const& ie = to;
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
				IE const& ie = to;
				encode_multi(encoder, ie);
				check_arity(encoder, ie);
			}
		}
		else //single-instance field
		{
			if constexpr (is_optional_v<IE>) //optional field
			{
				if constexpr (has_setter_type_v<IE>) //with setter
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

					if (ie.ref_field().is_set())
					{
						med::encode(encoder, ie);
					}
				}
				else //w/o setter
				{
					IE const& ie = to;
					CODEC_TRACE("%c[%s]", ie.ref_field().is_set()?'+':'-', name<IE>());
					if (ie.ref_field().is_set() || has_default_value_v<IE>)
					{
						med::encode(encoder, ie);
					}
				}
			}
			else //mandatory field
			{
				if constexpr (has_setter_type_v<IE>) //with setter
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
					if (ie.ref_field().is_set())
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
					IE const& ie = to;
					CODEC_TRACE("%c{%s}", ie.ref_field().is_set()?'+':'-', class_name<IE>());
					if (med::detail::has_set_length<IE>::value || ie.ref_field().is_set())
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

	template <class PREV_IE, class TO, class ENCODER>
	static constexpr void apply(TO const&, ENCODER&) {}
};

}	//end: namespace sl

template <class TRAITS, class ...IES>
struct base_sequence : container<TRAITS, IES...>
{
	using ies_types = typename container<TRAITS, IES...>::ies_types;

	template <class ENCODER>
	void encode(ENCODER& encoder) const
	{
		meta::foreach_prev<ies_types>(sl::seq_enc{}, this->m_ies, encoder);
	}

	template <class DECODER, class UNEXP>
	void decode(DECODER& decoder, UNEXP& unexp)
	{
		value<std::size_t> vtag;
		meta::foreach_prev<ies_types>(sl::seq_dec{}, this->m_ies, decoder, unexp, vtag);
	}
};

template <class ...IES>
using sequence = base_sequence<base_traits, IES...>;

} //end: namespace med

