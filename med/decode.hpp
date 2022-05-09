/**
@file
decoding entry point

@copyright Denis Priyomov 2016-2017
Distributed under the MIT License
(See accompanying file LICENSE or visit https://github.com/cppden/med)
*/

#pragma once

#include <utility>

#include "exception.hpp"
#include "optional.hpp"
#include "state.hpp"
#include "padding.hpp"
#include "length.hpp"
#include "name.hpp"
#include "placeholder.hpp"
#include "value.hpp"
#include "meta/typelist.hpp"


namespace med {

//structure layer
namespace sl {

//Tag
//PEEK_SAVE - to preserve state in case of peek tag or not
template <class TAG_TYPE, bool PEEK_SAVE, class DECODER>
constexpr auto decode_tag(DECODER& decoder)
{
	TAG_TYPE ie;
	typename as_writable_t<TAG_TYPE>::value_type value{};
	if constexpr (PEEK_SAVE && is_peek_v<TAG_TYPE>)
	{
		CODEC_TRACE("PEEK TAG %s", class_name<TAG_TYPE>());
		if (decoder(PUSH_STATE{}, ie))
		{
			value = decoder(ie, IE_TAG{});
			decoder(POP_STATE{});
		}
	}
	else
	{
		value = decoder(ie, IE_TAG{});
	}
	CODEC_TRACE("%s=%zX [%s]", __FUNCTION__, std::size_t(value), class_name<TAG_TYPE>());
	return value;
}
//Length
template <class LEN_TYPE, class DECODER>
constexpr std::size_t decode_len(DECODER& decoder)
{
	LEN_TYPE ie;
	if constexpr (is_peek_v<LEN_TYPE>)
	{
		CODEC_TRACE("PEEK LEN %s", name<LEN_TYPE>());
		if (decoder(PUSH_STATE{}, ie))
		{
			decoder(ie, IE_LEN{});
			decoder(POP_STATE{});
		}
	}
	else
	{
		decoder(ie, IE_LEN{});
	}
	std::size_t value = ie.get_encoded();
	CODEC_TRACE("%s=%zX [%s]", __FUNCTION__, value, class_name<LEN_TYPE>());
	value_to_length(ie, value);
	return value;
}

template <bool REF_BY_IE, class DECODER, class IE>
struct length_decoder;

template <class DECODER, class IE>
struct len_dec_impl
{
	using state_type = typename DECODER::state_type;
	using size_state = typename DECODER::size_state;
	using allocator_type = typename DECODER::allocator_type;
	template <class... PA>
	using padder_type = typename DECODER::template padder_type<PA...>;

	using length_type = typename IE::length_type;

	len_dec_impl(DECODER& decoder, IE& ie) noexcept
		: m_decoder{ decoder }
		, m_ie{ ie }
		, m_start{ m_decoder(GET_STATE{}) }
	{
		CODEC_TRACE("start %s with len=%s...:", name<IE>(), name<length_type>());
	}

#ifdef CODEC_TRACE_ENABLE
	~len_dec_impl() noexcept
	{
		CODEC_TRACE("finish %s with len=%s...:", name<IE>(), name<length_type>());
	}
#endif

	//check if placeholder was visited
	explicit operator bool() const                     { return static_cast<bool>(m_size_state); }
	auto size() const                                  { return m_size_state.size(); }

	//forward regular types to decoder
	template <class... Args> //NOTE: decltype is needed to expose actually defined operators
	auto operator() (Args&&... args) -> decltype(std::declval<DECODER>()(std::forward<Args>(args)...))
	{
		return m_decoder(std::forward<Args>(args)...);
	}

	template <class T>
	static constexpr auto produce_meta_info()          { return DECODER::template produce_meta_info<T>(); }

	allocator_type& get_allocator()                    { return m_decoder.get_allocator(); }

protected:
	//reduce size of the buffer for current length and elements from the start of IE
	template <int DELTA>
	void calc_buf_len(std::size_t& len_value) const
	{
		if constexpr (DELTA < 0)
		{
			if (len_value < std::size_t(-DELTA))
			{
				CODEC_TRACE("len=%zu less than delta=%+d", len_value, DELTA);
				MED_THROW_EXCEPTION(invalid_value, name<IE>(), len_value/*, m_decoder.get_context().buffer()*/);
			}
		}
		len_value += DELTA;
		auto const state_delta = std::size_t(m_decoder(GET_STATE{}) - m_start);
		CODEC_TRACE("len<%d>=%zu state-delta=%zu", DELTA, len_value, state_delta);
		if (state_delta > len_value)
		{
			CODEC_TRACE("len=%zu less than buffer has been advanced=%zu", len_value, state_delta);
			MED_THROW_EXCEPTION(invalid_value, name<IE>(), len_value/*, m_decoder.get_context().buffer()*/);
		}
		len_value -= state_delta;
	}

