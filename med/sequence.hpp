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

namespace med {

namespace sl {

//TODO: more flexible is to compare current size read and how many bits to rewind for RO IE.
template <class IE, class FUNC, class TAG>
inline void clear_tag(FUNC& func, TAG& vtag)
{
	if constexpr (is_peek_v<typename IE::tag_type>)
	{
		func(POP_STATE{});
		vtag.clear();
	}
	else
	{
		vtag.clear();
	}
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
inline MED_RESULT encode_multi(FUNC& func, IE const& ie)
{
	CODEC_TRACE("%s *%zu", name<IE>(), ie.count());
	for (auto& field : ie)
	{
		CODEC_TRACE("[%s]%c", name<IE>(), field.is_set() ? '+':'-');
		if (field.is_set())
		{
			MED_CHECK_FAIL(sl::encode_ie<IE>(func, field, typename IE::ie_type{}));
			if constexpr (codec_e::STRUCTURED == get_codec_kind_v<FUNC>)
			{
				if (ie.last() != &field)
				{
					MED_CHECK_FAIL(func(ie, NEXT_CONTAINER_ELEMENT{}));
				}
			}
		}
		else
		{
			return func(error::MISSING_IE, name<IE>(), ie.count(), ie.count() - 1);
		}
	}
	MED_RETURN_SUCCESS;
}


template <class... IES>
struct seq_for;

template <class IE, class... IES>
struct seq_for<IE, IES...>
{
	template <class PREV_IE, class TO, class FUNC, class UNEXP, class TAG>
	static inline MED_RESULT decode(TO& to, FUNC& func, UNEXP& unexp, TAG& vtag)
	{
		IE& ie = to;
		if constexpr (is_multi_field_v<IE>)
		{
			if constexpr (has_tag_type_v<IE>) //mulit-field with tag
			{
				//multi-instance optional or mandatory field w/ tag w/o counter
				static_assert(!has_count_getter_v<IE> && !is_counter_v<IE> && !has_condition_v<IE>, "TO IMPLEMENT!");

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
					CODEC_TRACE("->T=%zx[%s]*%zu", vtag.get_encoded(), name<IE>(), ie.count());
					auto* field = ie.push_back(func);
					MED_CHECK_FAIL(MED_EXPR_AND(field) med::decode(func, *field, unexp));

					if (func(PUSH_STATE{})) //not at the end
					{
						//convert const to writable
						using TAG_IE = typename IE::tag_type::writable;
						TAG_IE tag;
#if (MED_EXCEPTIONS)
						//TODO: avoid try/catch
//						try
//						{
							func(tag, typename TAG_IE::ie_type{});
							vtag.set_encoded(tag.get_encoded());
							CODEC_TRACE("pop tag=%zx", vtag.get_encoded());
//						}
//						catch (med::overflow const& ex)
//						{
//							vtag.clear();
//						}
#else
						if (func(tag, typename TAG_IE::ie_type{}))
						{
							vtag.set_encoded(tag.get_encoded());
							CODEC_TRACE("pop tag=%zx", vtag.get_encoded());
						}
						else
						{
							vtag.clear();
						}
#endif //MED_EXCEPTIONS
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
					func(POP_STATE{}); //restore state
					func(error::SUCCESS); //clear error
				}
				MED_CHECK_FAIL(check_arity(func, ie));
			}
			else //multi-field w/o tag
			{
				if constexpr (has_tag_type_v<PREV_IE>)
				{
					discard(func, vtag);
				}

				if constexpr (has_count_getter_v<IE>) //multi-field w/ count-getter
				{
					std::size_t count = typename IE::count_getter{}(to);
					CODEC_TRACE("[%s]*%zu", name<IE>(), count);
					MED_CHECK_FAIL(check_arity(func, ie, count));
					while (count--)
					{
						auto* field = ie.push_back(func);
						MED_CHECK_FAIL(MED_EXPR_AND(field) med::decode(func, *field, unexp));
					}
				}
				else if constexpr (is_counter_v<IE>) //multi-field w/ counter
				{
					typename IE::counter_type counter_ie;
					MED_CHECK_FAIL(med::decode(func, counter_ie));
					auto count = counter_ie.get_encoded();
					CODEC_TRACE("[%s] = %zu", name<IE>(), std::size_t(count));
					MED_CHECK_FAIL(check_arity(func, ie, count));
					while (count--)
					{
						auto* field = ie.push_back(func);
						CODEC_TRACE("#%zu = %p", std::size_t(count), (void*)field);
						MED_CHECK_FAIL(MED_EXPR_AND(field) med::decode(func, *field, unexp));
					}
				}
				else if constexpr (has_condition_v<IE>) //conditional multi-field
				{
					if (typename IE::condition{}(to))
					{
						do
						{
							CODEC_TRACE("C[%s]#%zu", name<IE>(), ie.count());
							auto* field = ie.push_back(func);
							MED_CHECK_FAIL(MED_EXPR_AND(field) med::decode(func, *field, unexp));
						}
						while (typename IE::condition{}(to));

						MED_CHECK_FAIL(check_arity(func, ie));
					}
					else
					{
						CODEC_TRACE("skipped C[%s]", name<IE>());
					}
				}
				else
				{
					CODEC_TRACE("[%s]*[%zu..%zu]: %s", name<IE>(), IE::min, IE::max, class_name<FUNC>());
					std::size_t count = 0;
					while (func(CHECK_STATE{}, ie) && count < IE::max)
					{
						if (auto* field = ie.push_back(func))
						{
							if constexpr (codec_e::STRUCTURED == get_codec_kind_v<FUNC>)
							{
								if (count > 0)
								{
									MED_CHECK_FAIL(func(ie, NEXT_CONTAINER_ELEMENT{}));
								}
							}
							MED_CHECK_FAIL(sl::decode_ie<IE>(func, *field, typename IE::ie_type{}, unexp));
						}
#if (!MED_EXCEPTIONS)
						else
						{
							return func(error::OUT_OF_MEMORY, name<IE>());
						}
#endif
						++count;
					}

					MED_CHECK_FAIL(check_arity(func, ie, count));
				}
			}
		}
		else //single-instance field
		{
			if constexpr (is_optional_v<IE>)
			{
				if constexpr (has_tag_type_v<IE>) //optional field with tag
				{
					//read a tag or use the tag read before
					if (!vtag)
					{
						//save state before decoding a tag
						if (not func(PUSH_STATE{}))
						{
							CODEC_TRACE("EoF at %s", name<IE>());
							MED_RETURN_SUCCESS; //end of buffer
						}

						//convert const to writable
						using TAG_IE = typename IE::tag_type::writable;
						TAG_IE tag;
						MED_CHECK_FAIL(func(tag, typename TAG_IE::ie_type{}));
						vtag.set_encoded(tag.get_encoded());
						CODEC_TRACE("pop tag=%zx", std::size_t(tag.get_encoded()));
					}

					if (IE::tag_type::match(vtag.get_encoded())) //check tag decoded
					{
						CODEC_TRACE("T=%zx[%s]", std::size_t(vtag.get_encoded()), name<IE>());
						clear_tag<IE>(func, vtag); //clear current tag as decoded
						MED_CHECK_FAIL(med::decode(func, ie.ref_field(), unexp));
					}
				}
				else //optional w/o tag
				{
					//discard tag if needed
					if constexpr (is_optional_v<PREV_IE> and has_tag_type_v<PREV_IE>)
					{
						discard(func, vtag);
					}

					if constexpr (has_condition_v<IE>) //conditional field
					{
						bool const was_set = typename IE::condition{}(to);
						if (was_set || has_default_value_v<IE>)
						{
							CODEC_TRACE("C[%s]", name<IE>());
							MED_CHECK_FAIL(med::decode(func, ie, unexp));
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

						if (not func(CHECK_STATE{}, ie))
						{
							CODEC_TRACE("EOF at [%s]", name<IE>());
							MED_RETURN_SUCCESS; //end of buffer
						}
						MED_CHECK_FAIL(med::decode(func, ie.ref_field(), unexp));
					}
				}
			}
			else //mandatory field
			{
				CODEC_TRACE("{%s}...", name<IE>());

				//if switched from optional with tag then discard it since mandatory is read as whole
				if constexpr (is_optional_v<PREV_IE> and has_tag_type_v<PREV_IE>)
				{
					discard(func, vtag);
				}
				MED_CHECK_FAIL(med::decode(func, ie, unexp));
			}
		}
		return seq_for<IES...>::template decode<IE>(to, func, unexp, vtag);
	}

	template <class PREV_IE, class TO, class FUNC>
	static inline MED_RESULT encode(TO const& to, FUNC& func)
	{
		if constexpr (is_multi_field_v<IE>)
		{
			if constexpr (is_counter_v<IE>)
			{
				static_assert(!has_tag_type_v<IE>, "missed case");
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
						MED_CHECK_FAIL(check_arity(func, ie));
						MED_CHECK_FAIL(med::encode(func, counter_ie));
						MED_CHECK_FAIL(encode_multi(func, ie));
					}
				}
				//mandatory multi-field w/ counter w/o tag
				else
				{
					IE const& ie = to;
					CODEC_TRACE("CV{%s}=%zu", name<IE>(), ie.count());
					typename IE::counter_type counter_ie;
					counter_ie.set_encoded(ie.count());
					MED_CHECK_FAIL(check_arity(func, ie));
					MED_CHECK_FAIL(med::encode(func, counter_ie));
					MED_CHECK_FAIL(encode_multi(func, ie));
				}
			}
			else //multi-field w/o counter
			{
				IE const& ie = to;
				MED_CHECK_FAIL(encode_multi(func, ie));
				MED_CHECK_FAIL(check_arity(func, ie));
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
					MED_CHECK_FAIL(ie.copy(static_cast<IE const&>(to), func));

					typename IE::setter_type setter;
					if constexpr (std::is_same_v<bool, decltype(setter(ie, to))>)
					{
						if (not setter(ie, to))
						{
							MED_RETURN_ERROR(error::INVALID_VALUE, func, name<IE>(), ie.get());
						}
					}
					else
					{
						setter(ie, to);
					}
					if (ie.ref_field().is_set())
					{
						MED_CHECK_FAIL(med::encode(func, ie));
					}
				}
				else //w/o setter
				{
					IE const& ie = to;
					CODEC_TRACE("%c[%s]", ie.ref_field().is_set()?'+':'-', name<IE>());
					if (ie.ref_field().is_set() || has_default_value_v<IE>)
					{
						MED_CHECK_FAIL(med::encode(func, ie));
					}
				}
			}
			else //mandatory field
			{
				if constexpr (has_setter_type_v<IE>) //with setter
				{
					CODEC_TRACE("{%s} with setter", name<IE>());
					IE ie;
					MED_CHECK_FAIL(ie.copy(static_cast<IE const&>(to), func));

					typename IE::setter_type setter;
					if constexpr (std::is_same_v<bool, decltype(setter(ie, to))>)
					{
						if (not setter(ie, to))
						{
							MED_RETURN_ERROR(error::INVALID_VALUE, func, name<IE>(), ie.get());
						}
					}
					else
					{
						setter(ie, to);
					}
					if (ie.ref_field().is_set())
					{
						MED_CHECK_FAIL(med::encode(func, ie));
					}
					else
					{
						return func(error::MISSING_IE, name<IE>(), 1, 0);
					}
				}
				else //w/o setter
				{
					IE const& ie = to;
					CODEC_TRACE("%c{%s}", ie.ref_field().is_set()?'+':'-', class_name<IE>());
					if (ie.ref_field().is_set())
					{
						MED_CHECK_FAIL(med::encode(func, ie));
					}
					else
					{
						return func(error::MISSING_IE, name<IE>(), 1, 0);
					}
				}
			}
		}
		return seq_for<IES...>::template encode<IE>(to, func);
	}
};

template <>
struct seq_for<>
{
	template <class PREV_IE, class TO, class FUNC, class UNEXP, class TAG>
	static constexpr MED_RESULT decode(TO&, FUNC&&, UNEXP&, TAG&)
	{
		MED_RETURN_SUCCESS;
	}

	template <class PREV_IE, class TO, class FUNC>
	static constexpr MED_RESULT encode(TO&, FUNC&&)
	{
		MED_RETURN_SUCCESS;
	}
};

}	//end: namespace sl

template <class ...IES>
struct sequence : container<IES...>
{
	static constexpr bool ordered = true; //used only in JSON, smth more useful?

	template <class ENCODER>
	MED_RESULT encode(ENCODER&& encoder) const
	{
		return sl::seq_for<IES...>::template encode<void>(this->m_ies, encoder);
	}

	template <class DECODER, class UNEXP>
	MED_RESULT decode(DECODER&& decoder, UNEXP& unexp)
	{
		value<std::size_t> vtag; //TODO: any clue from sequence?
		return sl::seq_for<IES...>::template decode<void>(this->m_ies, decoder, unexp, vtag);
	}
};


} //end: namespace med

