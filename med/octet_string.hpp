/**
@file
octet string IE definition

@copyright Denis Priyomov 2016-2018
Distributed under the MIT License
(See accompanying file LICENSE or visit https://github.com/cppden/med)
*/

#pragma once

#include <utility>
#include <string_view>

#include "field.hpp"
#include "debug.hpp"
#include "value_traits.hpp"

namespace med {

template <std::size_t MIN, std::size_t MAX>
struct octets
{
	template <typename TO, typename FROM, class Enable = std::enable_if_t<sizeof(TO) == sizeof(FROM)>>
	static void copy(TO* out, FROM const* in, std::size_t size)
	{
		if constexpr (MIN != MAX) //varying string
		{
			std::memcpy(out, in, size);
		}
		else //fixed string
		{
			(void)size;
			copy_impl(out, in, std::make_index_sequence<MIN>{});
		}
	}

private:
	template <typename TO, typename FROM>
	static constexpr void copy_octet(TO*, FROM const*) { }

	template <typename TO, typename FROM, std::size_t OFS, std::size_t... Is>
	static void copy_octet(TO* out, FROM const* in)
	{
		out[OFS] = in[OFS];
		copy_octet<TO, FROM, Is...>(out, in);
	}

	template <typename TO, typename FROM, std::size_t... Is>
	static void copy_impl(TO* out, FROM const* in, std::index_sequence<Is...>)
	{
		copy_octet<TO, FROM, Is...>(out, in);
	}
};

//variable length octets with external storage
class octets_var_extern
{
public:
	bool is_set() const                         { return m_data != nullptr; }

	std::size_t size() const                    { return m_size; }
	uint8_t const* data() const                 { return m_data; }

	void clear()                                { m_data = nullptr; m_size = 0; }
	void assign(void const* b_, void const* e_)
	{
		m_data = static_cast<uint8_t const*>(b_);
		m_size = num_octs_t(static_cast<uint8_t const*>(e_) - m_data);
	}

private:
	uint8_t const* m_data {nullptr};
	num_octs_t     m_size {0}; //not using size_t to reduce layout size
};

//variable length octets with internal storage
template <std::size_t MAX_LEN>
class octets_var_intern
{
public:
	bool is_set() const                         { return m_is_set; }

	std::size_t size() const                    { return m_size; }
	void resize(std::size_t v)                  { m_size = num_octs_t((v <= MAX_LEN) ? v : MAX_LEN); m_is_set = true; }
	uint8_t const* data() const                 { return is_set() ? m_data : nullptr; }
	uint8_t* data()                             { return m_data; }
	void clear()                                { m_size = 0; m_is_set = false; }

	//let external data to be set externally (risky but more efficient)
	uint8_t* emplace(std::size_t num)           { resize(num); return data(); }

	//limits are tested externally (octet_string_impl::set_encoded)
	void assign(uint8_t const* beg_, uint8_t const* end_)
	{
		m_size = num_octs_t(end_ - beg_);
		octets<0, MAX_LEN>::copy(m_data, beg_, m_size);
		m_is_set = true;
	}

private:
	uint8_t     m_data[MAX_LEN];
	num_octs_t  m_size {0};
	bool        m_is_set {false};
};

//fixed length octets with external storage
template <std::size_t LEN>
class octets_fix_extern
{
public:
	bool is_set() const                         { return nullptr != data(); }

	constexpr std::size_t size() const          { return LEN; }
	uint8_t const* data() const                 { return m_data; }

	void clear()                                { m_data = nullptr; }
	void assign(uint8_t const* p, void const*)  { m_data = p; }

private:
	uint8_t const* m_data {nullptr};
};

//fixed length octets with internal storage
template <std::size_t LEN>
class octets_fix_intern
{
public:
	bool is_set() const                         { return m_is_set; }

	static constexpr std::size_t size()         { return LEN; }
	uint8_t const* data() const                 { return m_data; }
	uint8_t* data()                             { return m_data; }

