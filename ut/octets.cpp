#include "ut.hpp"

#include "encode.hpp"
#include "decode.hpp"
#include "encoder_context.hpp"
#include "decoder_context.hpp"
#include "octet_encoder.hpp"
#include "octet_decoder.hpp"


struct var_intern : med::octet_string<med::octets_var_intern<8>, med::min<0>> {};
struct var_extern : med::octet_string<med::octets_var_extern, med::min<0>, med::max<8>> {};
struct fix_intern : med::octet_string<med::octets_fix_intern<8>> {};
struct fix_extern : med::octet_string<med::octets_fix_extern<8>> {};
struct OCTS : med::sequence<
	O< T<1>, L, var_intern >,
	O< T<2>, L, var_extern >,
	O< T<3>, L, fix_intern >,
	O< T<4>, L, fix_extern >
>
{};

TEST(octets, var_intern)
{
	uint8_t buffer[16];
	med::encoder_context<> ctx{ buffer };

	uint8_t const in[] = {1,2,3,4,5};
	OCTS msg;

	ASSERT_TRUE(msg.ref<var_intern>().set(sizeof(in), in));
#if (MED_EXCEPTIONS)
	encode(med::octet_encoder{ctx}, msg);
#else
	if (!encode(med::octet_encoder{ctx}, msg)) { FAIL() << toString(ctx.error_ctx()); }
#endif
	uint8_t const encoded[] = {1,5,1,2,3,4,5};
	ASSERT_EQ(sizeof(encoded), ctx.buffer().get_offset());
	ASSERT_TRUE(Matches(encoded, buffer));

	med::decoder_context<> dctx{ctx.buffer().get_start(), ctx.buffer().get_offset()};
	OCTS dmsg;
#if (MED_EXCEPTIONS)
	decode(med::octet_decoder{dctx}, dmsg);
#else
	if (!decode(med::octet_decoder{dctx}, dmsg)) { FAIL() << toString(ctx.error_ctx()); }
#endif
	var_intern const* p = msg.cfield();
	ASSERT_NE(nullptr, p);
	ASSERT_EQ(sizeof(in), p->size());
	ASSERT_TRUE(Matches(in, p->data()));
}

TEST(octets, var_intern_zero)
{
	uint8_t buffer[16];
	med::encoder_context<> ctx{ buffer };

	OCTS msg;

	ASSERT_TRUE(msg.ref<var_intern>().set());
#if (MED_EXCEPTIONS)
	encode(med::octet_encoder{ctx}, msg);
#else
	if (!encode(med::octet_encoder{ctx}, msg)) { FAIL() << toString(ctx.error_ctx()); }
#endif
	uint8_t const encoded[] = {1,0};
	ASSERT_EQ(sizeof(encoded), ctx.buffer().get_offset());
	ASSERT_TRUE(Matches(encoded, buffer));

	med::decoder_context<> dctx{ctx.buffer().get_start(), ctx.buffer().get_offset()};
	OCTS dmsg;
#if (MED_EXCEPTIONS)
	decode(med::octet_decoder{dctx}, dmsg);
#else
	if (!decode(med::octet_decoder{dctx}, dmsg)) { FAIL() << toString(ctx.error_ctx()); }
#endif
	var_intern const* p = msg.cfield();
	ASSERT_NE(nullptr, p);
	ASSERT_EQ(0, p->size());
}

TEST(octets, var_extern)
{
	uint8_t buffer[16];
	med::encoder_context<> ctx{ buffer };

	uint8_t const in[] = {1,2,3,4,5};
	OCTS msg;

	ASSERT_TRUE(msg.ref<var_extern>().set(sizeof(in), in));
#if (MED_EXCEPTIONS)
	encode(med::octet_encoder{ctx}, msg);
#else
	if (!encode(med::octet_encoder{ctx}, msg)) { FAIL() << toString(ctx.error_ctx()); }
#endif
	uint8_t const encoded[] = {2,5,1,2,3,4,5};
	ASSERT_EQ(sizeof(encoded), ctx.buffer().get_offset());
	ASSERT_TRUE(Matches(encoded, buffer));

	med::decoder_context<> dctx{ctx.buffer().get_start(), ctx.buffer().get_offset()};
	OCTS dmsg;
#if (MED_EXCEPTIONS)
	decode(med::octet_decoder{dctx}, dmsg);
#else
	if (!decode(med::octet_decoder{dctx}, dmsg)) { FAIL() << toString(ctx.error_ctx()); }
#endif
	var_extern const* p = msg.cfield();
	ASSERT_NE(nullptr, p);
	ASSERT_EQ(sizeof(in), p->size());
	ASSERT_TRUE(Matches(in, p->data()));
}

