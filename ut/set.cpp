#include "ut.hpp"
#include "ut_proto.hpp"

#if 0 //TODO! FIXME
TEST(set, compound)
{
	using namespace std::string_view_literals;

	uint8_t buffer[128];
	med::encoder_context<> ctx{ buffer };

	cmp::SET msg;
	msg.ref<cmp::string>().set("12345678"sv);
	msg.ref<cmp::number>().set(0x12345678);

	encode(med::octet_encoder{ctx}, msg);

	EXPECT_STRCASEEQ(
		"00 0C 00 01 31 32 33 34 35 36 37 38 "
		"00 08 00 02 12 34 56 78 ",
		as_string(ctx.buffer())
	);

	decltype(msg) dmsg;
	med::decoder_context<> dctx;
	dctx.reset(ctx.buffer().get_start(), ctx.buffer().get_offset());
	decode(med::octet_decoder{dctx}, dmsg);

	EXPECT_EQ(msg.get<cmp::string>().get(), dmsg.get<cmp::string>().get());
	ASSERT_NE(nullptr, dmsg.get<cmp::number>());
	EXPECT_EQ(msg.get<cmp::number>()->get(), dmsg.get<cmp::number>()->get());
}
#endif

TEST(encode, set_ok)
{
	PROTO proto;

	MSG_SET& msg = proto.select();

	//mandatory fields
	msg.ref<FLD_UC>().set(0x11);
	msg.ref<FLD_U16>().set(0x35D9);

	uint8_t buffer[1024];
	med::encoder_context<> ctx{ buffer };

	encode(med::octet_encoder{ctx}, proto);

	uint8_t const encoded1[] = { 4
		, 0,0x0b, 0x11 //M<T16<0x0b>, UC>
		, 0,0x21, 2, 0x35, 0xD9 //M<T16<0x21>, L, U16>
	};
	EXPECT_EQ(sizeof(encoded1), ctx.buffer().get_offset());
	EXPECT_TRUE(Matches(encoded1, buffer));

	//optional fields
	ctx.reset();
	msg.ref<VFLD1>().set("test.this");

	encode(med::octet_encoder{ctx}, proto);

	uint8_t const encoded2[] = { 4
		, 0, 0x0b, 0x11
		, 0, 0x21, 2, 0x35, 0xD9
		, 0, 0x22, 9, 't', 'e', 's', 't', '.', 't', 'h', 'i', 's'
	};
	EXPECT_EQ(sizeof(encoded2), ctx.buffer().get_offset());
	EXPECT_TRUE(Matches(encoded2, buffer));

	ctx.reset();
	msg.ref<FLD_U24>().set(0xDABEEF);
	msg.ref<FLD_IP>().set(0xfee1ABBA);

	uint8_t const encoded3[] = { 4
		, 0, 0x0b, 0x11
		, 0, 0x21, 2, 0x35, 0xD9
		, 0, 0x49, 3, 0xDA, 0xBE, 0xEF
		, 0, 0x89, 0xFE, 0xE1, 0xAB, 0xBA
		, 0, 0x22, 9, 't', 'e', 's', 't', '.', 't', 'h', 'i', 's'
	};

	encode(med::octet_encoder{ctx}, proto);

	EXPECT_EQ(sizeof(encoded3), ctx.buffer().get_offset());
	EXPECT_TRUE(Matches(encoded3, buffer));
}

TEST(encode, set_fail_mandatory)
{
	PROTO proto;
	uint8_t buffer[1024];
	med::encoder_context<> ctx{ buffer };

	//missing mandatory fields in set
	MSG_SET& msg = proto.select();
	msg.ref<FLD_UC>().set(0);
	msg.ref<FLD_U24>().set(0);
	ctx.reset();
	EXPECT_THROW(encode(med::octet_encoder{ctx}, proto), med::missing_ie);
}