	void clear()                                { m_is_set = false; }
	//let external data to be set externally (risky but more efficient)
	uint8_t* emplace()                          { m_is_set = true; return data(); }

	//copy external data
	void assign(uint8_t const* p, void const*)  { octets<LEN, LEN>::copy(m_data, p, 0); m_is_set = true; }

private:
	uint8_t     m_data[LEN];
	bool        m_is_set {false};
};


template <class TRAITS, class VALUE = octets_var_extern>
struct octet_string_impl : IE<IE_OCTET_STRING>
{
	using traits     = TRAITS;
	using elem_type  = typename traits::value_type;
	using value_type = VALUE;
	using base_t = octet_string_impl;

	constexpr std::size_t size() const          { return m_value.size(); }

	elem_type const* data() const               { return m_value.data(); }
	using const_iterator = elem_type const*;
	const_iterator begin() const                { return data(); }
	const_iterator end() const                  { return begin() + size(); }
	const_iterator cbegin() const               { return begin(); }
	const_iterator cend() const                 { return end(); }

	auto* data()                                { return m_value.data(); }
	using iterator = elem_type const*;
	iterator begin()                            { return data(); }
	iterator end()                              { return begin() + size(); }

	void clear()                                { m_value.clear(); }

	template <class... ARGS>
	void copy(base_t const& from, ARGS&&...)
	{
		clear();
		m_value.assign(from.begin(), from.end());
	}

	template <class T = VALUE> decltype(std::declval<T>().resize(0))
	resize(std::size_t new_size)                { return m_value.resize(new_size); }

	template <class T = VALUE> decltype(std::declval<T>().emplace())
	emplace()                                   { return m_value.emplace(); }
	template <class T = VALUE> decltype(std::declval<T>().emplace(0))
	emplace(std::size_t num_bytes)              { return m_value.emplace(num_bytes); }

	bool set(std::size_t len, void const* data) { return set_encoded(len, data); }
	bool set()                                  { return set_encoded(0, this); }
	template <class ARR>
	auto set(ARR const& s) -> std::enable_if_t<
		std::is_integral_v<decltype(s.size())> && std::is_pointer_v<decltype(s.data())>, bool>
	{
		return this->set(s.size(), s.data());
	}
	template <typename T, std::size_t N>
	bool set(T const(&arr)[N])                  { return set(N * sizeof(T), arr); }

	//NOTE: do not override!
	bool set_encoded(std::size_t len, void const* data)
	{
		if constexpr (traits::min_octets != 0)
		{
			if (len < traits::min_octets)
			{
				CODEC_TRACE("ERROR: len=%zu < min=%zu", len, traits::min_octets);
				//m_value.clear();
				return false;
			}
		}
		if constexpr (traits::max_octets != inf())
		{
			if (len > traits::max_octets)
			{
				CODEC_TRACE("ERROR: len=%zu > max=%zu", len, traits::max_octets);
				//m_value.clear();
				return false;
			}
		}
		auto it = static_cast<iterator>(data);
		m_value.assign(it, it + len);
		return is_set();
	}

	value_type const& get() const               { return m_value; }
	bool is_set() const                         { return m_value.is_set(); }
	explicit operator bool() const              { return is_set(); }

	bool operator==(octet_string_impl const& rhs) const noexcept
	{
		return size() == rhs.size() && 0 == std::memcmp(data(), rhs.data(), size());
	}

protected:
	value_type  m_value;
};


template <class VALUE = octets_var_extern, class = void, class = void>
struct octet_string;

//ASCII-printable string not zero-terminated
template <class ...T>
struct ascii_string : octet_string<T...>
{
	using base_t = octet_string<T...>;
	using base_t::set;

	std::string_view get() const            { return std::string_view{(char const*)this->data(), this->size()}; }
	bool set(char const* psz)               { return this->set(std::string_view{psz}); }
	template <std::size_t N>
	bool set(char const(&arr)[N])           { return this->set(std::string_view{arr, strnlen(arr, N)}); }

