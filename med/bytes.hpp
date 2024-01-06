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

} //end: namespace med