	template <int DELTA>
	void decode_len_ie(length_type& len_ie)
	{
		decode(m_decoder, len_ie);
		std::size_t len_value = 0;
		value_to_length(len_ie, len_value);
		calc_buf_len<DELTA>(len_value);
		m_size_state = m_decoder(PUSH_SIZE{len_value});
	}


	DECODER&         m_decoder;
	IE&              m_ie;
	state_type const m_start;
	size_state       m_size_state; //to switch end of buffer for this IE
};

template <class DEPENDENT, class DEPENDENCY, class DEPS>
constexpr void invoke_dependency(DEPENDENCY const& ie, DEPS& deps)
{
	//TODO: extend to multiple deps/types? now only one length
	deps.commit(DEPENDENT::dependency(ie));
}


template <class DECODER, class IE>
struct length_decoder<false, DECODER, IE> : len_dec_impl<DECODER, IE>
{
	template <class... NEWDEPS>
	using make_dependent = typename DECODER::template make_dependent<NEWDEPS...>;
	using length_type = typename IE::length_type;
	using len_dec_impl<DECODER, IE>::len_dec_impl;
	using len_dec_impl<DECODER, IE>::operator();

	template <int DELTA>
	void operator() (placeholder::_length<DELTA>&)
	{
		CODEC_TRACE("len_dec[%s] %+d by placeholder", name<length_type>(), DELTA);
		length_type len_ie;
		return this->template decode_len_ie<DELTA>(len_ie);
	}
};

template <class DECODER, class IE>
struct length_decoder<true, DECODER, IE> : len_dec_impl<DECODER, IE>
{
	template <class... NEWDEPS>
	using make_dependent = typename DECODER::template make_dependent<NEWDEPS...>;
	using length_type = typename IE::length_type;
	using len_dec_impl<DECODER, IE>::len_dec_impl;
	using len_dec_impl<DECODER, IE>::operator();

