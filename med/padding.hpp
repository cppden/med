/**
@file
pseudo-IE for padding/alignments within containers

@copyright Denis Priyomov 2016
Distributed under the MIT License
(See accompanying file LICENSE or copy at https://opensource.org/licenses/MIT)
*/

#pragma once

#include "ie.hpp"

namespace med {

template <std::size_t BITS, uint8_t FILLER = 0>
struct padding
{
	using bits   = std::integral_constant<std::size_t, BITS>;
	using filler = std::integral_constant<uint8_t, FILLER>;
};

template <class, class Enable = void>
struct has_padding : std::false_type { };

template <class T>
struct has_padding<T, void_t<typename T::padding>> : std::true_type { };

template <class IE, class FUNC>
struct padder
{
	explicit padder(FUNC& func) noexcept
		: m_encoder{ func }
		, m_start{ m_encoder.get_state() }
	{}

	padder(padder&& rhs) noexcept
		: m_encoder{ rhs.m_encoder}
		, m_start{ rhs.m_start }
	{
	}

	explicit operator bool() const
	{
		auto const total_bits = (m_encoder.get_state() - m_start) * FUNC::granularity;
		auto const pad_bits = (IE::padding::bits::value - (total_bits % IE::padding::bits::value)) % IE::padding::bits::value;
		//CODEC_TRACE("PADDING %zu bits for %zd = %zu\n", pad_bits, total_bits, pad_bits + total_bits);
		return pad_bits ? m_encoder.padding(pad_bits, IE::padding::filler::value) : true;
	}

	padder& operator= (padder&&) = delete;
	padder(padder const&) = delete;
	padder(padder&) = delete;
	padder& operator= (padder const&) = delete;
	padder& operator= (padder&) = delete;

	FUNC&                     m_encoder;
	typename FUNC::state_type m_start;
};


template <class IE, class FUNC>
std::enable_if_t<has_padding<IE>::value, padder<IE, FUNC>>
inline add_padding(FUNC& func)
{
	return padder<IE, FUNC>{func};
}

template <class IE, class FUNC>
std::enable_if_t<!has_padding<IE>::value, bool>
constexpr add_padding(FUNC&)
{
	return true;
}


}	//end: namespace med
