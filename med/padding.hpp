/**
@file
pseudo-IE for padding/alignments within containers

@copyright Denis Priyomov 2016-2017
Distributed under the MIT License
(See accompanying file LICENSE or visit https://github.com/cppden/med)
*/

#pragma once

#include <type_traits>

#include "config.hpp"
#include "units.hpp"
#include "debug.hpp"
#include "state.hpp"

namespace med {

namespace detail {

template <std::size_t SIZE, uint8_t FILLER>
struct pad
{
	static constexpr std::size_t pad_size = SIZE;
	static constexpr uint8_t filler = FILLER;
};

} //end: namespace detail

template <class UNIT, uint8_t FILLER = 0, class Enable = void>
struct padding;

template <uint8_t BITS, uint8_t FILLER>
struct padding<bits<BITS>, FILLER, void>
	: detail::pad<BITS, FILLER> {};

template <uint8_t BYTES, uint8_t FILLER>
struct padding<bytes<BYTES>, FILLER, void>
	: detail::pad<BYTES, FILLER> {};

template <typename INT, uint8_t FILLER>
struct padding<INT, FILLER, std::enable_if_t<std::is_integral<INT>::value>>
	: detail::pad<sizeof(INT), FILLER> {};

template <class, class Enable = void>
struct has_padding : std::false_type { };

template <class T>
struct has_padding<T, std::void_t<typename T::padding>> : std::true_type { };

template <class IE, class FUNC>
struct padder
{
	using pad_traits = typename IE::padding;

	explicit padder(FUNC& func) noexcept
		: m_func{ func }
		, m_start{ m_func(GET_STATE{}) }
	{}

	void enable(bool v) const           { m_enable = v; }
	//current padding size in units of codec
	std::size_t size() const
	{
		if (m_enable)
		{
			auto const total_size = (m_func(GET_STATE{}) - m_start);
			//CODEC_TRACE("PADDING %zu bits for %zd = %zu\n", pad_bits, total_bits, pad_bits + total_bits);
			return (pad_traits::pad_size - (total_size % pad_traits::pad_size)) % pad_traits::pad_size;
		}
		return 0;
	}

	void add() const
	{
		if (auto const pad_size = size())
		{
			CODEC_TRACE("PADDING %zu", pad_size);
			m_func(ADD_PADDING{pad_size, IE::padding::filler});
		}
	}

private:
	padder(padder const&) = delete;
	padder& operator= (padder const&) = delete;

	FUNC&                     m_func;
	typename FUNC::state_type m_start;
	bool mutable              m_enable{ true };
};

}	//end: namespace med
