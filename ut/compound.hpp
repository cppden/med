#pragma once

#include "ut.hpp"

namespace cmp {

struct U8  : med::value<uint8_t>{};
struct U16 : med::value<uint16_t>{};
struct U24 : med::value<med::bytes<3>> {};
struct U32 : med::value<uint32_t>{};

struct code : U16 {};
struct length : U16 {};

template <code::value_type CODE = 0, class BODY = med::empty<>>
struct hdr :
	med::sequence<
		med::placeholder::_length<0>,
		M< med::value<med::fixed<CODE, code::value_type>> >,
		M< BODY >
	>
{
	BODY const& body() const              { return this->template get<BODY>(); }
	BODY& body()                          { return this->template ref<BODY>(); }
};

template <>
struct hdr<0, med::empty<>> :
	med::sequence<
		M<length>,
		M<code>
	>
{
	std::size_t get_tag() const           { return this->template get<code>().get(); }
	void set_tag(std::size_t val)         { this->template ref<code>().set(val); }
};


template <code::value_type CODE, class BODY>
struct avp : hdr<CODE, BODY>, med::tag_t<med::value<med::fixed<CODE, code::value_type>>>
{
	using length_type = length;

	bool is_set() const                   { return this->body().is_set(); }
	//using hdr<CODE, BODY>::get;
	decltype(auto) get() const            { return this->body().get(); }
	template <typename... ARGS>
	auto set(ARGS&&... args)              { return this->body().set(std::forward<ARGS>(args)...); }
};

struct string : avp<0x1, med::ascii_string<>> {};
struct number : avp<0x2, med::value<uint32_t>> {};

} //end: namespace cmp

