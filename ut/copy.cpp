#include "ut.hpp"

#include "encode.hpp"
#include "decode.hpp"
#include "encoder_context.hpp"
#include "decoder_context.hpp"
#include "octet_encoder.hpp"
#include "octet_decoder.hpp"

namespace cp {

struct var_intern : med::octet_string<med::octets_var_intern<8>, med::min<0>> {};
struct var_extern : med::octet_string<med::octets_var_extern, med::min<0>, med::max<8>> {};
struct fix_intern : med::octet_string<med::octets_fix_intern<8>> {};
struct fix_extern : med::octet_string<med::octets_fix_extern<8>> {};

struct octs : med::sequence<
	O< T<1>, L, var_intern >,
	O< T<2>, L, var_extern >,
	O< T<3>, L, fix_intern >,
	O< T<4>, L, fix_extern >
>{};

struct byte : med::value<uint8_t> {};
struct word : med::value<uint16_t> {};
struct dword : med::value<uint32_t> {};

struct cho : med::choice< byte
	, med::tag<C<1>, byte>
	, med::tag<C<2>, word>
	, med::tag<C<4>, dword>
>{};

struct seq : med::sequence<
	M< T<0xF1>, byte, med::min<2>, med::inf >,    //<TV>*[2,*)
	M< T<0xF2>, L, word, med::inf >,              //<TLV>(fixed)*[1,*)
	M< med::counter_t<byte>, dword, med::inf > //C<V>*[1,*)
>{};

struct seq_r : med::sequence<
	M< T<0xE1>, dword, med::min<2>, med::inf >,
	M< T<0xE2>, L, byte, med::min<2>, med::inf >,
	M< med::counter_t<byte>, word, med::min<2>, med::inf >
>{};

} //end: namespace cp

TEST(copy, seq_same)
{
	uint8_t const encoded[] = {
		0xF1, 0x13, //TV
		0xF1, 0x37, //TV
		0xF2, 2, 0xFE, 0xE1, //TLV
		0xF2, 2, 0xAB, 0xBA, //TLV
		1, 0xDE, 0xAD, 0xBE, 0xEF, //CV
	};

	cp::seq src_msg;
	cp::seq dst_msg;

	uint8_t dec_buf[128];
	{
		med::decoder_context<> ctx{ encoded, dec_buf };
#if (MED_EXCEPTIONS)
		decode(make_octet_decoder(ctx), src_msg);
		//fails when need allocator but not provided one
		ASSERT_THROW(dst_msg.copy(src_msg), med::exception);
		//succeed with allocator
		dst_msg.copy(src_msg, ctx);
#else
		if (!decode(make_octet_decoder(ctx), src_msg)) { FAIL() << toString(ctx.error_ctx()); }
		ASSERT_FALSE(dst_msg.copy(src_msg));
		ASSERT_TRUE(dst_msg.copy(src_msg, ctx));
#endif
	}

	{
		uint8_t buffer[128];
		med::encoder_context<> ctx{ buffer };

#if (MED_EXCEPTIONS)
		encode(make_octet_encoder(ctx), dst_msg);
#else
		if (!encode(make_octet_encoder(ctx), dst_msg)) { FAIL() << toString(ctx.error_ctx()); }
#endif
		EXPECT_EQ(sizeof(encoded), ctx.buffer().get_offset());
		EXPECT_TRUE(Matches(encoded, buffer));
	}
}

#if 1
TEST(copy, seq_diff)
{
	cp::seq src_msg;
	cp::seq_r dst_msg;
	uint8_t dec_buf[256];

	{
		uint8_t const encoded[] = {
			0xF1, 0x13, //TV(byte)
			0xF1, 0x37, //TV(byte)
			0xF2, 2, 0xFE, 0xE1, //TLV(word)
			0xF2, 2, 0xAB, 0xBA, //TLV(word)
			2, 0xDE, 0xAD, 0xBE, 0xEF, //CV(dword)
			   0xC0, 0x01, 0xCA, 0xFE, //CV(dword)
		};

		med::decoder_context<> ctx{ encoded, dec_buf };
#if (MED_EXCEPTIONS)
		decode(make_octet_decoder(ctx), src_msg);
		dst_msg.copy(src_msg, ctx);
#else
		if (!decode(make_octet_decoder(ctx), src_msg)) { FAIL() << toString(ctx.error_ctx()); }
		ASSERT_TRUE(dst_msg.copy(src_msg, ctx));
#endif
	}

	{
//		M< T<0xE1>, dword, med::min<2>, med::inf >,
//		M< T<0xE2>, L, byte, med::min<2>, med::inf >,
//		M< med::counter_t<byte>, word, med::min<2>, med::inf >
		uint8_t const encoded[] = {
			0xE1, 0xDE, 0xAD, 0xBE, 0xEF,
			0xE1, 0xC0, 0x01, 0xCA, 0xFE,
			0xE2, 1, 0x13,
			0xE2, 1, 0x37,
			2, 0xFE, 0xE1,
			   0xAB, 0xBA,
		};

		uint8_t buffer[128];
		med::encoder_context<> ctx{ buffer };

#if (MED_EXCEPTIONS)
		encode(make_octet_encoder(ctx), dst_msg);
#else
		if (!encode(make_octet_encoder(ctx), dst_msg)) { FAIL() << toString(ctx.error_ctx()); }
#endif
		EXPECT_EQ(sizeof(encoded), ctx.buffer().get_offset());
		EXPECT_TRUE(Matches(encoded, buffer));
	}
}
#endif

TEST(copy, choice)
{
	cp::cho src;
	src.ref<cp::word>().set(0xABBA);

	cp::cho dst;
	dst.ref<cp::byte>().set(1);
	dst.copy(src);

	cp::word const* pw = dst.cselect();
	ASSERT_NE(nullptr, pw);
	EXPECT_EQ(0xABBA, pw->get());
}

//TODO: test copy_to, copy of not set fields?
