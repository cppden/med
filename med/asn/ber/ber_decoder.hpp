#pragma once
/**
@file
ASN.1 BER decoder definition

@copyright Denis Priyomov 2018
Distributed under the MIT License
(See accompanying file LICENSE or visit https://github.com/cppden/med)
*/

#include "debug.hpp"
#include "name.hpp"
#include "state.hpp"
#include "count.hpp"
#include "octet_string.hpp"
#include "ber_tag.hpp"
#include "ber_info.hpp"
#include "../asn.hpp"

namespace med::asn::ber {

//http://graphics.stanford.edu/~seander/bithacks.html#FixedSignExtend
template <typename T, uint8_t B>
constexpr auto signextend(T x) -> std::enable_if_t<std::is_signed_v<T>, T>
{
	struct {T x:B;} s {};
	return s.x = x;
}
template <typename T, uint8_t B>
constexpr auto signextend(T x) -> std::enable_if_t<std::is_unsigned_v<T>, T>
{
	return x;
}

template <class DEC_CTX>
struct decoder : info
{
	//required for length_encoder
	using state_type = typename DEC_CTX::buffer_type::state_type;
	using size_state = typename DEC_CTX::buffer_type::size_state;
	using allocator_type = typename DEC_CTX::allocator_type;

	explicit decoder(DEC_CTX& ctx_) : m_ctx{ ctx_ } { }
	DEC_CTX& get_context() noexcept             { return m_ctx; }
	allocator_type& get_allocator()             { return get_context().get_allocator(); }

	//state
	auto operator() (PUSH_SIZE ps)              { return get_context().buffer().push_size(ps.size, ps.commit); }
	template <class IE>
	bool operator() (PUSH_STATE, IE const&)     { return get_context().buffer().push_state(); }
	void operator() (POP_STATE)                 { get_context().buffer().pop_state(); }
	auto operator() (GET_STATE)                 { return get_context().buffer().get_state(); }
	template <class IE>
	bool operator() (CHECK_STATE, IE const&)    { return !get_context().buffer().empty(); }

	//IE_TAG
	template <class IE> [[nodiscard]] std::size_t operator() (IE&, IE_TAG)
	{
		//constexpr auto nbytes = IE::traits::num_bytes(); TODO: expose asn traits directly
		constexpr std::size_t nbytes = bits_to_bytes(IE::traits::bits);
		uint8_t const* input = get_context().buffer().template advance<IE, nbytes>();
		std::size_t vtag = 0;
		get_bytes_impl(input, vtag, std::make_index_sequence<nbytes>{});
		CODEC_TRACE("T=%zX [%s] %zu bytes: %s", vtag, name<IE>(), nbytes, get_context().buffer().toString());
		return vtag;
	}
	//IE_LEN
	template <class IE> void operator() (IE& ie, IE_LEN)
	{
		//CODEC_TRACE("LEN[%s]: %s", name<IE>(), get_context().buffer().toString());
		auto const len = ber_length<IE>();
		ie.set_encoded(len);
		CODEC_TRACE("L=%zX [%s]: %s", len, name<IE>(), get_context().buffer().toString());
	}

	//IE_NULL
	template <class IE> constexpr void operator() (IE&, IE_NULL) const
	{
	}

	//IE_VALUE
	template <class IE> void operator() (IE& ie, IE_VALUE)
	{
		if constexpr (is_seqof_v<IE>)
		{
			CODEC_TRACE("SEQOF[%s] *[%zu..%zu]", name<IE>(), IE::min, IE::max);
			while (this->operator()(CHECK_STATE{}, ie))
			{
				auto* field = ie.push_back(*this);
				decode(*this, *field);
			}
			check_arity(*this, ie);
		}
		else if constexpr (is_oid_v<IE>)
		{
			CODEC_TRACE("OID[%s] *[%zu..%zu]", name<IE>(), IE::min, IE::max);
			while (this->operator()(CHECK_STATE{}, ie))
			{
				auto* field = ie.push_back(*this);
				this->operator()(*field, IE_VALUE{});
			}
			check_arity(*this, ie);
		}
		else
		{
			CODEC_TRACE("V[%s]: %s", name<IE>(), get_context().buffer().toString());
			if constexpr (std::is_same_v<bool, typename IE::value_type>)
			{
				//X.690 8.2 Encoding of a boolean value
				return ie.set_encoded(get_context().buffer().template pop<IE>() != 0);
			}
			else if constexpr (std::is_integral_v<typename IE::value_type>)
			{
				//X.690 8.3 Encoding of an integer value
				//X.690 8.4 Encoding of an enumerated value
				if (auto const len = get_context().buffer().size(); 0 < len && len < 127) //1..127 in one octet
				{
					CODEC_TRACE("\t%zu octets: %s", len, get_context().buffer().toString());
					auto* input = get_context().buffer().template advance<IE>(len); //value
					ie.set_encoded(get_bytes<typename IE::value_type>(input, len));
				}
				else
				{
					MED_THROW_EXCEPTION(invalid_value, name<IE>(), len, get_context().buffer())
				}
			}
			else if constexpr (std::is_floating_point_v<typename IE::value_type>)
			{
				MED_THROW_EXCEPTION(unknown_tag, name<IE>(), 0, get_context().buffer())
			}
			else
			{
				static_assert(std::is_void_v<typename IE::value_type>, "NOT IMPLEMENTED?");
			}
		}
	}

