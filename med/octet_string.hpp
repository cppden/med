/*!
@file
TODO: define.

@copyright Denis Priyomov 2016
Distributed under the MIT License
(See accompanying file LICENSE or copy at https://opensource.org/licenses/MIT)
*/

#pragma once

#include <cstring>

#include "ie.hpp"

namespace med {

//varying string
template <std::size_t MIN, std::size_t MAX, typename Enable = void>
struct octets
{
	static void copy(uint8_t* out, uint8_t const* in, std::size_t size)
	{
		std::memcpy(out, in, size);
	}
};

//fixed string
template <std::size_t MIN, std::size_t MAX>
struct octets<MIN, MAX, std::enable_if_t<MIN == MAX>>
{
	static void copy(uint8_t* out, uint8_t const* in, std::size_t)
	{
		copy_impl(out, in, std::make_index_sequence<MIN>{});
	}

private:
	template <typename T>
	static constexpr void copy_octet(T* out, T const* in) { }

	template <typename T, std::size_t INDEX, std::size_t... Is>
	static void copy_octet(T* out, T const* in)
	{
		out[INDEX] = in[INDEX];
		copy_octet<T, Is...>(out, in);
	}

	template <typename T, std::size_t... Is>
	static void copy_impl(T* out, T const* in, std::index_sequence<Is...>)
	{
		copy_octet<T, Is...>(out, in);
	}
};

//variable length octets with external storage
class octets_var_extern
{
public:
	bool empty() const                                  { return 0 == m_size; }

	std::size_t size() const                            { return m_size; }
	uint8_t const* data() const                         { return m_data; }

	void assign(uint8_t const* b_, uint8_t const* e_)   { m_data = b_; m_size = e_ - b_;}

private:
	uint8_t const* m_data;
	std::size_t    m_size {0};
};

//variable length octets with internal storage
template <std::size_t MAX_LEN>
class octets_var_intern
{
public:
	bool empty() const                                  { return 0 == m_size; }

	std::size_t size() const                            { return m_size; }
	void resize(std::size_t v)                          { m_size = (v <= MAX_LEN) ? v : 0; }
	uint8_t const* data() const                         { return m_data; }
	uint8_t* data()                                     { return m_data; }

	//NOTE: no check for MAX_LEN since it's limited by caller in octet_string::set
	void assign(uint8_t const* beg_, uint8_t const* end_)
	{
		m_size = static_cast<std::size_t>(end_ - beg_);
		octets<0, MAX_LEN>::copy(m_data, beg_, m_size);
	}

private:
	uint8_t     m_data[MAX_LEN];
	std::size_t m_size {0};
};

//fixed length octets with external storage
template <std::size_t LEN>
class octets_fix_extern
{
public:
	bool empty() const                                  { return nullptr == data(); }

	constexpr std::size_t size() const                  { return LEN; }
	uint8_t const* data() const                         { return m_data; }

	void assign(uint8_t const* beg_, uint8_t const* end_)
	{
		std::size_t const nsize = static_cast<std::size_t>(end_ - beg_);
		if (nsize == LEN)
		{
			m_data = beg_;
		}
		else
		{
			CODEC_TRACE("ERROR: len=%zu != fix=%zu", nsize, LEN);
			m_data = nullptr;
		}
	}

private:
	uint8_t const* m_data {nullptr};
};

//fixed length octets with internal storage
template <std::size_t LEN>
class octets_fix_intern
{
public:
	bool empty() const                                  { return !m_is_set; }

	constexpr std::size_t size() const                  { return LEN; }
	uint8_t const* data() const                         { return m_data; }
	uint8_t* data()                                     { return m_data; }

	void assign(uint8_t const* beg_, uint8_t const* end_)
	{
		std::size_t const nsize = static_cast<std::size_t>(end_ - beg_);
		if (nsize == LEN)
		{
			octets<LEN, LEN>::copy(m_data, beg_, 0);
			m_is_set = true;
		}
		else
		{
			CODEC_TRACE("ERROR: len=%zu != fix=%zu", nsize, LEN);
			m_is_set = false;
		}
	}

private:
	uint8_t     m_data[LEN];
	bool        m_is_set {false};
};


template <std::size_t MIN_LEN, std::size_t MAX_LEN, class VALUE = octets_var_extern>
struct octet_string_impl : IE<IE_OCTET_STRING>
{
	using value_type = VALUE;
	using base_t = octet_string_impl;

