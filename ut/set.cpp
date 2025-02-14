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

	auto& msg = proto.ref<MSG_SET>();

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
	auto& msg = proto.ref<MSG_SET>();
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

	auto& msg = proto.ref<MSG_MSET>();

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
	auto& msg = proto.ref<MSG_MSET>();
	msg.ref<FLD_UC> ().push_back(ctx)->set(0);
	msg.ref<FLD_UC> ().push_back(ctx)->set(0);
	msg.ref<FLD_U8> ().push_back(ctx)->set(0);
	msg.ref<FLD_U16>().push_back(ctx)->set(0);
	encode(med::octet_encoder{ctx}, proto);

	msg.ref<FLD_U24>().push_back(ctx)->set(0);
	EXPECT_THROW(encode(med::octet_encoder{ctx}, proto), med::missing_ie);
}

TEST(encode, set_func_ok)
{
	PROTO proto;

	MSG_SET_FUNC& msg = proto.ref<MSG_SET_FUNC>();

	//one mandatory, one conditional + setter, one optional
	msg.ref<FLD_UC>().set(0x11);
	msg.ref<FLD_U16>().set(3);
	msg.ref<FLD_IP>().set(1);

	uint8_t buffer[1024];
	med::encoder_context ctx{buffer};

	EXPECT_NO_THROW(encode(med::octet_encoder{ctx}, proto));

	// M< T16<0x0b>, FLD_UC >, //<TV>
	// M< T16<0x0d>, FLD_FLAGS, FLD_FLAGS::setter >,
	// O< T16<0x21>, FLD_U16, FLD_FLAGS::has_bits<FLD_FLAGS::U16> >,
	// O< T16<0x89>, FLD_IP >
	uint8_t const encoded1[] = { 0x24
		, 0, 0x0b, 0x11
		, 0, 0x0d, 0b0000'0101
		, 0, 0x21, 0x00, 0x03
		, 0, 0x89, 0x00, 0x00, 0x00, 0x01
	};
	EXPECT_EQ(sizeof(encoded1), ctx.buffer().get_offset());
	EXPECT_TRUE(Matches(encoded1, buffer));

	//one mandatory + setter
	ctx.reset();
	msg.clear();
	msg.ref<FLD_UC>().set(0x11);
	EXPECT_NO_THROW(encode(med::octet_encoder{ctx}, proto));

	// M< T16<0x0b>, FLD_UC >, //<TV>
	// M< T16<0x0d>, FLD_FLAGS, FLD_FLAGS::setter >,
	uint8_t const encoded2[] = { 0x24
		, 0, 0x0b, 0x11
		, 0, 0x0d, 0b0000'0000
	};
	EXPECT_EQ(sizeof(encoded2), ctx.buffer().get_offset());
	EXPECT_TRUE(Matches(encoded2, buffer));

	//full msg
	ctx.reset();
	msg.clear();
	msg.ref<FLD_UC>().set(0x11);
	msg.ref<FLD_U16>().set(3);
	msg.ref<FLD_IP>().set(1);
	msg.ref<FLD_U24>().set(2);
	msg.ref<FLD_U8>().set(4);
	msg.ref<FLD_QTY>().set(5);

	// try to set wrong flags
	msg.ref<FLD_FLAGS>().set(255);

	EXPECT_NO_THROW(encode(med::octet_encoder{ctx}, proto));

	// M< T16<0x0b>, FLD_UC >, //<TV>
	// M< T16<0x0d>, FLD_FLAGS, FLD_FLAGS::setter >,
	// O< T16<0x0e>, FLD_QTY, FLD_FLAGS::has_bits<FLD_FLAGS::QTY> >,
	// O< T16<0x0c>, FLD_U8 >, //<TV>
	// O< T16<0x21>, FLD_U16, FLD_FLAGS::has_bits<FLD_FLAGS::U16> >,
	// O< T16<0x49>, FLD_U24, FLD_FLAGS::has_bits<FLD_FLAGS::U24> >,
	// O< T16<0x89>, FLD_IP >
	uint8_t const encoded3[] = { 0x24
		, 0, 0x0b, 0x11
		, 0, 0x0d, 0b0100'0111
		, 0, 0x0e, 0x05
		, 0, 0x0c, 0x04
		, 0, 0x21, 0x00, 0x03
		, 0, 0x49, 0x00, 0x00, 0x02
		, 0, 0x89, 0x00, 0x00, 0x00, 0x01
	};

	EXPECT_EQ(sizeof(encoded3), ctx.buffer().get_offset());
	EXPECT_TRUE(Matches(encoded3, buffer));
}