TEST(encode, mset_ok)
{
	uint8_t buffer[1024];
	med::encoder_context<> ctx{ buffer };

	PROTO proto;

	MSG_MSET& msg = proto.select();

	//mandatory fields
	msg.ref<FLD_UC> ().push_back(ctx)->set(0x11);
	msg.ref<FLD_UC> ().push_back(ctx)->set(0x12);
	msg.ref<FLD_U8> ().push_back(ctx)->set(0x13);
	msg.ref<FLD_U16>().push_back(ctx)->set(0x35D9);
	msg.ref<FLD_U16>().push_back(ctx)->set(0x35DA);

	encode(med::octet_encoder{ctx}, proto);

	uint8_t const encoded1[] = { 0x14
		, 0,0x0b, 0x11 //M<T16<0x0b>, FLD_UC, med::arity<2>>
		, 0,0x0b, 0x12 //M<T16<0x0b>, FLD_UC, med::arity<2>>
		, 0,0x0c, 0x13 //M<T16<0x0c>, FLD_U8, med::max<2>>
		, 0,0x21, 2, 0x35, 0xD9 //M<T16<0x21>, L, FLD_U16, med::max<3>>
		, 0,0x21, 2, 0x35, 0xDA //M<T16<0x21>, L, FLD_U16, med::max<3>>
	};
	EXPECT_EQ(sizeof(encoded1), ctx.buffer().get_offset());
	EXPECT_TRUE(Matches(encoded1, buffer));

	//optional fields
	ctx.reset();
	msg.ref<VFLD1>().push_back(ctx)->set("test.this");

	encode(med::octet_encoder{ctx}, proto);

	uint8_t const encoded2[] = { 0x14
		, 0, 0x0b, 0x11
		, 0, 0x0b, 0x12
		, 0, 0x0c, 0x13
		, 0, 0x21, 2, 0x35, 0xD9
		, 0, 0x21, 2, 0x35, 0xDA
		, 0, 0x22, 9, 't', 'e', 's', 't', '.', 't', 'h', 'i', 's'
	};
	EXPECT_EQ(sizeof(encoded2), ctx.buffer().get_offset());
	EXPECT_TRUE(Matches(encoded2, buffer));

	ctx.reset();
	msg.ref<FLD_U8> ().push_back(ctx)->set(0x14);
	msg.ref<FLD_U24>().push_back(ctx)->set(0xDABEEF);
	msg.ref<FLD_U24>().push_back(ctx)->set(0x22BEEF);
	msg.ref<FLD_IP> ().push_back(ctx)->set(0xfee1ABBA);
	msg.ref<FLD_IP> ().push_back(ctx)->set(0xABBAc001);
	msg.ref<VFLD1>  ().push_back(ctx)->set("test.it");

	uint8_t const encoded3[] = { 0x14
		, 0, 0x0b, 0x11
		, 0, 0x0b, 0x12
		, 0, 0x0c, 0x13
		, 0, 0x0c, 0x14
		, 0, 0x21, 2, 0x35, 0xD9
		, 0, 0x21, 2, 0x35, 0xDA
		, 0, 0x49, 3, 0xDA, 0xBE, 0xEF
		, 0, 0x49, 3, 0x22, 0xBE, 0xEF
		, 0, 0x89, 0xFE, 0xE1, 0xAB, 0xBA
		, 0, 0x89, 0xAB, 0xBA, 0xc0, 0x01
		, 0, 0x22, 9, 't', 'e', 's', 't', '.', 't', 'h', 'i', 's'
		, 0, 0x22, 7, 't', 'e', 's', 't', '.', 'i', 't'
	};

	encode(med::octet_encoder{ctx}, proto);

	EXPECT_EQ(sizeof(encoded3), ctx.buffer().get_offset());
	EXPECT_TRUE(Matches(encoded3, buffer));
}

TEST(encode, mset_fail_arity)
{
	PROTO proto;
	uint8_t buffer[1024];
	med::encoder_context<> ctx{ buffer };

	//arity violation in optional
	MSG_MSET& msg = proto.select();
	msg.ref<FLD_UC> ().push_back(ctx)->set(0);
	msg.ref<FLD_UC> ().push_back(ctx)->set(0);
	msg.ref<FLD_U8> ().push_back(ctx)->set(0);
	msg.ref<FLD_U16>().push_back(ctx)->set(0);
	encode(med::octet_encoder{ctx}, proto);

	msg.ref<FLD_U24>().push_back(ctx)->set(0);
	EXPECT_THROW(encode(med::octet_encoder{ctx}, proto), med::missing_ie);
}

