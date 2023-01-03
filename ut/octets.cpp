#include "ut.hpp"


struct var_intern : med::octet_string<med::octets_var_intern<8>, med::min<0>> {};
struct var_extern : med::octet_string<med::octets_var_extern, med::min<0>, med::max<8>> {};
struct var_intern_min : med::octet_string<med::octets_var_intern<8>, med::min<2>> {};
struct var_extern_min : med::octet_string<med::octets_var_extern, med::min<2>, med::max<8>> {};
struct fix_intern : med::octet_string<med::octets_fix_intern<8>> {};
struct fix_extern : med::octet_string<med::octets_fix_extern<8>> {};
struct OCTS : med::sequence<
	O< T<1>, L, var_intern >,
	O< T<2>, L, var_extern >,
	O< T<3>, L, fix_intern >,
	O< T<4>, L, fix_extern >,
	O< T<5>, L, var_intern_min >,
	O< T<6>, L, var_extern_min >
>
{};

TEST(octets, var_intern)
{
	uint8_t buffer[16];
	med::encoder_context<> ctx{ buffer };

	uint8_t const in[] = {1,2,3,4,5};
	OCTS msg;

	ASSERT_TRUE(msg.ref<var_intern>().set(sizeof(in), in));
	encode(med::octet_encoder{ctx}, msg);

	uint8_t const encoded[] = {1,5,1,2,3,4,5};
	ASSERT_EQ(sizeof(encoded), ctx.buffer().get_offset());
	ASSERT_TRUE(Matches(encoded, buffer));

	med::decoder_context<> dctx{ctx.buffer().get_start(), ctx.buffer().get_offset()};
	OCTS dmsg;
	decode(med::octet_decoder{dctx}, dmsg);

	auto* p = msg.get<var_intern>();
	ASSERT_NE(nullptr, p);
	ASSERT_EQ(sizeof(in), p->size());
	ASSERT_TRUE(Matches(in, p->data()));
}

TEST(octets, var_intern_overrun)
{
	uint8_t const encoded[] = {1,9, 1,2,3,4,5,6,7,8,9};
	OCTS msg;

	ASSERT_FALSE(msg.ref<var_intern>().set(sizeof(encoded), encoded));
	msg.clear();

	med::decoder_context<> ctx{encoded, sizeof(encoded)};
	EXPECT_THROW(decode(med::octet_decoder{ctx}, msg), med::invalid_value);
}

TEST(octets, var_intern_zero_len)
{
	uint8_t buffer[16];
	med::encoder_context<> ctx{ buffer };

	OCTS msg;

	ASSERT_TRUE(msg.ref<var_intern>().set());
	encode(med::octet_encoder{ctx}, msg);

	uint8_t const encoded[] = {1,0};
	ASSERT_EQ(sizeof(encoded), ctx.buffer().get_offset());
	ASSERT_TRUE(Matches(encoded, buffer));

	med::decoder_context<> dctx{ctx.buffer().get_start(), ctx.buffer().get_offset()};
	OCTS dmsg;
	decode(med::octet_decoder{dctx}, dmsg);

	auto* p = msg.get<var_intern>();
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
	encode(med::octet_encoder{ctx}, msg);

	uint8_t const encoded[] = {2,5,1,2,3,4,5};
	ASSERT_EQ(sizeof(encoded), ctx.buffer().get_offset());
	ASSERT_TRUE(Matches(encoded, buffer));

	med::decoder_context<> dctx{ctx.buffer().get_start(), ctx.buffer().get_offset()};
	OCTS dmsg;
	decode(med::octet_decoder{dctx}, dmsg);

	auto* p = msg.get<var_extern>();
	ASSERT_NE(nullptr, p);
	ASSERT_EQ(sizeof(in), p->size());
	ASSERT_TRUE(Matches(in, p->data()));
}