TEST(encode, set_func_fail)
{
	PROTO proto;

	MSG_SET_FUNC& msg = proto.ref<MSG_SET_FUNC>();

	//NO mandatory, one conditional + setter, one optional
	msg.ref<FLD_U16>().set(3);
	msg.ref<FLD_IP>().set(1);

	uint8_t buffer[1024];
	med::encoder_context ctx{buffer};

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

	auto const* msg = proto.get<MSG_SET>();
	ASSERT_NE(nullptr, msg);

	ASSERT_EQ(0x11, msg->get<FLD_UC>().get());
	ASSERT_EQ(0x35D9, msg->get<FLD_U16>().get());
	auto* fld3 = msg->get<FLD_U24>();
	auto* fld4 = msg->get<FLD_IP>();
	auto* vfld1 = msg->get<VFLD1>();
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

	msg = proto.get<MSG_SET>();
	ASSERT_NE(nullptr, msg);

	fld3 = msg->get<FLD_U24>();
	fld4 = msg->get<FLD_IP>();
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

	msg = proto.get<MSG_SET>();
	ASSERT_NE(nullptr, msg);

	fld3 = msg->get<FLD_U24>();
	fld4 = msg->get<FLD_IP>();
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

	auto const* msg = proto.get<MSG_MSET>();
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

	msg = proto.get<MSG_MSET>();
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

	msg = proto.get<MSG_MSET>();
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

TEST(decode, set_func_ok)
{
	PROTO proto;

	//mandatory fields only
	uint8_t const encoded1[] = { 0x24
		, 0, 0x0b, 0x11
		, 0, 0x0d, 0x00,
	};
	med::decoder_context ctx{encoded1};
	EXPECT_NO_THROW(decode(med::octet_decoder{ctx}, proto));
	{
		MSG_SET_FUNC const* msg = proto.get<MSG_SET_FUNC>();
		ASSERT_NE(nullptr, msg);

		ASSERT_EQ(0x11, msg->get<FLD_UC>().get());
		ASSERT_EQ(0x00, msg->get<FLD_FLAGS>().get());
		FLD_U24 const* fld1 = msg->get<FLD_U24>();
		FLD_U16 const* fld2 = msg->get<FLD_U16>();
		FLD_U8 const* fld3 = msg->get<FLD_U8>();
		FLD_IP const* fld4 = msg->get<FLD_IP>();
		FLD_QTY const* fld5 = msg->get<FLD_QTY>();
		ASSERT_EQ(nullptr, fld1);
		ASSERT_EQ(nullptr, fld2);
		ASSERT_EQ(nullptr, fld3);
		ASSERT_EQ(nullptr, fld4);
		ASSERT_EQ(nullptr, fld5);
	}
	//all fields but in reverse order
	uint8_t const encoded2[] = { 0x24
		, 0, 0x89, 0x00, 0x00, 0x00, 0x01
		, 0, 0x49, 0x00, 0x00, 0x02
		, 0, 0x21, 0x00, 0x03
		, 0, 0x0c, 0x04
		, 0, 0x0e, 0x05
		, 0, 0x0b, 0x11
		, 0, 0x0d, 0b0100'0111,
	};
	ctx.reset(encoded2);
	EXPECT_NO_THROW(decode(med::octet_decoder{ctx}, proto));
	{
		MSG_SET_FUNC const* msg = proto.get<MSG_SET_FUNC>();
		ASSERT_NE(nullptr, msg);

		ASSERT_EQ(0x11, msg->get<FLD_UC>().get());
		FLD_U24 const* fld1 = msg->get<FLD_U24>();
		FLD_U16 const* fld2 = msg->get<FLD_U16>();
		FLD_U8 const* fld3 = msg->get<FLD_U8>();
		FLD_IP const* fld4 = msg->get<FLD_IP>();
		FLD_QTY const* fld5 = msg->get<FLD_QTY>();
		ASSERT_NE(nullptr, fld1);
		ASSERT_NE(nullptr, fld2);
		ASSERT_NE(nullptr, fld3);
		ASSERT_NE(nullptr, fld4);
		ASSERT_NE(nullptr, fld5);
		ASSERT_EQ(2, fld1->get());
		ASSERT_EQ(3, fld2->get());
		ASSERT_EQ(4, fld3->get());
		ASSERT_EQ(1, fld4->get());
		ASSERT_EQ(5, fld5->get());
		// ASSERT_EQ(0b0010'1111, msg->get<FLD_FLAGS>().get());
		ASSERT_TRUE(FLD_FLAGS::has_bits<FLD_FLAGS::U16>{}(msg->body()));
		ASSERT_TRUE(FLD_FLAGS::has_bits<FLD_FLAGS::U24>{}(msg->body()));
		ASSERT_TRUE(FLD_FLAGS::has_bits<FLD_FLAGS::QTY>{}(msg->body()));
		constexpr size_t u8_qty = 1;
		ASSERT_TRUE(FLD_FLAGS::has_bits<u8_qty << FLD_FLAGS::U8_QTY>{}(msg->body()));
		auto const uc_qty_minus_one = 1 - 1;
		ASSERT_FALSE(FLD_FLAGS::has_bits<uc_qty_minus_one << FLD_FLAGS::UC_QTY>{}(msg->body()));
	}
}

TEST(decode, set_func_fail)
{
	PROTO proto;

	//mandatory fields only, but some flags are set
	uint8_t const encoded1[] = { 0x24
		, 0, 0x0b, 0x11
		, 0, 0x0d,  0b0100'0111
	};
	med::decoder_context ctx{encoded1};
	EXPECT_THROW(decode(med::octet_decoder{ctx}, proto), med::missing_ie);

	//some conditional fields, but flags are NOT set
	uint8_t const encoded2[] = { 0x24
		, 0, 0x49, 0x00, 0x00, 0x02
		, 0, 0x21, 0x00, 0x03
		, 0, 0x0c, 0x04
		, 0, 0x0b, 0x11
		, 0, 0x0d,  0b0100'0000
	};
	ctx.reset(encoded2);
	EXPECT_THROW(decode(med::octet_decoder{ctx}, proto), med::extra_ie);
}
