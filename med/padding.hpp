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
		: m_func{ func }
		, m_start{ m_func(GET_STATE{}) }
	{}

	padder(padder&& rhs) noexcept
		: m_func{ rhs.m_func}
		, m_start{ rhs.m_start }
	{
	}

	void enable(bool v) const           { m_enable = v; }
	//current padding size in bits
	std::size_t size() const
	{
		if (m_enable)
		{
			auto const total_bits = (m_func(GET_STATE{}) - m_start) * FUNC::granularity;
			//CODEC_TRACE("PADDING %zu bits for %zd = %zu\n", pad_bits, total_bits, pad_bits + total_bits);
			return (IE::padding::bits::value - (total_bits % IE::padding::bits::value)) % IE::padding::bits::value;
		}
		return 0;
	}

	explicit operator bool() const
	{
		if (auto const pad_bits = size())
		{
			CODEC_TRACE("PADDING %zu bits", pad_bits);
			return m_func(ADD_PADDING{pad_bits, IE::padding::filler::value});
		}
		return true;
	}

	padder(padder const&) = delete;
	padder& operator= (padder const&) = delete;

	FUNC&                     m_func;
	typename FUNC::state_type m_start;
	bool mutable              m_enable{ true };
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

template <class T>
inline void padding_enable(T const& pad, bool v)            { pad.enable(v); }
constexpr void padding_enable(bool const pad, bool v)       { }

template <class T>
inline std::size_t padding_size(T const& pad)               { return pad.size(); }
constexpr std::size_t padding_size(bool const pad)          { return 0; }

}	//end: namespace med