TEST(octets, var_extern_overrun)
{
	uint8_t const encoded[] = {2,9, 1,2,3,4,5,6,7,8,9};
	OCTS msg;

	ASSERT_FALSE(msg.ref<var_extern>().set(sizeof(encoded), encoded));
	msg.clear();

	med::decoder_context<> ctx{encoded, sizeof(encoded)};
	EXPECT_THROW(decode(med::octet_decoder{ctx}, msg), med::invalid_value);
}

TEST(octets, var_extern_zero_len)
{
	uint8_t buffer[16];
	med::encoder_context<> ctx{ buffer };

	OCTS msg;

	ASSERT_TRUE(msg.ref<var_extern>().set());
	encode(med::octet_encoder{ctx}, msg);

	uint8_t const encoded[] = {2,0};
	ASSERT_EQ(sizeof(encoded), ctx.buffer().get_offset());
	ASSERT_TRUE(Matches(encoded, buffer));

	med::decoder_context<> dctx{ctx.buffer().get_start(), ctx.buffer().get_offset()};
	OCTS dmsg;
	decode(med::octet_decoder{dctx}, dmsg);

	auto* p = msg.get<var_extern>();
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
	encode(med::octet_encoder{ctx}, msg);

	uint8_t const encoded[] = {3,8,1,2,3,4,5,6,7,8};
	ASSERT_EQ(sizeof(encoded), ctx.buffer().get_offset());
	ASSERT_TRUE(Matches(encoded, buffer));

	med::decoder_context<> dctx{ctx.buffer().get_start(), ctx.buffer().get_offset()};
	OCTS dmsg;
	decode(med::octet_decoder{dctx}, dmsg);

	auto* p = msg.get<fix_intern>();
	ASSERT_NE(nullptr, p);
	ASSERT_EQ(sizeof(in), p->size());
	ASSERT_TRUE(Matches(in, p->data()));
}

TEST(octets, fix_intern_limits)
{
	uint8_t const encoded[] = {3,9,1,2,3,4,5,6,7,8,9};
	med::decoder_context<> ctx{encoded, sizeof(encoded)};
	OCTS msg;
	ASSERT_FALSE(msg.ref<fix_intern>().set(7, encoded));
	ASSERT_FALSE(msg.ref<fix_intern>().set(9, encoded));
	EXPECT_THROW(decode(med::octet_decoder{ctx}, msg), med::invalid_value);

	{
		uint8_t const encoded[] = {3,1, 1};
		med::decoder_context<> ctx{encoded, sizeof(encoded)};
		EXPECT_THROW(decode(med::octet_decoder{ctx}, msg), med::invalid_value);
	}
}


TEST(octets, fix_extern)
{
	uint8_t buffer[16];
	med::encoder_context<> ctx{ buffer };

	uint8_t const in[] = {1,2,3,4,5,6,7,8};
	OCTS msg;

	ASSERT_TRUE(msg.ref<fix_extern>().set(sizeof(in), in));
	encode(med::octet_encoder{ctx}, msg);

	uint8_t const encoded[] = {4,8,1,2,3,4,5,6,7,8};
	ASSERT_EQ(sizeof(encoded), ctx.buffer().get_offset());
	ASSERT_TRUE(Matches(encoded, buffer));

	med::decoder_context<> dctx{ctx.buffer().get_start(), ctx.buffer().get_offset()};
	OCTS dmsg;
	decode(med::octet_decoder{dctx}, dmsg);

	auto* p = msg.get<fix_extern>();
	ASSERT_NE(nullptr, p);
	ASSERT_EQ(sizeof(in), p->size());
	ASSERT_TRUE(Matches(in, p->data()));
}

TEST(octets, fix_extern_limits)
{
	uint8_t const encoded[] = {4,9,1,2,3,4,5,6,7,8,9};
	med::decoder_context<> ctx{encoded, sizeof(encoded)};
	OCTS msg;
	ASSERT_FALSE(msg.ref<fix_extern>().set(7, encoded));
	ASSERT_FALSE(msg.ref<fix_extern>().set(9, encoded));
	EXPECT_THROW(decode(med::octet_decoder{ctx}, msg), med::invalid_value);

	{
		uint8_t const encoded[] = {4,1, 1};
		med::decoder_context<> ctx{encoded, sizeof(encoded)};
		EXPECT_THROW(decode(med::octet_decoder{ctx}, msg), med::invalid_value);
	}
}