	//IE_BIT_STRING
	template <class IE> void operator() (IE& ie, IE_BIT_STRING)
	{
		auto const unused_bits = get_context().buffer().template pop<IE>(); //num of unused bits [0..7]
		auto const len = get_context().buffer().size();
		std::size_t const num_bits = len * 8 - unused_bits;
		CODEC_TRACE("\tBSTR[%s] %zu bits: %s", name<IE>(), num_bits, get_context().buffer().toString());
		if (ie.set_encoded(num_bits, get_context().buffer().begin()))
		{
			get_context().buffer().template advance<IE>(len);
		}
		else
		{
			MED_THROW_EXCEPTION(invalid_value, name<IE>(), num_bits, get_context().buffer())
		}
	}

	//IE_OCTET_STRING
	template <class IE> void operator() (IE& ie, IE_OCTET_STRING)
	{
		auto const len = get_context().buffer().size();
		CODEC_TRACE("\tOSTR[%s] %zu octets: %s", name<IE>(), len, get_context().buffer().toString());
		if (ie.set_encoded(len, get_context().buffer().begin()))
		{
			get_context().buffer().template advance<IE>(len);
		}
		else
		{
			MED_THROW_EXCEPTION(invalid_value, name<IE>(), len, get_context().buffer())
		}
	}

#ifndef UNIT_TEST
private:
#endif

	template <class IE>
	std::size_t ber_length()
	{
		uint8_t bytes = get_context().buffer().template pop<IE>();
		//short form
		if (bytes < 0x80) { return bytes; }

		bytes &= 0x7F;
		if (bytes)
		{
			return get_bytes<std::size_t>(get_context().buffer().template advance<IE>(bytes), bytes);
		}
		else //indefinite form (X.690 8.1.3.6)
		//8.1.3.6 length octets indicate that the contents octets are terminated by end-of-contents octets
		//(two zero octets), and shall consist of a single octet.
		{
			MED_THROW_EXCEPTION(invalid_value, name<IE>(), 0x80, get_context().buffer())
		}
	}

	template <typename VALUE>
	static constexpr void get_byte(uint8_t const*, VALUE&) { }

	template <typename VALUE, std::size_t OFS, std::size_t... Is>
	static void get_byte(uint8_t const* input, VALUE& value)
	{
		value = (value << 8) | *input;
		get_byte<VALUE, Is...>(++input, value);
	}

	template<typename VALUE, std::size_t... Is>
	static void get_bytes_impl(uint8_t const* input, VALUE& value, std::index_sequence<Is...>)
	{
		get_byte<VALUE, Is...>(input, value);
	}

	template <class IE>
	static auto get_bytes(uint8_t const* input)
	{
		constexpr std::size_t NUM_BYTES = bits_to_bytes(IE::traits::bits);
		typename IE::value_type value{};
		get_bytes_impl(input, value, std::make_index_sequence<NUM_BYTES>{});
		return value;
	}

	template <typename T>
	static T get_bytes(uint8_t const* input, uint8_t num_bytes)
	{
		//TODO: template to unwrap w/o duck-typing and allow char/short/int/long...
		switch (num_bytes)
		{
		case 1: return signextend<T, 8>(input[0]);
		case 2: if constexpr (sizeof(T) >= 2) return signextend<T, 16>((T(input[0]) <<  8) | (T(input[1])) );
		case 3: if constexpr (sizeof(T) >= 3) return signextend<T, 24>((T(input[0]) << 16) | (T(input[1]) <<  8) | (T(input[2])) );
		case 4: if constexpr (sizeof(T) >= 4) return signextend<T, 32>((T(input[0]) << 24) | (T(input[1]) << 16) | (T(input[2]) << 8) | (T(input[3])) );
		case 5: if constexpr (sizeof(T) >= 5) return signextend<T, 40>((T(input[0]) << 32) | (T(input[1]) << 24) | (T(input[2]) << 16) | (T(input[3])<< 8) | (T(input[4])) );
		case 6: if constexpr (sizeof(T) >= 6) return signextend<T, 48>((T(input[0]) << 40) | (T(input[1]) << 32) | (T(input[2]) << 24) | (T(input[3])<<16) | (T(input[4])<< 8) | (T(input[5])) );
		case 7: if constexpr (sizeof(T) >= 7) return signextend<T, 56>((T(input[0]) << 48) | (T(input[1]) << 40) | (T(input[2]) << 32) | (T(input[3])<<24) | (T(input[4])<<16) | (T(input[5])<< 8) | (T(input[6])) );
		case 8: if constexpr (sizeof(T) >= 8) return signextend<T, 64>((T(input[0]) << 56) | (T(input[1]) << 48) | (T(input[2]) << 40) | (T(input[3])<<32) | (T(input[4])<<24) | (T(input[5])<<16) | (T(input[6])<<8) | (T(input[7])));
		default: MED_THROW_EXCEPTION(invalid_value, __FUNCTION__, num_bytes)
		}
	}

	DEC_CTX& m_ctx;
};

} //end: namespace med::asn::ber
