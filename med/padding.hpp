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

enum class filler_value : uint8_t {};
template <uint8_t N> struct filler
{
	using type = std::integral_constant<filler_value, filler_value{N}>;
};
enum class offset_value : int8_t {};
template <int N> struct offset
{
	using type = std::integral_constant<offset_value, offset_value{N}>;
};
enum class delta_value : int8_t {};
template <int N> struct delta
{
	using type = std::integral_constant<delta_value, delta_value{N}>;
};

namespace detail {

template <class T>
using type_of = typename T::type;

struct def_value
{
	using type = void;
};

template <class, class...> struct value_of;

template <class R, R Val, class... Tail>
struct value_of<R, std::integral_constant<R, Val>, Tail...> : std::integral_constant<R, Val> {};

template <class R>
struct value_of<R, void> : std::integral_constant<R, R{}> {};

template <class R, class Head, class... Tail>
struct value_of<R, Head, Tail...> : value_of<R, Tail...> {};

template <std::size_t SIZE, class... DOF>
struct pad_traits
{
	static constexpr std::size_t pad_bits = SIZE;
	static constexpr uint8_t filler = uint8_t(value_of<filler_value, type_of<DOF>...>::value);
	static constexpr int offset = int(value_of<offset_value, type_of<DOF>...>::value);
	static constexpr int delta = int(value_of<delta_value, type_of<DOF>...>::value);
};

} //end: namespace detail


template <class T, class D = detail::def_value, class O = detail::def_value, class F = detail::def_value>
struct padding;

template <std::size_t BITS, class D, class O, class F> requires (BITS > 0)
struct padding<bits<BITS>, D, O, F>
{
	using padding_traits = detail::pad_traits<BITS, D, O, F>;
};

template <std::size_t BYTES, class D, class O, class F> requires (BYTES > 0)
struct padding<bytes<BYTES>, D, O, F>
{
	using padding_traits = detail::pad_traits<BYTES*8, D, O, F>;
};

template <std::integral T, class D, class O, class F>
struct padding<T, D, O, F>
{
	using padding_traits = detail::pad_traits<sizeof(T)*8, D, O, F>;
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

	void enable_padding(bool v) const           { m_enable = v; }

	static constexpr std::size_t calc_padding_size(std::size_t len)
	{
		constexpr auto pad_bytes = PAD_TRAITS::pad_bits/8;
		auto const padding_size = (pad_bytes - ((len + PAD_TRAITS::offset) % pad_bytes) + PAD_TRAITS::delta) % pad_bytes;
		CODEC_TRACE("%s: pad=%zu bytes for len=%zu+%d delta=%d => %zu", __FUNCTION__
					, pad_bytes, len, PAD_TRAITS::offset, PAD_TRAITS::delta, padding_size);
		return padding_size;
	}

	void add_padding() const
	{
		if (auto const pad_bytes = padding_size())
		{
			CODEC_TRACE("PADDING %u bytes", pad_bytes);
			m_func(ADD_PADDING{pad_bytes, PAD_TRAITS::filler});
		}
	}

	//current padding size in units of codec
	uint8_t padding_size() const        { return m_enable ? calc_padding_size(m_func(GET_STATE{}) - m_start) : 0; }

private:
	octet_padder(octet_padder const&) = delete;
	octet_padder& operator= (octet_padder const&) = delete;

	FUNC&                     m_func;
	typename FUNC::state_type m_start;
	bool mutable              m_enable{ true };
};

}	//end: namespace med