TEST(octets, var_intern_min)
{
	uint8_t buffer[16];
	med::encoder_context<> ctx{ buffer };

	uint8_t const in[] = {1,2};
	OCTS msg;

	ASSERT_TRUE(msg.ref<var_intern_min>().set(sizeof(in), in));
	encode(med::octet_encoder{ctx}, msg);

	uint8_t const encoded[] = {5,2,1,2};
	ASSERT_EQ(sizeof(encoded), ctx.buffer().get_offset());
	ASSERT_TRUE(Matches(encoded, buffer));

	med::decoder_context<> dctx{ctx.buffer().get_start(), ctx.buffer().get_offset()};
	OCTS dmsg;
	decode(med::octet_decoder{dctx}, dmsg);

	auto* p = msg.get<var_intern_min>();
	ASSERT_NE(nullptr, p);
	ASSERT_EQ(sizeof(in), p->size());
	ASSERT_TRUE(Matches(in, p->data()));
}

TEST(octets, var_intern_min_limits)
{
	uint8_t const encoded[] = {5,9, 1,2,3,4,5,6,7,8,9};
	OCTS msg;

	ASSERT_FALSE(msg.ref<var_intern_min>().set(sizeof(encoded), encoded));
	ASSERT_FALSE(msg.ref<var_intern_min>().set(1, encoded));
	msg.clear();

	med::decoder_context<> ctx{encoded, sizeof(encoded)};
	EXPECT_THROW(decode(med::octet_decoder{ctx}, msg), med::invalid_value);

	{
		uint8_t const encoded[] = {5,1, 1};
		med::decoder_context<> ctx{encoded, sizeof(encoded)};
		EXPECT_THROW(decode(med::octet_decoder{ctx}, msg), med::invalid_value);
	}
}

TEST(octets, var_extern_min)
{
	uint8_t buffer[16];
	med::encoder_context<> ctx{ buffer };

	uint8_t const in[] = {1,2};
	OCTS msg;

	ASSERT_TRUE(msg.ref<var_extern_min>().set(sizeof(in), in));
	encode(med::octet_encoder{ctx}, msg);

	uint8_t const encoded[] = {6,2,1,2};
	ASSERT_EQ(sizeof(encoded), ctx.buffer().get_offset());
	ASSERT_TRUE(Matches(encoded, buffer));

	med::decoder_context<> dctx{ctx.buffer().get_start(), ctx.buffer().get_offset()};
	OCTS dmsg;
	decode(med::octet_decoder{dctx}, dmsg);

	auto* p = msg.get<var_extern_min>();
	ASSERT_NE(nullptr, p);
	ASSERT_EQ(sizeof(in), p->size());
	ASSERT_TRUE(Matches(in, p->data()));
}

TEST(octets, var_extern_min_limits)
{
	uint8_t const encoded[] = {6,9, 1,2,3,4,5,6,7,8,9};
	OCTS msg;

	ASSERT_FALSE(msg.ref<var_extern_min>().set(sizeof(encoded), encoded));
	ASSERT_FALSE(msg.ref<var_extern_min>().set(1, encoded));
	msg.clear();

	med::decoder_context<> ctx{encoded, sizeof(encoded)};
	EXPECT_THROW(decode(med::octet_decoder{ctx}, msg), med::invalid_value);
	{
		uint8_t const encoded[] = {6,1, 1};
		med::decoder_context<> ctx{encoded, sizeof(encoded)};
		EXPECT_THROW(decode(med::octet_decoder{ctx}, msg), med::invalid_value);
	}
}

TEST(octets, ascii_string)
{
	char arr[] = "str";
	med::ascii_string s;
	s.set(arr);
	EXPECT_EQ(s.get().size(), std::strlen(arr));
}
