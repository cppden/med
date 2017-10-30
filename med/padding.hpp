/**
@file
pseudo-IE for padding/alignments within containers

@copyright Denis Priyomov 2016-2017
Distributed under the MIT License
(See accompanying file LICENSE or visit https://github.com/cppden/med)
*/

#pragma once


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

	MED_RESULT add() const
	{
		if (auto const pad_bits = size())
		{
			CODEC_TRACE("PADDING %zu bits", pad_bits);
			return m_func(ADD_PADDING{pad_bits, IE::padding::filler::value});
		}
		MED_RETURN_SUCCESS;
	}

	padder(padder const&) = delete;
	padder& operator= (padder const&) = delete;

	FUNC&                     m_func;
	typename FUNC::state_type m_start;
	bool mutable              m_enable{ true };
};

using dummy_padder = std::true_type;

template <class IE, class FUNC>
std::enable_if_t<has_padding<IE>::value, padder<IE, FUNC>>
inline add_padding(FUNC& func)
{
	return padder<IE, FUNC>{func};
}

template <class IE, class FUNC>
std::enable_if_t<!has_padding<IE>::value, dummy_padder>
constexpr add_padding(FUNC&)
{
	return dummy_padder{};
}

template <class T>
inline void padding_enable(T const& pad, bool v)            { pad.enable(v); }
constexpr void padding_enable(dummy_padder, bool)           { }

template <class T>
inline std::size_t padding_size(T const& pad)               { return pad.size(); }
constexpr std::size_t padding_size(dummy_padder)            { return 0; }

template <class T>
inline MED_RESULT padding_do(T const& pad)                  { return pad.add(); }
#ifdef MED_NO_EXCEPTION
constexpr MED_RESULT padding_do(dummy_padder)               { return true; }
#else //!MED_NO_EXCEPTION
constexpr MED_RESULT padding_do(dummy_padder)               { }
#endif //MED_NO_EXCEPTIONS

}	//end: namespace med