	enum limits_e : std::size_t
	{
		min_octets = MIN_LEN,
		max_octets = MAX_LEN,
	};

	constexpr std::size_t size() const      { return m_value.size(); }
	constexpr std::size_t get_length() const{ return size(); }

	uint8_t const* data() const             { return m_value.data(); }
	using const_iterator = uint8_t const*;
	const_iterator begin() const            { return data(); }
	const_iterator end() const              { return begin() + size(); }
	const_iterator cbegin() const           { return begin(); }
	const_iterator cend() const             { return end(); }

	uint8_t* data()                         { return m_value.data(); }
	using iterator = uint8_t const*;
	iterator begin()                        { return data(); }
	iterator end()                          { return begin() + size(); }

	bool set(std::size_t len, void const* data)
	{
		if (len >= MIN_LEN)
		{
			uint8_t const* it = static_cast<uint8_t const*>(data);
			m_value.assign(it, it + (len <= MAX_LEN ? len : MAX_LEN));
			return !m_value.empty();
		}
		CODEC_TRACE("ERROR: len=%zu < min=%zu", len, MIN_LEN);
		return false;
	}

	template <typename T, std::size_t SIZE>
	bool set(T const(&data)[SIZE])
	{
		static_assert(sizeof(T) == 1, "INVALID TYPE TO SET OCTET_STRING");
		static_assert(SIZE >= MIN_LEN && SIZE <= MAX_LEN, "INVALID SIZE TO SET OCTET_STRING");
		return set(SIZE, data);
	}

	bool set(char const* psz)               { return set(std::strlen(psz), psz); }

	value_type const& get() const           { return m_value; }
	bool is_set() const                     { return !m_value.empty(); }
	explicit operator bool() const          { return is_set(); }

protected:
	value_type  m_value;
};


template <class VALUE = octets_var_extern, class = void, class = void> struct octet_string;

template <class VALUE>
struct octet_string<VALUE, void, void> : octet_string_impl<0, std::numeric_limits<std::size_t>::max(), VALUE> {};

template <std::size_t MIN>
struct octet_string<min<MIN>, void, void>  : octet_string_impl<MIN, std::numeric_limits<std::size_t>::max(), octets_var_extern> {};
template <class VALUE, std::size_t MIN>
struct octet_string<VALUE, min<MIN>, void> : octet_string_impl<MIN, std::numeric_limits<std::size_t>::max(), VALUE> {};
template <std::size_t MIN, class VALUE>
struct octet_string<min<MIN>, VALUE, void> : octet_string_impl<MIN, std::numeric_limits<std::size_t>::max(), VALUE> {};

template <std::size_t MAX>
struct octet_string<max<MAX>, void, void>  : octet_string_impl<0, MAX, octets_var_extern> {};
template <class VALUE, std::size_t MAX>
struct octet_string<VALUE, max<MAX>, void> : octet_string_impl<0, MAX, VALUE> {};
template <std::size_t MAX, class VALUE>
struct octet_string<max<MAX>, VALUE, void> : octet_string_impl<0, MAX, VALUE> {};

template <std::size_t MIN, std::size_t MAX>
struct octet_string<min<MIN>, max<MAX>, void>  : octet_string_impl<MIN, MAX, octets_var_extern> {};
template <class VALUE, std::size_t MIN, std::size_t MAX>
struct octet_string<VALUE, min<MIN>, max<MAX>> : octet_string_impl<MIN, MAX, VALUE> {};
template <std::size_t MIN, std::size_t MAX, class VALUE>
struct octet_string<min<MIN>, max<MAX>, VALUE> : octet_string_impl<MIN, MAX, VALUE> {};

template <std::size_t FIXED>
struct octet_string<octets_fix_extern<FIXED>, void, void> : octet_string_impl<FIXED, FIXED, octets_fix_extern<FIXED>> {};
template <std::size_t FIXED>
struct octet_string<octets_fix_intern<FIXED>, void, void> : octet_string_impl<FIXED, FIXED, octets_fix_intern<FIXED>> {};

template <std::size_t MAX, std::size_t MIN>
struct octet_string<octets_var_intern<MAX>, min<MIN>,void> : octet_string_impl<MIN, MAX, octets_var_intern<MAX>> {};

}	//end: namespace med