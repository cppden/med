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
#include "value.hpp"
#include "meta/typelist.hpp"


namespace med {

//structure layer
namespace sl {

//Tag
template <class TAG_TYPE>
constexpr auto decode_tag(auto& decoder)
{
	TAG_TYPE ie;
	typename as_writable_t<TAG_TYPE>::value_type value{};
	value = decoder(ie, IE_TAG{});
	CODEC_TRACE("%s=0x%zX(%zu) [%s]", __FUNCTION__, std::size_t(value), std::size_t(value), class_name<TAG_TYPE>());
	return value;
}
//Length
template <class LEN_TYPE>
constexpr std::size_t decode_len(auto& decoder)
{
	LEN_TYPE ie;
	decoder(ie, IE_LEN{});
	return value_to_length(ie);
}

template <class TYPE_CTX, class DECODER, class IE, class... DEPS>
constexpr void ie_decode(DECODER& decoder, IE& ie, DEPS&... deps);


template <class LEN_TYPE, class TYPE_CTX, class DECODER, class IE, class... DEPS>
void apply_len(DECODER& decoder, IE& ie, DEPS&... deps)
{
	using META_INFO = typename TYPE_CTX::meta_info_type;
	using EXP_TAG = typename TYPE_CTX::explicit_tag_type;
	using EXP_LEN = typename TYPE_CTX::explicit_length_type;

	using pad_traits = typename get_padding<LEN_TYPE>::type;
	using dependency_t = get_dependency_t<LEN_TYPE>;
	CODEC_TRACE("%s[%s]<%s:%s>(%zu) - %s", __FUNCTION__, name<IE>(), name<EXP_TAG>()
		, name<EXP_LEN>(), sizeof...(deps), name<dependency_t>());

	if constexpr (std::is_void_v<dependency_t>)
	{
		using ctx = type_context<typename TYPE_CTX::ie_type, META_INFO, EXP_TAG, EXP_LEN>;

		if constexpr (not std::is_void_v<EXP_LEN>)
		{
			//!!! len is not yet available when explicit
			auto end = decoder(PUSH_SIZE{0, false});
			if constexpr (std::is_void_v<pad_traits>)
			{
				ie_decode<ctx>(decoder, ie, end, deps...);
				//TODO: ??? as warning not error
				if (0 != end.size()) { MED_THROW_EXCEPTION(overflow, name<IE>(), end.size()); }
			}
			else
			{
				CODEC_TRACE("padded %s...:", name<LEN_TYPE>());
				using pad_t = typename DECODER::template padder_type<pad_traits, DECODER>;
				pad_t pad{decoder};
				ie_decode<ctx>(decoder, ie, end, deps...);
				CODEC_TRACE("before padding %s: end=%zu padding=%u", name<LEN_TYPE>(), end.size(), pad.padding_size());
				//if (end.size() != pad.padding_size()) { MED_THROW_EXCEPTION(overflow, name<IE>(), end.size()); }
				end.restore_end();
				pad.add_padding();
			}
		}
		else
		{
			auto const len = decode_len<LEN_TYPE>(decoder);
			auto end = decoder(PUSH_SIZE{len});
			if constexpr (std::is_void_v<pad_traits>)
			{
				ie_decode<ctx>(decoder, ie, deps...);
				//TODO: ??? as warning not error
				if (0 != end.size()) { MED_THROW_EXCEPTION(overflow, name<IE>(), end.size()); }
			}
			else
			{
				CODEC_TRACE("padded %s...:", name<LEN_TYPE>());
				using pad_t = typename DECODER::template padder_type<pad_traits, DECODER>;
				pad_t pad{decoder};
				ie_decode<ctx>(decoder, ie, deps...);
				CODEC_TRACE("before padding %s: end=%zu padding=%u", name<LEN_TYPE>(), end.size(), pad.padding_size());
				//if (end.size() != pad.padding_size()) { MED_THROW_EXCEPTION(overflow, name<IE>(), end.size()); }
				end.restore_end();
				pad.add_padding();
			}
		}
	}
	else
	{
		static_assert(sizeof...(deps) == 0, "NO RECURSION OR MIX w/EXPLICIT LENGTH");
		CODEC_TRACE("'%s' depends on '%s'", name<LEN_TYPE>(), name<dependency_t>());

		using DEPENDENT = LEN_TYPE;
		using DEPENDENCY = dependency_t;
		using ctx = type_context<typename TYPE_CTX::ie_type, META_INFO, EXP_TAG, EXP_LEN, DEPENDENT, DEPENDENCY>;

		auto const len = decode_len<LEN_TYPE>(decoder);
		auto end = decoder(PUSH_SIZE{len, false});
		if constexpr (std::is_void_v<pad_traits>)
		{
			ie_decode<ctx>(decoder, ie, end, deps...);
			//TODO: ??? as warning not error
			CODEC_TRACE("decoded '%s' depends on '%s'", name<LEN_TYPE>(), name<dependency_t>());
			if (0 != end.size()) { MED_THROW_EXCEPTION(overflow, name<IE>(), end.size()); }
		}
		else
		{
			CODEC_TRACE("padded %s...:", name<LEN_TYPE>());
			using pad_t = typename DECODER::template padder_type<pad_traits, DECODER>;
			pad_t pad{decoder};
			ie_decode<ctx>(decoder, ie, end, deps...);
			CODEC_TRACE("decoded '%s' depends on '%s'", name<LEN_TYPE>(), name<dependency_t>());
			//if (end.size() != pad.padding_size()) { MED_THROW_EXCEPTION(overflow, name<IE>(), end.size()); }
			end.restore_end();
			pad.add_padding();
		}
	}
}

constexpr void explicit_len_commit(auto&, auto&, auto&) {}

template <class IE>
constexpr void explicit_len_commit(auto& decoder, IE& ie, auto& end, auto start)
{
	decoder(ie, typename IE::ie_type{});
	auto const after_len = decoder(GET_STATE{});
	auto const delta = after_len - start;
	auto len = value_to_length(ie) + delta;
	CODEC_TRACE("%s<%s>=%zu (delta=%ld)", __FUNCTION__, name<IE>(), len, delta);
	end.commit(len);
}


template <class DEPENDENT, class DEPENDENCY, class DEPS>
constexpr void invoke_dependency(DEPENDENCY const& ie, DEPS& deps)
{
	deps.commit(DEPENDENT::dependency(ie));
}

template <class TYPE_CTX, class DECODER, class IE, class... DEPS>
constexpr void ie_decode(DECODER& decoder, IE& ie, DEPS&... deps)
{
	using META_INFO = typename TYPE_CTX::meta_info_type;
	using EXP_TAG = typename TYPE_CTX::explicit_tag_type;
	using EXP_LEN = typename TYPE_CTX::explicit_length_type;

	if constexpr (not meta::list_is_empty_v<META_INFO>)
	{
		static_assert(std::is_void_v<typename TYPE_CTX::dependent_type>);
		static_assert(std::is_void_v<typename TYPE_CTX::dependency_type>);

		using mi = meta::list_first_t<META_INFO>;
		using mi_rest = meta::list_rest_t<META_INFO>;
		CODEC_TRACE("%s[%s]<%s:%s>(%zu) w/ meta=%s", __FUNCTION__, name<IE>(), name<EXP_TAG>(), name<EXP_LEN>(), sizeof... (deps), class_name<mi>());
		using info_t = get_info_t<mi>;

		if constexpr (mi::kind == mik::LEN)
		{
			if constexpr (APresentIn<info_t, IE>)
			{
				static_assert(sizeof...(deps) == 0);
				auto const start = decoder(GET_STATE{});
				CODEC_TRACE("->> explicit len [%s]", name<info_t>());
				apply_len<info_t, type_context<typename TYPE_CTX::ie_type, mi_rest, EXP_TAG, info_t>>(decoder, ie, start);
			}
			else
			{
				apply_len<info_t, type_context<typename TYPE_CTX::ie_type, mi_rest, EXP_TAG, EXP_LEN>>(decoder, ie, deps...);
			}
		}
		else
		{
			static_assert(mi::kind == mik::TAG);
			auto const tag = decode_tag<info_t>(decoder);
			if (not info_t::match(tag))
			{
				//NOTE: this can only be called for mandatory field thus it's fail case (not unexpected)
				MED_THROW_EXCEPTION(unknown_tag, name<IE>(), tag)
			}
			ie_decode<type_context<typename TYPE_CTX::ie_type, mi_rest, EXP_TAG, EXP_LEN>>(decoder, ie, deps...);
		}
	}
	else
	{
		using ie_type = typename IE::ie_type;
		if constexpr (AContainer<IE>)
		{
			CODEC_TRACE(">>> %s<%s:%s>", name<IE>(), name<EXP_TAG>(), name<EXP_LEN>());
			if constexpr (not std::is_void_v<EXP_TAG>)
			{
				static_assert(std::is_void_v<EXP_LEN>);
				static_assert(std::is_same_v<EXP_TAG, get_field_type_t<meta::list_first_t<typename IE::ies_types>>>);
				/* NOTE: 1st IE is expected to be explicit so it s.b. skipped as was decoded in meta */
				ie.template decode<meta::list_rest_t<typename IE::ies_types>>(decoder, deps...);
			}
			else if constexpr (not std::is_void_v<EXP_LEN>)
			{
				/* NOTE: explicit length is valid for sequence only */
				using ctx = type_context<typename TYPE_CTX::ie_type, meta::typelist<>, void, EXP_LEN>;
				ie.template decode<typename IE::ies_types, ctx>(decoder, deps...);
			}
			else
			{
				ie.decode(decoder, deps...);
			}
			CODEC_TRACE("<<< %s<%s:%s>", name<IE>(), name<EXP_TAG>(), name<EXP_LEN>());
		}
		else
		{
			using field_t = get_field_type_t<IE>;
			CODEC_TRACE("PRIMITIVE %s[%.30s](%zu): %s <%s:%s>", __FUNCTION__, name<IE>(), sizeof...(deps), name<ie_type>(), name<EXP_TAG>(), name<EXP_LEN>());

			if constexpr (std::is_same_v<field_t, EXP_LEN>)
			{
				explicit_len_commit(decoder, ie, deps...);
			}
			else
			{
				decoder(ie, ie_type{});
			}
		}
	}
}

}	//end: namespace sl

template <class DECODER, AHasIeType IE, class... DEPS>
constexpr void decode(DECODER&& decoder, IE& ie, DEPS&... deps)
{
	using META_INFO = meta::produce_info_t<DECODER, IE>;
	sl::ie_decode<type_context<typename IE::ie_type, META_INFO>>(decoder, ie, deps...);
}

}	//end: namespace med
