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
struct pad_traits
{
	static constexpr std::size_t pad_bits = SIZE;
	static constexpr uint8_t filler = FILLER;
};

} //end: namespace detail

template <class T, uint8_t FILLER = 0, class Enable = void>
struct padding;

template <std::size_t BITS, uint8_t FILLER>
struct padding<bits<BITS>, FILLER, std::enable_if_t<(BITS > 0)>>
{
	using padding_traits = detail::pad_traits<BITS, FILLER>;
};

template <std::size_t BYTES, uint8_t FILLER>
struct padding<bytes<BYTES>, FILLER, std::enable_if_t<(BYTES > 0)>>
{
	using padding_traits = detail::pad_traits<BYTES*8, FILLER>;
};

template <class T, uint8_t FILLER>
struct padding<T, FILLER, std::enable_if_t<std::is_integral_v<T>>>
{
	using padding_traits = detail::pad_traits<sizeof(T)*8, FILLER>;
};

template <class, class Enable = void>
struct get_padding
{
	using type = void;
};

template <class T>
struct get_padding<T, std::enable_if_t<!std::is_void_v<typename T::padding_traits>>>
{
	using type = typename T::padding_traits;
};

template <class T>
struct get_padding<T, std::enable_if_t<!std::is_void_v<typename T::traits>>> : get_padding<typename T::traits> { };

template <class PAD_TRAITS, class FUNC>
struct octet_padder
{
	explicit octet_padder(FUNC& func) noexcept
		: m_func{ func }
		, m_start{ m_func(GET_STATE{}) }
	{}

	void enable(bool v) const           { m_enable = v; }

	//current padding size in units of codec
	std::size_t size() const
	{
		if (m_enable)
		{
			constexpr auto pad_bytes = PAD_TRAITS::pad_bits/8;
			auto const total_size = m_func(GET_STATE{}) - m_start;
			//CODEC_TRACE("ADDING %zu bytes for padding=%zu in %zd=%zu\n", (pad_bytes - (total_size % pad_bytes)) % pad_bytes, pad_bytes, total_size, pad_bytes + total_size);
			return (pad_bytes - (total_size % pad_bytes)) % pad_bytes;
		}
		return 0;
	}

	void add() const
	{
		if (auto const pad_bytes = size())
		{
			CODEC_TRACE("PADDING %zu bytes", pad_bytes);
			m_func(ADD_PADDING{pad_bytes, PAD_TRAITS::filler});
		}
	}

private:
	octet_padder(octet_padder const&) = delete;
	octet_padder& operator= (octet_padder const&) = delete;

	FUNC&                     m_func;
	typename FUNC::state_type m_start;
	bool mutable              m_enable{ true };
};

}	//end: namespace med