TEST(octets, var_extern_zero)
{
	uint8_t buffer[16];
	med::encoder_context<> ctx{ buffer };

	OCTS msg;

	ASSERT_TRUE(msg.ref<var_extern>().set());
#if (MED_EXCEPTIONS)
	encode(med::octet_encoder{ctx}, msg);
#else
	if (!encode(med::octet_encoder{ctx}, msg)) { FAIL() << toString(ctx.error_ctx()); }
#endif
	uint8_t const encoded[] = {2,0};
	ASSERT_EQ(sizeof(encoded), ctx.buffer().get_offset());
	ASSERT_TRUE(Matches(encoded, buffer));

	med::decoder_context<> dctx{ctx.buffer().get_start(), ctx.buffer().get_offset()};
	OCTS dmsg;
#if (MED_EXCEPTIONS)
	decode(med::octet_decoder{dctx}, dmsg);
#else
	if (!decode(med::octet_decoder{dctx}, dmsg)) { FAIL() << toString(ctx.error_ctx()); }
#endif
	var_extern const* p = msg.cfield();
	ASSERT_NE(nullptr, p);
	ASSERT_EQ(0, p->size());
}


TEST(octets, fix_intern)
{
	uint8_t buffer[16];
	med::encoder_context<> ctx{ buffer };

	uint8_t const in[] = {1,2,3,4,5,6,7,8};
	OCTS msg;

	ASSERT_TRUE(msg.ref<fix_intern>().set(sizeof(in), in));
#if (MED_EXCEPTIONS)
	encode(med::octet_encoder{ctx}, msg);
#else
	if (!encode(med::octet_encoder{ctx}, msg)) { FAIL() << toString(ctx.error_ctx()); }
#endif
	uint8_t const encoded[] = {3,8,1,2,3,4,5,6,7,8};
	ASSERT_EQ(sizeof(encoded), ctx.buffer().get_offset());
	ASSERT_TRUE(Matches(encoded, buffer));

	med::decoder_context<> dctx{ctx.buffer().get_start(), ctx.buffer().get_offset()};
	OCTS dmsg;
#if (MED_EXCEPTIONS)
	decode(med::octet_decoder{dctx}, dmsg);
#else
	if (!decode(med::octet_decoder{dctx}, dmsg)) { FAIL() << toString(ctx.error_ctx()); }
#endif
	fix_intern const* p = msg.cfield();
	ASSERT_NE(nullptr, p);
	ASSERT_EQ(sizeof(in), p->size());
	ASSERT_TRUE(Matches(in, p->data()));
}

TEST(octets, fix_extern)
{
	uint8_t buffer[16];
	med::encoder_context<> ctx{ buffer };

	uint8_t const in[] = {1,2,3,4,5,6,7,8};
	OCTS msg;

	ASSERT_TRUE(msg.ref<fix_extern>().set(sizeof(in), in));
#if (MED_EXCEPTIONS)
	encode(med::octet_encoder{ctx}, msg);
#else
	if (!encode(med::octet_encoder{ctx}, msg)) { FAIL() << toString(ctx.error_ctx()); }
#endif
	uint8_t const encoded[] = {4,8,1,2,3,4,5,6,7,8};
	ASSERT_EQ(sizeof(encoded), ctx.buffer().get_offset());
	ASSERT_TRUE(Matches(encoded, buffer));

	med::decoder_context<> dctx{ctx.buffer().get_start(), ctx.buffer().get_offset()};
	OCTS dmsg;
#if (MED_EXCEPTIONS)
	decode(med::octet_decoder{dctx}, dmsg);
#else
	if (!decode(med::octet_decoder{dctx}, dmsg)) { FAIL() << toString(ctx.error_ctx()); }
#endif
	fix_extern const* p = msg.cfield();
	ASSERT_NE(nullptr, p);
	ASSERT_EQ(sizeof(in), p->size());
	ASSERT_TRUE(Matches(in, p->data()));
}

