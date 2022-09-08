#pragma once
/**
@file
helpers to read and write bytes(octets)

@copyright Denis Priyomov 2022
Distributed under the MIT License
(See accompanying file LICENSE or visit https://github.com/cppden/med)
*/

#include <utility>

#include "value_traits.hpp"

namespace med {

template <class T>
concept AIeValue = requires(T t)
{
	typename T::value_type;
	typename T::traits;
	{ T::traits::bits + 0 } -> std::unsigned_integral;
};

#if 0
template <typename VALUE>
constexpr void get_byte(uint8_t const*, VALUE&) { }

template <typename VALUE, std::size_t OFS, std::size_t... Is>
constexpr void get_byte(uint8_t const* input, VALUE& value)
{
	value = (value << 8) | *input;
	get_byte<VALUE, Is...>(++input, value);
}

template <uint8_t NUM_BYTES, typename VALUE = std::size_t>
constexpr VALUE get_bytes(uint8_t const* input)
{
	return [input]<std::size_t... Is>(std::index_sequence<Is...>)
	{
		VALUE value{};
		get_byte<VALUE, Is...>(input, value);
		return value;
	}(std::make_index_sequence<NUM_BYTES>{});
}

template <std::size_t NUM_BYTES>
constexpr void put_byte(uint8_t*, std::size_t) { }

template <std::size_t NUM_BYTES, std::size_t OFS, std::size_t... Is>
constexpr void put_byte(uint8_t* output, std::size_t value)
{
	output[OFS] = uint8_t(value >> ((NUM_BYTES - OFS - 1) * 8));
	put_byte<NUM_BYTES, Is...>(output, value);
}

template <std::size_t NUM_BYTES>
constexpr void put_bytes(std::size_t value, uint8_t* output)
{
	[]<std::size_t... Is>(std::size_t val, uint8_t* out, std::index_sequence<Is...>)
	{
		put_byte<NUM_BYTES, Is...>(out, val);
	}(value, output, std::make_index_sequence<NUM_BYTES>{});
}

#else
template <uint8_t NUM_BYTES>
constexpr void get_byte(uint8_t const*, uint8_t*) { }

template <uint8_t NUM_BYTES, std::size_t OFS, std::size_t... Is>
constexpr void get_byte(uint8_t const* input, uint8_t* out)
{
	constexpr uint8_t SHIFT = NUM_BYTES - OFS - 1;
	out[SHIFT] = input[OFS];
	get_byte<NUM_BYTES, Is...>(input, out);
}

template <uint8_t NUM_BYTES, typename VALUE = std::size_t>
constexpr VALUE get_bytes(uint8_t const* input)
{
	return [input]<std::size_t... Is>(std::index_sequence<Is...>)
	{
		union {
			VALUE   value{};
			uint8_t bytes[sizeof(value)];
		} out;
		get_byte<NUM_BYTES, Is...>(input, out.bytes);
		return out.value;
	}(std::make_index_sequence<NUM_BYTES>{});
}

template <std::size_t NUM_BYTES>
constexpr void put_byte(uint8_t*, uint8_t const*) { }

template <std::size_t NUM_BYTES, std::size_t OFS, std::size_t... Is>
constexpr void put_byte(uint8_t* output, uint8_t const* inp)
{
	output[OFS] = inp[NUM_BYTES - OFS - 1];
	put_byte<NUM_BYTES, Is...>(output, inp);
}

template <std::size_t NUM_BYTES>
constexpr void put_bytes(std::size_t value, uint8_t* output)
{
	[]<std::size_t... Is>(std::size_t val, uint8_t* out, std::index_sequence<Is...>)
	{
		union {
			std::size_t value;
			uint8_t bytes[sizeof(value)];
		} inp;
		inp.value = val;

		put_byte<NUM_BYTES, Is...>(out, inp.bytes);
	}(value, output, std::make_index_sequence<NUM_BYTES>{});
}

#endif

template <AIeValue IE>
constexpr auto get_bytes(uint8_t const* input)
{
	constexpr std::size_t NUM_BYTES = bits_to_bytes(IE::traits::bits);
	using VALUE = typename IE::value_type;
	return get_bytes<NUM_BYTES, VALUE>(input);
}

template <AIeValue IE>
constexpr void put_bytes(IE const& ie, uint8_t* output)
{
	put_bytes<bits_to_bytes(IE::traits::bits)>(ie.get_encoded(), output);
}


} //end: namespace med