TEST(decode, set_ok)
{
	PROTO proto;

	//mandatory fields
	uint8_t const encoded1[] = { 4
		, 0, 0x0b, 0x11
		, 0, 0x21, 2, 0x35, 0xD9
	};
	med::decoder_context<> ctx{ encoded1 };
	decode(med::octet_decoder{ctx}, proto);

	MSG_SET const* msg = proto.select();
	ASSERT_NE(nullptr, msg);

	ASSERT_EQ(0x11, msg->get<FLD_UC>().get());
	ASSERT_EQ(0x35D9, msg->get<FLD_U16>().get());
	FLD_U24 const* fld3 = msg->field();
	FLD_IP const* fld4 = msg->field();
	VFLD1 const* vfld1 = msg->field();
	ASSERT_EQ(nullptr, fld3);
	ASSERT_EQ(nullptr, fld4);
	ASSERT_EQ(nullptr, vfld1);

	//optional fields
	uint8_t const encoded2[] = { 4
		, 0, 0x0b, 0x11
		, 0, 0x21, 2, 0x35, 0xD9
		, 0, 0x22, 9, 't', 'e', 's', 't', '.', 't', 'h', 'i', 's'
	};
	ctx.reset(encoded2, sizeof(encoded2));
	decode(med::octet_decoder{ctx}, proto);

	msg = proto.select();
	ASSERT_NE(nullptr, msg);

	fld3 = msg->field();
	fld4 = msg->field();
	ASSERT_EQ(nullptr, fld3);
	ASSERT_EQ(nullptr, fld4);

	EQ_STRING_O(VFLD1, "test.this");

	//all fields out of order
	uint8_t const encoded3[] = { 4
		, 0, 0x22, 9, 't', 'e', 's', 't', '.', 't', 'h', 'i', 's'
		, 0, 0x89, 0xFE, 0xE1, 0xAB, 0xBA
		, 0, 0x49, 3, 0xDA, 0xBE, 0xEF
		, 0, 0x21, 2, 0x35, 0xD9
		, 0, 0x0b, 0x11
	};
	ctx.reset(encoded3, sizeof(encoded3));
	decode(med::octet_decoder{ctx}, proto);

	msg = proto.select();
	ASSERT_NE(nullptr, msg);

	fld3 = msg->field();
	fld4 = msg->field();
	ASSERT_NE(nullptr, fld3);
	ASSERT_NE(nullptr, fld4);

	ASSERT_EQ(0xDABEEF, fld3->get());
	ASSERT_EQ(0xfee1ABBA, fld4->get());
	EQ_STRING_O(VFLD1, "test.this");
}

TEST(decode, set_fail)
{
	PROTO proto;
	med::decoder_context<> ctx;

	//missing mandatory field
	uint8_t const missing_mandatory[] = { 4
		, 0, 0x22, 9, 't', 'e', 's', 't', '.', 't', 'h', 'i', 's'
		, 0, 0x49, 3, 0xDA, 0xBE, 0xEF
		, 0, 0x21, 2, 0x35, 0xD9
		, 0, 0x89, 0xFE, 0xE1, 0xAB, 0xBA
	};
	ctx.reset(missing_mandatory, sizeof(missing_mandatory));
	EXPECT_THROW(decode(med::octet_decoder{ctx}, proto), med::missing_ie);

	//extra field in set
	uint8_t const extra_field[] = { 4
		, 0, 0x49, 3, 0xDA, 0xBE, 0xEF
		, 0, 0x49, 3, 0xDA, 0xBE, 0xEF
	};
	ctx.reset(extra_field, sizeof(extra_field));
	EXPECT_THROW(decode(med::octet_decoder{ctx}, proto), med::extra_ie);
}