	//length position by exact type match
	template <class T, class ...ARGS>
	constexpr auto operator () (T& len_ie, ARGS&&...) ->
		std::enable_if_t<std::is_same_v<get_field_type_t<T>, length_type>, void>
	{
		CODEC_TRACE("len_dec[%s] by IE", name<length_type>());
		//don't count the size of length IE itself just like with regular LV
		constexpr auto len_ie_size = +DECODER::template size_of<length_type>();
		return this->template decode_len_ie<len_ie_size>(len_ie);
	}
};

template <class EXPOSED, class DECODER, class IE, class... DEPS>
constexpr void container_decoder(DECODER& decoder, IE& ie, DEPS&... deps)
{
	call_if<is_callable_with_v<DECODER, ENTRY_CONTAINER>>::call(decoder, ENTRY_CONTAINER{}, ie);

	if constexpr (is_length_v<IE>) //container with length_type defined
	{
		using length_type = typename IE::length_type;
		using pad_traits = typename get_padding<length_type>::type;

		if constexpr (std::is_void_v<pad_traits>)
		{
			length_decoder<IE::template has<length_type>(), DECODER, IE> ld{ decoder, ie};
			ie.decode(ld, deps...);
		}
		else
		{
			using pad_t = typename DECODER::template padder_type<pad_traits, DECODER>;
			pad_t pad{decoder};
			{
				length_decoder<IE::template has<length_type>(), DECODER, IE> ld{decoder, ie};
				ie.decode(ld, deps...);
				//special case for empty elements w/o length placeholder
				pad.enable_padding(static_cast<bool>(ld));
				CODEC_TRACE("%sable padding for len=%zu", ld?"en":"dis", ld.size());
			}
			pad.add_padding();
			//TODO: treat this case as warning? happens only in case of last IE with padding ended prematuraly
			//if (std::size_t const left = ld.size() - padding_size(pad))
			//MED_THROW_EXCEPTION(overflow, name<IE>(), left)
		}
	}
	else
	{
		CODEC_TRACE("%s w/o length exposed=%s...:", name<IE>(), name<EXPOSED>());
		if constexpr (std::is_void_v<EXPOSED>)
		{
			ie.decode(decoder, deps...);
		}
		else
		{
			ie.template decode<meta::list_rest_t<typename IE::ies_types>>(decoder, deps...);
		}
	}

	call_if<is_callable_with_v<DECODER, EXIT_CONTAINER>>::call(decoder, EXIT_CONTAINER{}, ie);
}

template <class META_INFO, class EXPOSED, class DECODER, class IE, class... DEPS>
constexpr void ie_decode(DECODER& decoder, IE& ie, DEPS&... deps)
{
	if constexpr (not meta::list_is_empty_v<META_INFO>)
	{
		using mi = meta::list_first_t<META_INFO>;
		using mi_rest = meta::list_rest_t<META_INFO>;
		CODEC_TRACE("%s[%s-%s](%zu) w/ %s", __FUNCTION__, name<IE>(), name<EXPOSED>(), sizeof... (deps), class_name<mi>());

		if constexpr (mi::kind == mik::TAG)
		{
			auto const tag = decode_tag<mi, true>(decoder);
			if (not mi::match(tag))
			{
				//NOTE: this can only be called for mandatory field thus it's fail case (not unexpected)
				MED_THROW_EXCEPTION(unknown_tag, name<IE>(), tag)
			}
		}
		else if constexpr (mi::kind == mik::LEN)
		{
			using length_type = typename mi::length_type;
			using pad_traits = typename get_padding<length_type>::type;

			auto len = decode_len<length_type>(decoder);

			using dependency_type = get_dependency_t<length_type>;
			if constexpr (std::is_void_v<dependency_type>)
			{
				//TODO: may have fixed length like in BER:NULL/BOOL so need to check here
				auto end = decoder(PUSH_SIZE{len});
				if constexpr (std::is_void_v<pad_traits>)
				{
					ie_decode<mi_rest, EXPOSED>(decoder, ie, deps...);
					//TODO: ??? as warning not error
					if (0 != end.size()) { MED_THROW_EXCEPTION(overflow, name<IE>(), end.size()); }
				}
				else
				{
					CODEC_TRACE("padded len_type=%s...:", name<length_type>());
					using pad_t = typename DECODER::template padder_type<pad_traits, DECODER>;
					pad_t pad{decoder};
					ie_decode<mi_rest, EXPOSED>(decoder, ie, deps...);
					if (0 != end.size()) { MED_THROW_EXCEPTION(overflow, name<IE>(), end.size()); }
					end.restore_end();
					pad.add_padding();
				}
				return;
			}
			else
			{
				CODEC_TRACE("'%s' depends on '%s'", name<length_type>(), name<dependency_type>());

				using dep_decoder_t = typename DECODER::template make_dependent<length_type, dependency_type>;
				dep_decoder_t dep_decoder{decoder.get_context()};

				auto end = dep_decoder(PUSH_SIZE{len, false});
				if constexpr (std::is_void_v<pad_traits>)
				{
					ie_decode<mi_rest, EXPOSED>(dep_decoder, ie, end, deps...);
					//TODO: ??? as warning not error
					CODEC_TRACE("decoded '%s' depends on '%s'", name<length_type>(), name<dependency_type>());
					if (0 != end.size()) { MED_THROW_EXCEPTION(overflow, name<IE>(), end.size()); }
				}
				else
				{
					CODEC_TRACE("padded %s...:", name<length_type>());
					using pad_t = typename DECODER::template padder_type<pad_traits, DECODER>;
					pad_t pad{dep_decoder};
					ie_decode<mi_rest, EXPOSED>(dep_decoder, ie, end, deps...);
					CODEC_TRACE("decoded '%s' depends on '%s'", name<length_type>(), name<dependency_type>());
					if (0 != end.size()) { MED_THROW_EXCEPTION(overflow, name<IE>(), end.size()); }
					end.restore_end();
					pad.add_padding();
				}
				return;
			}
		}

		ie_decode<mi_rest, EXPOSED>(decoder, ie, deps...);
	}
	else
	{
		using ie_type = typename IE::ie_type;
		CODEC_TRACE("%s[%.30s](%zu): %s", __FUNCTION__, class_name<IE>(), sizeof... (deps), class_name<ie_type>());
		if constexpr (std::is_base_of_v<CONTAINER, ie_type>)
		{
			container_decoder<EXPOSED>(decoder, ie, deps...);
		}
		else
		{
			if constexpr (is_peek_v<IE>)
			{
				CODEC_TRACE("PEEK %s[%s]", __FUNCTION__, name<IE>());
				if (decoder(PUSH_STATE{}, ie))
				{
					decoder(ie, ie_type{});
					decoder(POP_STATE{});
				}
			}
			else
			{
				using dependency_type = get_dependency_t<DECODER>;

				CODEC_TRACE("PRIMITIVE %s[%s] w/ deps '%s'", __FUNCTION__, name<IE>(), class_name<dependency_type>());
				decoder(ie, ie_type{});
				if constexpr (std::is_same_v<get_field_type_t<IE>, dependency_type>)
				{
					using dependent_type = typename DECODER::dependent_type;
					CODEC_TRACE("decoded dependency '%s' of dependent '%s' deps=%zu", name<IE>(), class_name<dependent_type>(), sizeof...(DEPS));
					invoke_dependency<dependent_type, dependency_type>(ie, deps...);
				}
			}
		}
	}
}

}	//end: namespace sl

template <class DECODER, class IE, class... DEPS>
constexpr void decode(DECODER&& decoder, IE& ie, DEPS&... deps)
{
	if constexpr (has_ie_type_v<IE>)
	{
		using mi = meta::produce_info_t<DECODER, IE>;
		sl::ie_decode<mi, void>(decoder, ie, deps...);
	}
	else //special cases like passing a placeholder
	{
		decoder(ie);
	}
}

}	//end: namespace med
