/**
@file
pseudo-IE for padding/alignments within containers

@copyright Denis Priyomov 2016-2017
Distributed under the MIT License
(See accompanying file LICENSE or visit https://github.com/cppden/med)
*/

#pragma once

#include "config.hpp"
#include "units.hpp"
#include "debug.hpp"

namespace med {

namespace detail {

template <std::size_t BITS, bool INCLUSIVE, uint8_t FILLER>
struct pad
{
	static constexpr bool inclusive = INCLUSIVE;
	static constexpr std::size_t bits = BITS;
	static constexpr uint8_t filler = FILLER;
};

} //end: namespace detail

template <class UNIT, bool INCLUSIVE = false, uint8_t FILLER = 0, class Enable = void>
struct padding;

template <uint8_t BITS, bool INCLUSIVE, uint8_t FILLER>
struct padding<bits<BITS>, INCLUSIVE, FILLER, void>
	: detail::pad<BITS, INCLUSIVE, FILLER> {};

template <uint8_t BYTES, bool INCLUSIVE, uint8_t FILLER>
struct padding<bytes<BYTES>, INCLUSIVE, FILLER, void>
	: detail::pad<BYTES*8, INCLUSIVE, FILLER> {};

template <typename INT, bool INCLUSIVE, uint8_t FILLER>
struct padding<INT, INCLUSIVE, FILLER, std::enable_if_t<std::is_integral<INT>::value>>
	: detail::pad<sizeof(INT)*8, INCLUSIVE, FILLER> {};

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
	//current padding size in bits
	std::size_t size() const
	{
		if (m_enable)
		{
			auto const total_bits = (m_func(GET_STATE{}) - m_start) * FUNC::granularity;
			//CODEC_TRACE("PADDING %zu bits for %zd = %zu\n", pad_bits, total_bits, pad_bits + total_bits);
			return (pad_traits::bits - (total_bits % pad_traits::bits)) % pad_traits::bits;
		}
		return 0;
	}

	MED_RESULT add() const
	{
		if (auto const pad_bits = size())
		{
			CODEC_TRACE("PADDING %zu bits", pad_bits);
			return m_func(ADD_PADDING{pad_bits, IE::padding::filler});
		}
		MED_RETURN_SUCCESS;
	}

private:
	padder(padder const&) = delete;
	padder& operator= (padder const&) = delete;

	FUNC&                     m_func;
	typename FUNC::state_type m_start;
	bool mutable              m_enable{ true };
};

}	//end: namespace med