	template <std::size_t N>
	void print(char (&sz)[N]) const
	{
		auto const& s = this->get();
		std::snprintf(sz, sizeof(sz), "%.*s", int(s.size()), s.data());
	}
};

constexpr std::size_t MAX_OCTS = std::numeric_limits<num_octs_t>::max();

template <class VALUE>
struct octet_string<VALUE, void, void>
		: octet_string_impl<detail::octet_traits<uint8_t, 0, MAX_OCTS>, VALUE> {};
template <class VALUE, class EXT_TRAITS>
struct octet_string<VALUE, EXT_TRAITS, void>
		: octet_string_impl<detail::octet_traits<uint8_t, 0, MAX_OCTS, EXT_TRAITS>, VALUE> {};

template <std::size_t MIN>
struct octet_string<min<MIN>, void, void>
		: octet_string_impl<detail::octet_traits<uint8_t, MIN, MAX_OCTS>, octets_var_extern> {};
template <class VALUE, std::size_t MIN>
struct octet_string<VALUE, min<MIN>, void>
		: octet_string_impl<detail::octet_traits<uint8_t, MIN, MAX_OCTS>, VALUE> {};
template <std::size_t MIN, class VALUE>
struct octet_string<min<MIN>, VALUE, void>
		: octet_string_impl<detail::octet_traits<uint8_t, MIN, MAX_OCTS>, VALUE> {};

template <std::size_t MAX>
struct octet_string<max<MAX>, void, void>
		: octet_string_impl<detail::octet_traits<uint8_t, 0, MAX>, octets_var_extern> {};
template <class VALUE, std::size_t MAX>
struct octet_string<VALUE, max<MAX>, void>
		: octet_string_impl<detail::octet_traits<uint8_t, 0, MAX>, VALUE> {};
template <std::size_t MAX, class VALUE>
struct octet_string<max<MAX>, VALUE, void>
		: octet_string_impl<detail::octet_traits<uint8_t, 0, MAX>, VALUE> {};

template <std::size_t MIN, std::size_t MAX>
struct octet_string<min<MIN>, max<MAX>, void>
		: octet_string_impl<detail::octet_traits<uint8_t, MIN, MAX>, octets_var_extern> {};
template <class VALUE, std::size_t MIN, std::size_t MAX>
struct octet_string<VALUE, min<MIN>, max<MAX>>
		: octet_string_impl<detail::octet_traits<uint8_t, MIN, MAX>, VALUE> {};
template <std::size_t MIN, std::size_t MAX, class VALUE>
struct octet_string<min<MIN>, max<MAX>, VALUE>
		: octet_string_impl<detail::octet_traits<uint8_t, MIN, MAX>, VALUE> {};

template <std::size_t FIXED>
struct octet_string<octets_fix_extern<FIXED>, void, void>
		: octet_string_impl<detail::octet_traits<uint8_t, FIXED>, octets_fix_extern<FIXED>>
{
	using base_t = octet_string_impl<detail::octet_traits<uint8_t, FIXED>, octets_fix_extern<FIXED>>;
	using base_t::set;

	void set(uint8_t const(&data)[FIXED])   { this->set_encoded(FIXED, &data[0]); }
};

template <std::size_t FIXED>
struct octet_string<octets_fix_intern<FIXED>, void, void>
		: octet_string_impl<detail::octet_traits<uint8_t, FIXED>, octets_fix_intern<FIXED>>
{
	using base_t = octet_string_impl<detail::octet_traits<uint8_t, FIXED>, octets_fix_intern<FIXED>>;
	using base_t::set;

	void set(uint8_t const(&data)[FIXED])   { this->set_encoded(FIXED, &data[0]); }
};
template <std::size_t MAX, std::size_t MIN>
struct octet_string<octets_var_intern<MAX>, min<MIN>, void>
		: octet_string_impl<detail::octet_traits<uint8_t, MIN, MAX>, octets_var_intern<MAX>> {};

}	//end: namespace med
