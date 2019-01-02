/**
@file
Google JSON definitions

@copyright Denis Priyomov 2018
Distributed under the MIT License
(See accompanying file LICENSE or visit https://github.com/cppden/med)
*/

#pragma once

#include "../value.hpp"
#include "../octet_string.hpp"
#include "../set.hpp"
#include "../sequence.hpp"
#include "../buffer.hpp"
#include "../tag_named.hpp"
#include "../encoder_context.hpp"
#include "../decoder_context.hpp"

namespace med::json {

template <class N>
using T = tag_named<N>;

//string with hash
template <class ...T>
struct hash_string : octet_string<T...>
{
	using base_t = octet_string<T...>;
//	using base_t::set;
	using hash_type = uint64_t;

//	std::string_view get() const            { return std::string_view{(char const*)this->data(), this->size()}; }
//	bool set(char const* psz)               { return this->set_encoded(std::strlen(psz), psz); }

	hash_type get_tag() const                   { return get_hash(); }
//	auto set(hash_type v)                       { return set_encoded(v); }

	//static constexpr bool is_const = false;
	hash_type get_hash() const                  { return m_hash; }
	void set_hash(hash_type v)                  { m_hash = v; }

private:
	hash_type  m_hash{};
};

template <class, class Enable = void>
struct has_hash_type : std::false_type { };
template <class T>
struct has_hash_type<T, std::void_t<typename T::hash_type>> : std::true_type { };
template <class T>
constexpr bool has_hash_type_v = has_hash_type<T>::value;


using boolean = med::value<bool>;
using integer = med::value<int>;
using longint = med::value<long long int>;
using unsignedint = med::value<unsigned int>;
using unsignedlong = med::value<unsigned long long int>;
using number  = med::value<double>;
//using null = med::value<void>;
using string  = med::ascii_string<>;

template <class ...IEs>
struct object : set<hash_string<>, IEs...>{};

template <class ...IEs>
struct strict_object : sequence<IEs...>{};

using encoder_context = med::encoder_context<buffer<char*>>;
using decoder_context = med::decoder_context<buffer<char const*>>;

} //end: namespace med::json
