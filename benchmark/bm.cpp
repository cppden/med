#include <benchmark/benchmark.h>

#include "med.hpp"
#include "encode.hpp"
#include "decode.hpp"
#include "encoder_context.hpp"
#include "decoder_context.hpp"
#include "octet_encoder.hpp"
#include "octet_decoder.hpp"

namespace {

template <typename ...T>
using M = med::mandatory<T...>;
template <typename ...T>
using O = med::optional<T...>;

using L = med::length_t<med::value<uint8_t>>;
using CNT = med::counter_t<med::value<uint16_t>>;
template <std::size_t TAG>
using T = med::value<med::fixed<TAG, uint8_t>>;
template <std::size_t TAG>
using C = med::value<med::fixed<TAG>>;
template <uint8_t BITS>
using HDR = med::value<med::bits<BITS>>;

struct FLD_UC : med::value<uint8_t>
{
	static constexpr char const* name() { return "UC"; }
};
struct FLD_U16 : med::value<uint16_t>
{
	static constexpr char const* name() { return "U16"; }
};
struct FLD_U24 : med::value<med::bytes<3>>
{
	static constexpr char const* name() { return "U24"; }
};

struct FLD_IP : med::value<uint32_t>
{
	static constexpr char const* name() { return "IP-Address"; }
	template <std::size_t N>
	void print(char (&sz)[N]) const
	{
		uint32_t ip = get();
		std::snprintf(sz, sizeof(sz), "%u.%u.%u.%u", uint8_t(ip >> 24), uint8_t(ip >> 16), uint8_t(ip >> 8), uint8_t(ip));
	}
};

struct FLD_DW : med::value<uint32_t>
{
	static constexpr char const* name() { return "Double-Word"; }
};

struct VFLD1 : med::ascii_string<med::min<5>, med::max<10>>, med::with_snapshot
{
	static constexpr char const* name() { return "url"; }
};

struct custom_length : med::value<uint8_t>
{
	static bool value_to_length(std::size_t& v)
	{
		if (v < 5)
		{
			v = 2*(v + 1);
			return true;
		}
		return false;
	}

	static bool length_to_value(std::size_t& v)
	{
		if (0 == (v & 1)) //should be even
		{
			v = (v - 2) / 2;
			return true;
		}
		return false;
	}

	static constexpr char const* name() { return "Custom-Length"; }
};
using CLEN = med::length_t<custom_length>;

struct MSG_SEQ : med::sequence<
	M< FLD_UC >,              //<V>
	M< T<0x21>, FLD_U16 >,    //<TV>
	M< L, FLD_U24 >,          //<LV>(fixed)
	M< T<0x42>, L, FLD_IP >, //<TLV>(fixed)
	O< T<0x51>, FLD_DW >,     //[TV]
	O< T<0x12>, CLEN, VFLD1 > //TLV(var)
>
{
	static constexpr char const* name() { return "Msg-Seq"; }
};


struct PROTO : med::choice< HDR<8>
	, med::tag<C<0x01>, MSG_SEQ>
>
{
};


void BM_encode_ok(benchmark::State& state)
{
	PROTO proto;
	uint8_t buffer[1024];
	med::encoder_context<> ctx{ buffer };

	MSG_SEQ& msg = proto.select();
	msg.ref<FLD_UC>().set(37);
	msg.ref<FLD_U16>().set(0x35D9);
	msg.ref<FLD_U24>().set(0xDABEEF);
	msg.ref<FLD_IP>().set(0xFee1ABBA);


	msg.ref<FLD_DW>().set(0x01020304);
	msg.ref<VFLD1>().set("test.this!");

	while (state.KeepRunning())
	{
		ctx.reset();
#ifdef MED_NO_EXCEPTION
		if (!encode(med::make_octet_encoder(ctx), proto))
		{
			std::abort();
		}
#else
		encode(med::make_octet_encoder(ctx), proto);
#endif
	}
}
BENCHMARK(BM_encode_ok);

void BM_encode_fail(benchmark::State& state)
{
	PROTO proto;
	uint8_t buffer[1024];
	med::encoder_context<> ctx{ buffer };

	MSG_SEQ& msg = proto.select();
	msg.ref<FLD_UC>().set(0);
	msg.ref<FLD_U24>().set(0);

	while (state.KeepRunning())
	{
		ctx.reset();
#ifdef MED_NO_EXCEPTION
		if (encode(med::make_octet_encoder(ctx), proto)
		|| med::error::MISSING_IE != ctx.error_ctx().get_error())
		{
			std::abort();
		}
#else
		try
		{
			encode(med::make_octet_encoder(ctx), proto);
			std::abort();
		}
		catch (med::exception const& ex)
		{
			if (med::error::MISSING_IE != ex.error())
			{
				std::abort();
			}
		}
#endif
	}
}
BENCHMARK(BM_encode_fail);

void BM_decode_ok(benchmark::State& state)
{
	PROTO proto;
	med::decoder_context<> ctx;

	uint8_t const encoded[] = { 1
		, 37
		, 0x21, 0x35, 0xD9
		, 3, 0xDA, 0xBE, 0xEF
		, 0x42, 4, 0xFE, 0xE1, 0xAB, 0xBA
		, 0x51, 0x01, 0x02, 0x03, 0x04
		, 0x12, 4, 't', 'e', 's', 't', '.', 't', 'h', 'i', 's', '!'
	};

	while (state.KeepRunning())
	{
		ctx.reset(encoded, sizeof(encoded));
#ifdef MED_NO_EXCEPTION
		if (!decode(med::make_octet_decoder(ctx), proto))
		{
			std::abort();
		}
#else
		decode(med::make_octet_decoder(ctx), proto);
#endif
	}
}
BENCHMARK(BM_decode_ok);

void BM_decode_fail(benchmark::State& state)
{
	PROTO proto;
	med::decoder_context<> ctx;

	//invalid length of variable field (longer)
	uint8_t const bad_var_len_hi[] = { 1
		, 37
		, 0x21, 0x35, 0xD9
		, 3, 0xDA, 0xBE, 0xEF
		, 0x42, 4, 0xFE, 0xE1, 0xAB, 0xBA
		, 0x12, 5, 't', 'e', 's', 't', 'e', 's', 't', 'e', 's', 't', 'e'
	};

	while (state.KeepRunning())
	{
		ctx.reset(bad_var_len_hi, sizeof(bad_var_len_hi));
#ifdef MED_NO_EXCEPTION
		if (decode(med::make_octet_decoder(ctx), proto)
		|| med::error::INCORRECT_VALUE != ctx.error_ctx().get_error())
		{
			std::abort();
		}
#else
		try
		{
			decode(med::make_octet_decoder(ctx), proto);
			std::abort();
		}
		catch (med::exception const& ex)
		{
			if (med::error::INCORRECT_VALUE != ex.error())
			{
				std::abort();
			}
		}
#endif
	}
}
BENCHMARK(BM_decode_fail);

} //end: namespace

BENCHMARK_MAIN()