TEST(decode, mset_ok)
{
	PROTO proto;

	//mandatory fields
	uint8_t const encoded1[] = { 0x14
		, 0, 0x0b, 0x11
		, 0, 0x21, 2, 0x35, 0xD9
		, 0, 0x0b, 0x12
		, 0, 0x21, 2, 0x35, 0xDA
		, 0, 0x0c, 0x13
	};
	med::decoder_context<> ctx{ encoded1 };
	decode(med::octet_decoder{ctx}, proto);

	MSG_MSET const* msg = proto.select();
	ASSERT_NE(nullptr, msg);

	{
		ASSERT_EQ(2, msg->count<FLD_UC>());
		auto it = msg->get<FLD_UC>().begin();
		EXPECT_EQ(0x11, it->get());
		++it;
		EXPECT_EQ(0x12, it->get());
	}
	{
		ASSERT_EQ(1, msg->count<FLD_U8>());
		EXPECT_EQ(0x13, (msg->get<FLD_U8>().begin()->get()));
	}
	{
		ASSERT_EQ(2, msg->count<FLD_U16>());
		auto it = msg->get<FLD_U16>().begin();
		EXPECT_EQ(0x35D9, it->get());
		++it;
		EXPECT_EQ(0x35DA, it->get());
	}
	EXPECT_TRUE(msg->get<FLD_U24>().empty());
	EXPECT_TRUE(msg->get<FLD_IP>().empty());
	EXPECT_TRUE(msg->get<VFLD1>().empty());

	//optional fields
	uint8_t const encoded2[] = { 0x14
		, 0, 0x22, 9, 't', 'e', 's', 't', '.', 't', 'h', 'i', 's'
		, 0, 0x0b, 0x11
		, 0, 0x21, 2, 0x35, 0xD9
		, 0, 0x21, 2, 0x35, 0xDA
		, 0, 0x0c, 0x13
		, 0, 0x0b, 0x12
	};
	ctx.reset(encoded2, sizeof(encoded2));
	decode(med::octet_decoder{ctx}, proto);

	msg = proto.select();
	ASSERT_NE(nullptr, msg);

	EXPECT_TRUE(msg->get<FLD_U24>().empty());
	EXPECT_TRUE(msg->get<FLD_IP>().empty());

	EXPECT_EQ(1, msg->count<VFLD1>());
	EQ_STRING_O_(0, VFLD1, "test.this");

	//all fields out of order
	uint8_t const encoded3[] = { 0x14
		, 0, 0x22, 9, 't', 'e', 's', 't', '.', 't', 'h', 'i', 's'
		, 0, 0x22, 7, 't', 'e', 's', 't', '.', 'i', 't'
		, 0, 0x89, 0xFE, 0xE1, 0xAB, 0xBA
		, 0, 0x0b, 0x11
		, 0, 0x49, 3, 0xDA, 0xBE, 0xEF
		, 0, 0x49, 3, 0x22, 0xBE, 0xEF
		, 0, 0x21, 2, 0x35, 0xD9
		, 0, 0x21, 2, 0x35, 0xDA
		, 0, 0x89, 0xAB, 0xBA, 0xC0, 0x01
		, 0, 0x0b, 0x12
		, 0, 0x0c, 0x13
	};
	ctx.reset(encoded3, sizeof(encoded3));
	decode(med::octet_decoder{ctx}, proto);

	msg = proto.select();
	ASSERT_NE(nullptr, msg);

	{
		uint32_t const exp[] = {0xDABEEF, 0x22BEEF};
		auto const* p = exp;
		ASSERT_EQ(std::size(exp), msg->count<FLD_U24>());
		for (auto& f : msg->get<FLD_U24>())
		{
			EXPECT_EQ(*p++, f.get());
		}
	}
	{
		uint32_t const exp[] = {0xfee1ABBA, 0xABBAc001};
		auto const* p = exp;
		ASSERT_EQ(std::size(exp), msg->count<FLD_IP>());
		for (auto& f : msg->get<FLD_IP>())
		{
			EXPECT_EQ(*p++, f.get());
		}
	}
	EXPECT_EQ(2, msg->count<VFLD1>());
	EQ_STRING_O_(0, VFLD1, "test.this");
	EQ_STRING_O_(1, VFLD1, "test.it");
}

TEST(decode, mset_fail_arity)
{
	PROTO proto;
	med::decoder_context<> ctx;

	//arity underflow in mandatory field
	uint8_t const mandatory_underflow[] = { 0x14
		, 0, 0x0b, 0x11
		, 0, 0x0c, 0x13
		, 0, 0x21, 2, 0x35, 0xD9
	};
	ctx.reset(mandatory_underflow, sizeof(mandatory_underflow));
	EXPECT_THROW(decode(med::octet_decoder{ctx}, proto), med::missing_ie);

	//arity overflow in mandatory field
	uint8_t const mandatory_overflow[] = { 0x14
		, 0, 0x0b, 0x11
		, 0, 0x0b, 0x12
		, 0, 0x0c, 0x13
		, 0, 0x0c, 0x14
		, 0, 0x0c, 0x15
		, 0, 0x21, 2, 0x35, 0xD9
	};
	ctx.reset(mandatory_overflow, sizeof(mandatory_overflow));
	EXPECT_THROW(decode(med::octet_decoder{ctx}, proto), med::extra_ie);
}

