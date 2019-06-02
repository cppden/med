/**
@file
bit string IE definition

@copyright Denis Priyomov 2019
Distributed under the MIT License
(See accompanying file LICENSE or visit https://github.com/cppden/med)
*/

#pragma once

#include "octet_string.hpp"

namespace med {

//variable length bits
class bits_variable
{
public:
	bool is_set() const         { return m_least_bits > 0; }

	//total number of bytes = ceil(number of bits)
	std::size_t size() const    { return m_num_bytes; }
	nbits num_of_bits() const   { return nbits{8*size() - (8 - m_least_bits)}; }
	nbits least_bits() const    { return nbits{m_least_bits}; }

	uint8_t const* data() const { return inplace() && is_set() ? m_data.internal : m_data.external; }

	std::size_t uint() const
	{
		if (not inplace()) { throw invalid_value("too many bits", std::size_t(num_of_bits())); }
		std::size_t v = m_data.internal[0];
		for (std::size_t n = 1; n < size(); ++n) { v = (v << 8) | m_data.internal[n]; }
		return v >> (8 - m_least_bits);
	}
	void uint(nbits num_bits, std::size_t v)
	{
		set_nums<sizeof(m_data.internal)>(num_bits);
		v <<= 8 - m_least_bits;
		for (std::size_t n = 0; n < size(); ++n)
			{ m_data.internal[n] = uint8_t(v >> 8*(size() - n - 1)); }
	}

	void clear()                { m_num_bytes = 0; m_least_bits = 0; m_data.external = nullptr; }

	void assign_bits(void const* b, nbits num_bits)
	{
		set_nums<std::numeric_limits<decltype(m_num_bytes)>::max()>(num_bits);
		if (inplace()) { std::memcpy(&m_data.internal, b, m_num_bytes); }
		else           { m_data.external = static_cast<uint8_t const*>(b); }
	}

private:
	bool inplace() const        { return m_num_bytes <= sizeof(m_data.internal); }

	template <std::size_t MAX>
	void set_nums(nbits num_bits)
	{
		auto const num_bytes = bits_to_bytes(std::size_t(num_bits));
		if (num_bytes > MAX) { throw invalid_value("number of bits", std::size_t(num_bits)); }

		m_num_bytes = static_cast<decltype(m_num_bytes)>(num_bytes);
		m_least_bits = med::least_bits(std::size_t(num_bits));
	}

	uint16_t    m_num_bytes {0};
	uint8_t     m_least_bits {0}; //0 - not set, or 1..8

	union
	{
		uint8_t const*  external {nullptr};
		uint8_t         internal[sizeof(uint64_t)];
	}           m_data;
};

//fixed length bits
template <std::size_t NBITS, class Enable = void>
class bits_fixed;

template <std::size_t NBITS>
class bits_fixed<NBITS, std::enable_if_t<(NBITS > 8*sizeof(uint64_t))>>
{
public:
	bool is_set() const                     { return nullptr != data(); }

	static constexpr std::size_t size()     { return bits_to_bytes(NBITS); }
	static constexpr nbits num_of_bits()    { return nbits{NBITS}; }
	static constexpr nbits least_bits()     { return nbits{med::least_bits(NBITS)}; }
	uint8_t const* data() const             { return m_data; }

	void clear()                            { m_data = nullptr; }
	void assign_bits(void const* b, nbits)  { m_data = static_cast<uint8_t const*>(b); }

private:
	uint8_t const* m_data {nullptr};
};

template <std::size_t NBITS>
class bits_fixed<NBITS, std::enable_if_t<(NBITS <= 8*sizeof(uint64_t))>>
{
public:
	bool is_set() const                     { return m_set; }

	static constexpr std::size_t size()     { return bits_to_bytes(NBITS); }
	static constexpr nbits num_of_bits()    { return nbits{NBITS}; }
	static constexpr nbits least_bits()     { return nbits{med::least_bits(NBITS)}; }
	uint8_t const* data() const             { return is_set() ? m_data : nullptr; }

	void clear()                            { m_set = false; }

	std::size_t uint() const
	{
		std::size_t v = m_data[0];
		for (std::size_t n = 1; n < size(); ++n) { v = (v << 8) | m_data[n]; }
		return v >> (8 - uint8_t(least_bits()));
	}
	void uint(std::size_t v)
	{
		v <<= 8 - uint8_t(least_bits());
		for (std::size_t n = 0; n < size(); ++n)
			{ m_data[n] = uint8_t(v >> 8*(size() - n - 1)); }
		m_set = true;
	}

	void assign_bits(void const* b, nbits)
	{
		std::memcpy(&m_data, b, size());
		m_set = true;
	}

private:
	bool    m_set {false};
	uint8_t m_data[size()];
};

template <class TRAITS>
struct bit_string_impl : IE<IE_BIT_STRING>
{
	using traits     = TRAITS;
	using value_type = std::conditional_t<traits::min_bits == traits::max_bits, bits_fixed<traits::min_bits>, bits_variable>;
	using base_t = bit_string_impl;

	constexpr std::size_t size() const      { return m_value.size(); }
	constexpr std::size_t get_length() const{ return size(); }

	auto* data() const                      { return m_value.data(); }
	void clear()                            { m_value.clear(); }

	template <class... ARGS>
	void copy(base_t const& from, ARGS&&...)        { m_value = from.m_value; }

	bool set(std::size_t nbits, void const* data)   { return set_encoded(nbits, data); }
	//bool set(std::size_t nbits, std::size_t val)    { return set_encoded(0, nullptr); }

	//NOTE: do not override!
	bool set_encoded(std::size_t nb, void const* data)
	{
		if (nb >= traits::min_bits && nb <= traits::max_bits)
		{
			m_value.assign_bits(data, nbits{nb});
			return is_set();
		}
		CODEC_TRACE("ERROR: bits=%zu !=[%zu..%zu]", nb, traits::min_bits, traits::max_bits);
		return false;
	}

	value_type const& get() const           { return m_value; }
	bool is_set() const                     { return m_value.is_set(); }
	explicit operator bool() const          { return is_set(); }

protected:
	value_type  m_value;
};


template <class TRAITS = empty_traits, class = void, class = void>
struct bit_string;

constexpr std::size_t MAX_BITS = 8*std::numeric_limits<num_octs_t>::max();

template <class EXT_TRAITS>
struct bit_string<EXT_TRAITS, void, void>
		: bit_string_impl<detail::bits_traits<0, MAX_BITS, EXT_TRAITS>> {};

template <std::size_t MIN>
struct bit_string<min<MIN>, void, void>
		: bit_string_impl<detail::bits_traits<MIN, MAX_BITS>> {};

template <std::size_t MAX>
struct bit_string<max<MAX>, void, void>
		: bit_string_impl<detail::bits_traits<0, MAX>> {};

template <std::size_t MIN, std::size_t MAX>
struct bit_string<min<MIN>, max<MAX>, void>
		: bit_string_impl<detail::bits_traits<MIN, MAX>> {};

}	//end: namespace med
