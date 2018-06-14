
#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wgnu-zero-variadic-macro-arguments"
#endif
//#pragma clang diagnostic pop

#include "encode.hpp"
#include "decode.hpp"
#include "update.hpp"
#include "printer.hpp"
#include "encoder_context.hpp"
#include "decoder_context.hpp"
#include "octet_encoder.hpp"
#include "octet_decoder.hpp"

#include "ut.hpp"
#include "ut_proto.hpp"


TEST(name, tag)
{
	PROTO proto;

	static_assert(nullptr != PROTO::name_tag(1));
	auto atag = 1;
	EXPECT_STREQ(med::name<MSG_SEQ>(), PROTO::name_tag(atag));
	static_assert(nullptr == PROTO::name_tag(0xAA));
	atag = 0x55;
	EXPECT_EQ(nullptr, PROTO::name_tag(atag));

	static_assert(nullptr != MSG_SET::name_tag(0x0b));

	atag = 0x21;
	EXPECT_STREQ(med::name<FLD_U16>(), MSG_SET::name_tag(atag));
	static_assert(nullptr == MSG_SET::name_tag(0xAA));
	atag = 0x55;
	EXPECT_EQ(nullptr, MSG_SET::name_tag(atag));
}

#if 1
TEST(encode, seq_ok)
{
	PROTO proto;

	//mandatory only
	MSG_SEQ& msg = proto.select();
	msg.ref<FLD_UC>().set(37);
	msg.ref<FLD_U16>().set(0x35D9);
	msg.ref<FLD_U24>().set(0xDABEEF);
	msg.ref<FLD_IP>().set(0xFee1ABBA);

	//re-selection of current choice doesn't destroy contents
	proto.ref<MSG_SEQ>();

	uint8_t buffer[1024];
	med::encoder_context<> ctx{ buffer };

#if (MED_EXCEPTIONS)
	encode(med::octet_encoder{ctx}, proto);
#else
	ASSERT_TRUE(encode(med::octet_encoder{ctx}, proto)) << toString(ctx.error_ctx());
	EXPECT_EQ(nullptr, toString(ctx.error_ctx()));
#endif
	uint8_t const encoded1[] = { 1
		, 37
		, 0x21, 0x35, 0xD9
		, 3, 0xDA, 0xBE, 0xEF
		, 0x42, 4, 0xFE, 0xE1, 0xAB, 0xBA
	};
	EXPECT_EQ(sizeof(encoded1), ctx.buffer().get_offset());
	EXPECT_TRUE(Matches(encoded1, buffer));

	//with 1 optional
	ctx.reset();
	msg.ref<FLD_DW>().set(0x01020304);

#if (MED_EXCEPTIONS)
	encode(med::octet_encoder{ctx}, proto);
#else
	ASSERT_TRUE(encode(med::octet_encoder{ctx}, proto)) << toString(ctx.error_ctx());
#endif
	uint8_t const encoded2[] = { 1
		, 37
		, 0x21, 0x35, 0xD9
		, 3, 0xDA, 0xBE, 0xEF
		, 0x42, 4, 0xFE, 0xE1, 0xAB, 0xBA
		, 0x51, 0x01, 0x02, 0x03, 0x04
	};
	EXPECT_EQ(sizeof(encoded2), ctx.buffer().get_offset());
	EXPECT_TRUE(Matches(encoded2, buffer));

	//with 2 optionals
	ctx.reset();
	msg.ref<VFLD1>().set("test.this!");

#if (MED_EXCEPTIONS)
	encode(med::octet_encoder{ctx}, proto);
#else
	ASSERT_TRUE(encode(med::octet_encoder{ctx}, proto)) << toString(ctx.error_ctx());
#endif
	uint8_t const encoded3[] = { 1
		, 37
		, 0x21, 0x35, 0xD9
		, 3, 0xDA, 0xBE, 0xEF
		, 0x42, 4, 0xFE, 0xE1, 0xAB, 0xBA
		, 0x51, 0x01, 0x02, 0x03, 0x04
		, 0x12, 4, 't', 'e', 's', 't', '.', 't', 'h', 'i', 's', '!'
	};
	EXPECT_EQ(sizeof(encoded3), ctx.buffer().get_offset());
	EXPECT_TRUE(Matches(encoded3, buffer));

	//check RO access (compile test)
	{
		MSG_SEQ const& cmsg = msg;
		FLD_DW const* cpf = cmsg.field();
		ASSERT_NE(nullptr, cpf);
		FLD_DW& rf = msg.field();
		ASSERT_EQ(0x01020304, rf.get());
		//FLD_DW* pf = msg.field(); //invalid access
	}
}
#endif

#if 1
TEST(encode, seq_fail_mandatory)
{
	PROTO proto;
	uint8_t buffer[1024];
	med::encoder_context<> ctx{ buffer };

	MSG_SEQ& msg = proto.select();
	msg.ref<FLD_UC>().set(0);
	msg.ref<FLD_U24>().set(0);
	//ctx.reset();

#if (MED_EXCEPTIONS)
	EXPECT_THROW(encode(med::octet_encoder{ctx}, proto), med::missing_ie);
#else
	ASSERT_FALSE(encode(med::octet_encoder{ctx}, proto));
	EXPECT_EQ(med::error::MISSING_IE, ctx.error_ctx().get_error()) << toString(ctx.error_ctx());
	EXPECT_NE(nullptr, toString(ctx.error_ctx()));
#endif //MED_EXCEPTIONS
}

TEST(encode, mseq_ok)
{
	uint8_t buffer[1024];
	med::encoder_context<> ctx{ buffer };

	PROTO proto;

	//mandatory only
	MSG_MSEQ& msg = proto.select();
	static_assert(MSG_MSEQ::arity<FLD_UC>() == 2, "");
	msg.push_back<FLD_UC>(ctx)->set(37);
	msg.push_back<FLD_UC>(ctx)->set(38);
	msg.push_back<FLD_U16>(ctx)->set(0x35D9);
	msg.push_back<FLD_U16>(ctx)->set(0x35DA);
	msg.push_back<FLD_U24>(ctx)->set(0xDABEEF);
	msg.push_back<FLD_U24>(ctx)->set(0x22BEEF);
	msg.push_back<FLD_IP>(ctx)->set(0xFee1ABBA);
	msg.push_back<FLD_DW>(ctx)->set(0x01020304);
	{
		auto* s = msg.push_back<SEQOF_3<0>>();
		ASSERT_NE(nullptr, s);
		s->ref<FLD_U8>().set(1);
		s->ref<FLD_U16>().set(2);
	}

#if (MED_EXCEPTIONS)
	encode(med::octet_encoder{ctx}, proto);
#else
	ASSERT_TRUE(encode(med::octet_encoder{ctx}, proto)) << toString(ctx.error_ctx());
#endif //MED_EXCEPTIONS

	uint8_t const encoded1[] = { 0x11
		, 37                              //M< FLD_UC, med::arity<2> >
		, 38
		, 0x21, 0x35, 0xD9                //M< T<0x21>, FLD_U16, med::arity<2> >
		, 0x21, 0x35, 0xDA
		, 3, 0xDA, 0xBE, 0xEF             //M< L, FLD_U24, med::arity<2> >
		, 3, 0x22, 0xBE, 0xEF
		, 0x42, 4, 0xFE, 0xE1, 0xAB, 0xBA //M< T<0x42>, L, FLD_IP, med::max<2> >
		, 0x51, 0x01, 0x02, 0x03, 0x04    //M< T<0x51>, FLD_DW, med::max<2> >
		, 0,1, 1, 0x21, 0,2 //M<C16, Seq>  <=2
	};
	EXPECT_EQ(sizeof(encoded1), ctx.buffer().get_offset());
	EXPECT_TRUE(Matches(encoded1, buffer));

	//with more mandatory and optionals
	ctx.reset();

	msg.push_back<FLD_IP>(ctx)->set(0xABBAc001);
	msg.push_back<FLD_DW>(ctx)->set(0x12345678);

	msg.ref<FLD_TN>().set(2);

	msg.ref<FLD_CHO>().ref<FLD_U8>().set(33);
	SEQOF_1& s1 = msg.ref<SEQOF_1>();
	s1.push_back<FLD_W>(ctx)->set(0x1223);
	s1.push_back<FLD_W>(ctx)->set(0x3445);

	SEQOF_2* s2 = msg.push_back<SEQOF_2>(ctx);
	s2->ref<FLD_W>().set(0x1122);
	s2->ref<FLD_CHO>().ref<FLD_U16>().set(0x3344);

	msg.push_back<SEQOF_2>(ctx)->ref<FLD_W>().set(0x3344);

	uint8_t const bcd[] = {0x34, 0x56};
	msg.ref<FLD_NSCHO>().ref<BCD_1>().set(2, bcd);

	//O< T<0x80>, CNT, SEQOF_3<1>, med::max<3>>, //T[CV]*[0,3]
//	{
//		auto* s = msg.push_back<SEQOF_3<1>>(ctx);
//		s->ref<FLD_U8>().set(7);
//		s->ref<FLD_U16>().set(0x7654);
//		s = msg.push_back<SEQOF_3<1>>(ctx);
//		s->ref<FLD_U8>().set(9);
//		s->ref<FLD_U16>().set(0x9876);
//	}

	msg.push_back<VFLD1>(ctx)->set("test.this");
	msg.push_back<VFLD1>(ctx)->set("test.it");

#if (MED_EXCEPTIONS)
	encode(med::octet_encoder{ctx}, proto);
#else
	ASSERT_TRUE(encode(med::octet_encoder{ctx}, proto)) << toString(ctx.error_ctx());
#endif //MED_EXCEPTIONS
	uint8_t const encoded2[] = { 0x11
		, 37, 38 //2*<FLD_UC>
		, 0x21, 0x35, 0xD9 //2*<T=0x21, FLD_U16>
		, 0x21, 0x35, 0xDA
		, 3, 0xDA, 0xBE, 0xEF //2*<L, FLD_U24>
		, 3, 0x22, 0xBE, 0xEF
		, 0x42, 4, 0xFE, 0xE1, 0xAB, 0xBA //2*<T=0x42, L, FLD_IP>
		, 0x42, 4, 0xAB, 0xBA, 0xC0, 0x01
		, 0x51, 0x01, 0x02, 0x03, 0x04 //2*<T=0x51, FLD_DW>
		, 0x51, 0x12, 0x34, 0x56, 0x78
		, 0,1, 1, 0x21, 0,2 //1*<C16, SEQOF_3<u8, 21h=u16>
		, 0xE2 //[FLD_TN]
		, 0x60, 2, 0, 33 //[T=0x60, L, FLD_CHO]
		, 0x61, 4 //[T=0x61, L, SEQOF_1]
			, 0x12,0x23, 0x34,0x45 //2*<FLD_W>
		, 0x62, 7 //[T=0x62, L, SEQOF_2]
			, 0x11,0x22 //<FLD_W>
			, 0x06, 3, 2, 0x33,0x44 //O[T=0x06, L, FLD_CHO]
		, 0x62, 2 //[T=0x62, L, SEQOF_2]
			, 0x33,0x44 //<FLD_W>
			//O< T<0x06>, L, FLD_CHO >
		, 0x70, 3, 0x31, 0x45, 0x6F //[T=0x70, L, FLD_NSCHO]
//		, 0x80, 0,2,  7, 0x76,0x54,  9, 0x98, 0x76 //[T=0x80, C16, SEQOF_3<u8, 21h=u16>]
		, 9, 't', 'e', 's', 't', '.', 't', 'h', 'i', 's' //2*[L, VFLD1]
		, 7, 't', 'e', 's', 't', '.', 'i', 't'
	};

	EXPECT_EQ(sizeof(encoded2), ctx.buffer().get_offset());
	EXPECT_TRUE(Matches(encoded2, buffer));

	//check RO access (compile test)
	{
		MSG_MSEQ const& cmsg = msg;
		auto& rdw = cmsg.get<FLD_DW>();
		auto it = rdw.begin();
		ASSERT_NE(rdw.end(), it);
		ASSERT_EQ(0x01020304, it->get());
		++it;
		ASSERT_NE(rdw.end(), it);
		ASSERT_EQ(0x12345678, it->get());
//		FLD_DW const* rf = msg.field(1);
//		ASSERT_EQ(0x12345678, rf->get());
		//FLD_DW& rpf = msg.field(); //invalid access
		//FLD_DW* pf = msg.field(); //invalid access
	}
}

TEST(encode, mseq_fail_arity)
{
	uint8_t buffer[1024];
	med::encoder_context<> ctx{ buffer };

	//arity of <V> violation
	{
		PROTO proto;
		MSG_MSEQ& msg = proto.select();
		msg.push_back<FLD_UC>(ctx)->set(37);
		msg.push_back<FLD_U16>(ctx)->set(0x35D9);
		msg.push_back<FLD_U16>(ctx)->set(0x35DA);
		msg.push_back<FLD_U24>(ctx)->set(0xDABEEF);
		msg.push_back<FLD_U24>(ctx)->set(0x22BEEF);
		msg.push_back<FLD_IP>(ctx)->set(0xFee1ABBA);
		msg.push_back<FLD_IP>(ctx)->set(0xABBAc001);
		msg.push_back<FLD_DW>(ctx)->set(0x01020304);
		msg.push_back<FLD_DW>(ctx)->set(0x12345678);
		msg.push_back<VFLD1>(ctx)->set("test.this");

		ctx.reset();
#if (MED_EXCEPTIONS)
		EXPECT_THROW(encode(med::octet_encoder{ctx}, proto), med::missing_ie);
#else
		ASSERT_FALSE(encode(med::octet_encoder{ctx}, proto));
		EXPECT_EQ(med::error::MISSING_IE, ctx.error_ctx().get_error()) << toString(ctx.error_ctx());
		EXPECT_NE(nullptr, toString(ctx.error_ctx()));
#endif //MED_EXCEPTIONS
	}

	//arity of <TV> violation
	{
		PROTO proto;
		MSG_MSEQ& msg = proto.select();
		msg.push_back<FLD_UC>(ctx)->set(37);
		msg.push_back<FLD_UC>(ctx)->set(38);
		msg.push_back<FLD_U16>(ctx)->set(0x35D9);
		msg.push_back<FLD_U24>(ctx)->set(0xDABEEF);
		msg.push_back<FLD_U24>(ctx)->set(0x22BEEF);
		msg.push_back<FLD_IP>(ctx)->set(0xFee1ABBA);
		msg.push_back<FLD_IP>(ctx)->set(0xABBAc001);
		msg.push_back<FLD_DW>(ctx)->set(0x01020304);
		msg.push_back<FLD_DW>(ctx)->set(0x12345678);
		msg.push_back<VFLD1>(ctx)->set("test.this");

		ctx.reset();
#if (MED_EXCEPTIONS)
		EXPECT_THROW(encode(med::octet_encoder{ctx}, proto), med::missing_ie);
#else
		ASSERT_FALSE(encode(med::octet_encoder{ctx}, proto));
		EXPECT_EQ(med::error::MISSING_IE, ctx.error_ctx().get_error()) << toString(ctx.error_ctx());
		EXPECT_NE(nullptr, toString(ctx.error_ctx()));
#endif //MED_EXCEPTIONS
	}

	//arity of <LV> violation
	{
		PROTO proto;
		MSG_MSEQ& msg = proto.select();
		msg.push_back<FLD_UC>(ctx)->set(37);
		msg.push_back<FLD_UC>(ctx)->set(38);
		msg.push_back<FLD_U16>(ctx)->set(0x35D9);
		msg.push_back<FLD_U16>(ctx)->set(0x35DA);
		msg.push_back<FLD_U24>(ctx)->set(0xDABEEF);
		msg.push_back<FLD_IP>(ctx)->set(0xFee1ABBA);
		msg.push_back<FLD_IP>(ctx)->set(0xABBAc001);
		msg.push_back<FLD_DW>(ctx)->set(0x01020304);
		msg.push_back<FLD_DW>(ctx)->set(0x12345678);
		msg.push_back<VFLD1>(ctx)->set("test.this");

		ctx.reset();
#if (MED_EXCEPTIONS)
		EXPECT_THROW(encode(med::octet_encoder{ctx}, proto), med::missing_ie);
#else
		ASSERT_FALSE(encode(med::octet_encoder{ctx}, proto));
		EXPECT_EQ(med::error::MISSING_IE, ctx.error_ctx().get_error()) << toString(ctx.error_ctx());
		EXPECT_NE(nullptr, toString(ctx.error_ctx()));
#endif //MED_EXCEPTIONS
	}

	//arity of <TLV> violation
	{
		PROTO proto;
		MSG_MSEQ& msg = proto.select();
		msg.push_back<FLD_UC>(ctx)->set(37);
		msg.push_back<FLD_UC>(ctx)->set(38);
		msg.push_back<FLD_U16>(ctx)->set(0x35D9);
		msg.push_back<FLD_U16>(ctx)->set(0x35DA);
		msg.push_back<FLD_U24>(ctx)->set(0xDABEEF);
		msg.push_back<FLD_U24>(ctx)->set(0x22BEEF);
		msg.push_back<FLD_DW>(ctx)->set(0x01020304);
		msg.push_back<FLD_DW>(ctx)->set(0x12345678);
		msg.push_back<VFLD1>(ctx)->set("test.this");

		ctx.reset();
#if (MED_EXCEPTIONS)
		EXPECT_THROW(encode(med::octet_encoder{ctx}, proto), med::missing_ie);
#else
		ASSERT_FALSE(encode(med::octet_encoder{ctx}, proto));
		EXPECT_EQ(med::error::MISSING_IE, ctx.error_ctx().get_error()) << toString(ctx.error_ctx());
		EXPECT_NE(nullptr, toString(ctx.error_ctx()));
#endif //MED_EXCEPTIONS
	}
}

TEST(encode, mseq_fail_overflow)
{
	uint8_t buffer[1024];
	med::encoder_context<> ctx{ buffer };

	//output buffer overflow
	{
		PROTO proto;
		MSG_MSEQ& msg = proto.select();
		msg.push_back<FLD_UC>(ctx)->set(37);
		msg.push_back<FLD_UC>(ctx)->set(38);
		msg.push_back<FLD_U16>(ctx)->set(0x35D9);
		msg.push_back<FLD_U16>(ctx)->set(0x35DA);
		msg.push_back<FLD_U24>(ctx)->set(0xDABEEF);
		msg.push_back<FLD_U24>(ctx)->set(0x22BEEF);
		msg.push_back<FLD_IP>(ctx)->set(0xFee1ABBA);
		msg.push_back<FLD_DW>(ctx)->set(0x01020304);

		uint8_t buffer[10];
		med::encoder_context<> ctx{ buffer };

		ctx.reset();
#if (MED_EXCEPTIONS)
		EXPECT_THROW(encode(med::octet_encoder{ctx}, proto), med::overflow);
#else
		ASSERT_FALSE(encode(med::octet_encoder{ctx}, proto));
		EXPECT_EQ(med::error::OVERFLOW, ctx.error_ctx().get_error()) << toString(ctx.error_ctx());
		EXPECT_NE(nullptr, toString(ctx.error_ctx()));
#endif //MED_EXCEPTIONS
	}

}

TEST(encode, set_ok)
{
	PROTO proto;

	MSG_SET& msg = proto.select();

	//mandatory fields
	msg.ref<FLD_UC>().set(0x11);
	msg.ref<FLD_U16>().set(0x35D9);

	uint8_t buffer[1024];
	med::encoder_context<> ctx{ buffer };

#if (MED_EXCEPTIONS)
	encode(med::octet_encoder{ctx}, proto);
#else
	ASSERT_TRUE(encode(med::octet_encoder{ctx}, proto)) << toString(ctx.error_ctx());
#endif //MED_EXCEPTIONS

	uint8_t const encoded1[] = { 4
		, 0, 0x0b, 0x11
		, 0, 0x21, 2, 0x35, 0xD9
	};
	EXPECT_EQ(sizeof(encoded1), ctx.buffer().get_offset());
	EXPECT_TRUE(Matches(encoded1, buffer));

	//optional fields
	ctx.reset();
	msg.ref<VFLD1>().set("test.this");

#if (MED_EXCEPTIONS)
	encode(med::octet_encoder{ctx}, proto);
#else
	ASSERT_TRUE(encode(med::octet_encoder{ctx}, proto)) << toString(ctx.error_ctx());
#endif

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

#if (MED_EXCEPTIONS)
	encode(med::octet_encoder{ctx}, proto);
#else
	ASSERT_TRUE(encode(med::octet_encoder{ctx}, proto)) << toString(ctx.error_ctx());
#endif

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
#if (MED_EXCEPTIONS)
	EXPECT_THROW(encode(med::octet_encoder{ctx}, proto), med::missing_ie);
#else
	ASSERT_FALSE(encode(med::octet_encoder{ctx}, proto));
	EXPECT_EQ(med::error::MISSING_IE, ctx.error_ctx().get_error()) << toString(ctx.error_ctx());
	EXPECT_NE(nullptr, toString(ctx.error_ctx()));
#endif
}

TEST(encode, mset_ok)
{
	uint8_t buffer[1024];
	med::encoder_context<> ctx{ buffer };

	PROTO proto;

	MSG_MSET& msg = proto.select();

	//mandatory fields
	msg.push_back<FLD_UC>(ctx)->set(0x11);
	msg.push_back<FLD_UC>(ctx)->set(0x12);
	msg.push_back<FLD_U8>(ctx)->set(0x13);
	msg.push_back<FLD_U16>(ctx)->set(0x35D9);
	msg.push_back<FLD_U16>(ctx)->set(0x35DA);

#if (MED_EXCEPTIONS)
	encode(med::octet_encoder{ctx}, proto);
#else
	ASSERT_TRUE(encode(med::octet_encoder{ctx}, proto)) << toString(ctx.error_ctx());
#endif

	uint8_t const encoded1[] = { 0x14
		, 0, 0x0b, 0x11
		, 0, 0x0b, 0x12
		, 0, 0x0c, 0x13
		, 0, 0x21, 2, 0x35, 0xD9
		, 0, 0x21, 2, 0x35, 0xDA
	};
	EXPECT_EQ(sizeof(encoded1), ctx.buffer().get_offset());
	EXPECT_TRUE(Matches(encoded1, buffer));

	//optional fields
	ctx.reset();
	msg.push_back<VFLD1>(ctx)->set("test.this");

#if (MED_EXCEPTIONS)
	encode(med::octet_encoder{ctx}, proto);
#else
	ASSERT_TRUE(encode(med::octet_encoder{ctx}, proto)) << toString(ctx.error_ctx());
#endif

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
	msg.push_back<FLD_U8>(ctx)->set(0x14);
	msg.push_back<FLD_U24>(ctx)->set(0xDABEEF);
	msg.push_back<FLD_U24>(ctx)->set(0x22BEEF);
	msg.push_back<FLD_IP>(ctx)->set(0xfee1ABBA);
	msg.push_back<FLD_IP>(ctx)->set(0xABBAc001);
	msg.push_back<VFLD1>(ctx)->set("test.it");

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

#if (MED_EXCEPTIONS)
	encode(med::octet_encoder{ctx}, proto);
#else
	ASSERT_TRUE(encode(med::octet_encoder{ctx}, proto)) << toString(ctx.error_ctx());
#endif

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
	msg.push_back<FLD_UC>(ctx)->set(0);
	msg.push_back<FLD_UC>(ctx)->set(0);
	msg.push_back<FLD_U8>(ctx)->set(0);
	msg.push_back<FLD_U16>(ctx)->set(0);
#if (MED_EXCEPTIONS)
	encode(med::octet_encoder{ctx}, proto);
#else
	ASSERT_TRUE(encode(med::octet_encoder{ctx}, proto)) << toString(ctx.error_ctx());
#endif

	msg.push_back<FLD_U24>(ctx)->set(0);
#if (MED_EXCEPTIONS)
	EXPECT_THROW(encode(med::octet_encoder{ctx}, proto), med::missing_ie);
#else
	ASSERT_FALSE(encode(med::octet_encoder{ctx}, proto));
	EXPECT_EQ(med::error::MISSING_IE, ctx.error_ctx().get_error()) << toString(ctx.error_ctx());
	EXPECT_NE(nullptr, toString(ctx.error_ctx()));
#endif
}

constexpr std::size_t make_hash(std::size_t v) { return 137*(v + 39); }

TEST(encode, msg_func)
{
	uint8_t buffer[1024];
	med::encoder_context<> ctx{ buffer };

	PROTO proto;

	//mandatory only
	MSG_FUNC& msg = proto.select();
	msg.push_back<FLD_UC>(ctx)->set(37);
	msg.push_back<FLD_UC>(ctx)->set(38);

#if (MED_EXCEPTIONS)
	encode(med::octet_encoder{ctx}, proto);
#else
	ASSERT_TRUE(encode(med::octet_encoder{ctx}, proto)) << toString(ctx.error_ctx());
#endif

	uint8_t const encoded1[] = { 0xFF
		, 0x10
		, 37, 38
	};
	EXPECT_EQ(sizeof(encoded1), ctx.buffer().get_offset());
	EXPECT_TRUE(Matches(encoded1, buffer));

	//with 1 optional and 1 optional counted
	ctx.reset();
	msg.push_back<FLD_UC>(ctx)->set(39);
	msg.push_back<FLD_U8>(ctx)->set('a');
	msg.ref<FLD_U16>().set(0x35D9);

#if (MED_EXCEPTIONS)
	encode(med::octet_encoder{ctx}, proto);
#else
	ASSERT_TRUE(encode(med::octet_encoder{ctx}, proto)) << toString(ctx.error_ctx());
#endif

	uint8_t const encoded2[] = { 0xFF
		, 0x61
		, 37, 38, 39
		, 'a'
		, 0x35, 0xD9
	};
	EXPECT_EQ(sizeof(encoded2), ctx.buffer().get_offset());
	EXPECT_TRUE(Matches(encoded2, buffer));

	//with all optionals
	ctx.reset();
	msg.push_back<FLD_UC>(ctx)->set(40);
	msg.push_back<FLD_U8>(ctx)->set('b');
	msg.ref<FLD_U24>().set(0xDABEEF);
	for (uint8_t i = 0; i < 3; ++i)
	{
		FLD_IP* p = msg.push_back<FLD_IP>(ctx);
		ASSERT_NE(nullptr, p);
		p->set(make_hash(i));
	}

#if (MED_EXCEPTIONS)
	encode(med::octet_encoder{ctx}, proto);
#else
	ASSERT_TRUE(encode(med::octet_encoder{ctx}, proto)) << toString(ctx.error_ctx());
#endif

	uint8_t const encoded3[] = { 0xFF
		, 0xB7
		, 37, 38, 39, 40
		, 3
		, 'a', 'b'
		, 0x35, 0xD9
		, 0xDA, 0xBE, 0xEF
		, 0, 0, 0x14, 0xDF
		, 0, 0, 0x15, 0x68
		, 0, 0, 0x15, 0xF1
	};
	EXPECT_EQ(sizeof(encoded3), ctx.buffer().get_offset());
	EXPECT_TRUE(Matches(encoded3, buffer));
}

TEST(decode, seq_ok)
{
	PROTO proto;

	//mandatory only
	uint8_t const encoded1[] = { 1
		, 37
		, 0x21, 0x35, 0xD9
		, 3, 0xDA, 0xBE, 0xEF
		, 0x42, 4, 0xFE, 0xE1, 0xAB, 0xBA
	};
	med::decoder_context<> ctx;

	ctx.reset(encoded1, sizeof(encoded1));
#if (MED_EXCEPTIONS)
	decode(med::octet_decoder{ctx}, proto);
#else
	ASSERT_TRUE(decode(med::octet_decoder{ctx}, proto)) << toString(ctx.error_ctx());
#endif

	//check RO access (compile test)
	{
		PROTO const& cproto = proto;
		MSG_SEQ const* cmsg = cproto.select();
		ASSERT_NE(nullptr, cmsg);
	}

	MSG_SEQ const* msg = proto.select();
	ASSERT_NE(nullptr, msg);
	EXPECT_EQ(37, msg->get<FLD_UC>().get());
	EXPECT_EQ(1, msg->count<FLD_UC>());
	EXPECT_EQ(0x35D9, msg->get<FLD_U16>().get());
	EXPECT_EQ(0xDABEEF, msg->get<FLD_U24>().get());
	EXPECT_EQ(0xfee1ABBA, msg->get<FLD_IP>().get());

	EXPECT_EQ(0, msg->count<FLD_DW>());
	FLD_DW const* fld5 = msg->field();
	EXPECT_EQ(nullptr, fld5);

	EXPECT_EQ(0, msg->count<VFLD1>());
	VFLD1 const* vfld1 = msg->field();
	EXPECT_EQ(nullptr, vfld1);

	//with 1 optional
	uint8_t const encoded2[] = { 1
		, 37
		, 0x21, 0x35, 0xD9
		, 3, 0xDA, 0xBE, 0xEF
		, 0x42, 4, 0xFE, 0xE1, 0xAB, 0xBA
		, 0x51, 0x01, 0x02, 0x03, 0x04
	};
	ctx.reset(encoded2, sizeof(encoded2));
#if (MED_EXCEPTIONS)
	decode(med::octet_decoder{ctx}, proto);
#else
	ASSERT_TRUE(decode(med::octet_decoder{ctx}, proto)) << toString(ctx.error_ctx());
#endif

	msg = proto.select();
	ASSERT_NE(nullptr, msg);
	EXPECT_EQ(37, msg->get<FLD_UC>().get());
	EXPECT_EQ(0x35D9, msg->get<FLD_U16>().get());
	EXPECT_EQ(0xDABEEF, msg->get<FLD_U24>().get());
	EXPECT_EQ(0xfee1ABBA, msg->get<FLD_IP>().get());

	fld5 = msg->field();
	EXPECT_EQ(1, msg->count<FLD_DW>());
	ASSERT_NE(nullptr, fld5);
	EXPECT_EQ(0x01020304, fld5->get());

	//with 2 optionals
	uint8_t const encoded3[] = { 1
		, 37
		, 0x21, 0x35, 0xD9
		, 3, 0xDA, 0xBE, 0xEF
		, 0x42, 4, 0xFE, 0xE1, 0xAB, 0xBA
		, 0x51, 0x01, 0x02, 0x03, 0x04
		, 0x12, 4, 't', 'e', 's', 't', '.', 't', 'h', 'i', 's', '!'
	};
	ctx.reset(encoded3, sizeof(encoded3));
#if (MED_EXCEPTIONS)
	decode(med::octet_decoder{ctx}, proto);
#else
	ASSERT_TRUE(decode(med::octet_decoder{ctx}, proto)) << toString(ctx.error_ctx());
#endif

	msg = proto.select();
	ASSERT_NE(nullptr, msg);
	EXPECT_EQ(37, msg->get<FLD_UC>().get());
	EXPECT_EQ(0x35D9, msg->get<FLD_U16>().get());
	EXPECT_EQ(0xDABEEF, msg->get<FLD_U24>().get());
	EXPECT_EQ(0xfee1ABBA, msg->get<FLD_IP>().get());

	fld5 = msg->field();
	EXPECT_EQ(1, msg->count<FLD_DW>());
	ASSERT_NE(nullptr, fld5);
	EXPECT_EQ(0x01020304, fld5->get());

	vfld1 = msg->field();
	ASSERT_EQ(1, msg->count<VFLD1>());
	EQ_STRING_O(VFLD1, "test.this!");

	//check clear
	{
		auto& r = proto.ref<MSG_SEQ>();
		ASSERT_TRUE(r.is_set());
		r.clear();
		ASSERT_FALSE(r.is_set());
	}
}

TEST(decode, seq_fail)
{
	PROTO proto;

	//wrong choice tag
	uint8_t const wrong_choice[] = { 100, 37, 0x49 };
	med::decoder_context<> ctx{ wrong_choice };
#if (MED_EXCEPTIONS)
	EXPECT_THROW(decode(med::octet_decoder{ctx}, proto), med::unknown_tag);
#else
	ASSERT_FALSE(decode(med::octet_decoder{ctx}, proto));
	EXPECT_EQ(med::error::UNKNOWN_TAG, ctx.error_ctx().get_error()) << toString(ctx.error_ctx());
	EXPECT_NE(nullptr, toString(ctx.error_ctx()));
#endif

	//wrong tag of required field 2
	uint8_t const wrong_tag[] = { 2, 37, 0x49, 0xDA, 0xBE, 0xEF };
	ctx.reset(wrong_tag, sizeof(wrong_tag));
#if (MED_EXCEPTIONS)
	EXPECT_THROW(decode(med::octet_decoder{ctx}, proto), med::unknown_tag);
#else
	ASSERT_FALSE(decode(med::octet_decoder{ctx}, proto));
	EXPECT_EQ(med::error::UNKNOWN_TAG, ctx.error_ctx().get_error()) << toString(ctx.error_ctx());
	EXPECT_NE(nullptr, toString(ctx.error_ctx()));
#endif

	//missing required field
	uint8_t const missing[] = { 1, 37 };
	ctx.reset(missing, sizeof(missing));
#if (MED_EXCEPTIONS)
	EXPECT_THROW(decode(med::octet_decoder{ctx}, proto), med::overflow);
#else
	ASSERT_FALSE(decode(med::octet_decoder{ctx}, proto));
	EXPECT_EQ(med::error::OVERFLOW, ctx.error_ctx().get_error()) << toString(ctx.error_ctx());
	EXPECT_NE(nullptr, toString(ctx.error_ctx()));
#endif

	//incomplete field
	uint8_t const incomplete[] = { 1, 37, 0x21, 0x35 };
	ctx.reset(incomplete, sizeof(incomplete));
#if (MED_EXCEPTIONS)
	EXPECT_THROW(decode(med::octet_decoder{ctx}, proto), med::overflow);
#else
	ASSERT_FALSE(decode(med::octet_decoder{ctx}, proto));
	EXPECT_EQ(med::error::OVERFLOW, ctx.error_ctx().get_error()) << toString(ctx.error_ctx());
	EXPECT_NE(nullptr, toString(ctx.error_ctx()));
#endif

	//invalid length of fixed field
	uint8_t const bad_length[] = { 1
		, 37
		, 0x21, 0x35, 0xD9
		, 2, 0xDA, 0xBE, 0xEF
		, 0x42, 4, 0xFE, 0xE1, 0xAB, 0xBA
	};
	ctx.reset(bad_length, sizeof(bad_length));
#if (MED_EXCEPTIONS)
	EXPECT_THROW(decode(med::octet_decoder{ctx}, proto), med::overflow);
#else
	ASSERT_FALSE(decode(med::octet_decoder{ctx}, proto));
	EXPECT_EQ(med::error::OVERFLOW, ctx.error_ctx().get_error()) << toString(ctx.error_ctx());
	EXPECT_NE(nullptr, toString(ctx.error_ctx()));
#endif

	//invalid length of variable field (shorter)
	uint8_t const bad_var_len_lo[] = { 1
		, 37
		, 0x21, 0x35, 0xD9
		, 3, 0xDA, 0xBE, 0xEF
		, 0x42, 4, 0xFE, 0xE1, 0xAB, 0xBA
		, 0x12, 1, 't','e','s','t'
	};
	ctx.reset(bad_var_len_lo, sizeof(bad_var_len_lo));
#if (MED_EXCEPTIONS)
	EXPECT_THROW(decode(med::octet_decoder{ctx}, proto), med::invalid_value);
#else
	ASSERT_FALSE(decode(med::octet_decoder{ctx}, proto));
	EXPECT_EQ(med::error::INVALID_VALUE, ctx.error_ctx().get_error()) << toString(ctx.error_ctx());
	EXPECT_NE(nullptr, toString(ctx.error_ctx()));
#endif

	//invalid length of variable field (longer)
	uint8_t const bad_var_len_hi[] = { 1
		, 37
		, 0x21, 0x35, 0xD9
		, 3, 0xDA, 0xBE, 0xEF
		, 0x42, 4, 0xFE, 0xE1, 0xAB, 0xBA
		, 0x12, 5, 't', 'e', 's', 't', 'e', 's', 't', 'e', 's', 't', 'e'
	};
	ctx.reset(bad_var_len_hi, sizeof(bad_var_len_hi));
#if (MED_EXCEPTIONS)
	EXPECT_THROW(decode(med::octet_decoder{ctx}, proto), med::invalid_value);
#else
	ASSERT_FALSE(decode(med::octet_decoder{ctx}, proto));
	EXPECT_EQ(med::error::INVALID_VALUE, ctx.error_ctx().get_error()) << toString(ctx.error_ctx());
	EXPECT_NE(nullptr, toString(ctx.error_ctx()));
#endif
}

TEST(decode, mseq_ok)
{
	PROTO proto;
	med::decoder_context<> ctx;

	//mandatory only
	uint8_t const encoded1[] = { 0x11
		, 37
		, 38
		, 0x21, 0x35, 0xD9
		, 0x21, 0x35, 0xDA
		, 3, 0xDA, 0xBE, 0xEF
		, 3, 0x22, 0xBE, 0xEF
		, 0x42, 4, 0xFE, 0xE1, 0xAB, 0xBA
		, 0x51, 0x01, 0x02, 0x03, 0x04
		, 0,1, 2, 0x21, 0,3
	};
	ctx.reset(encoded1, sizeof(encoded1));
#if (MED_EXCEPTIONS)
	decode(med::octet_decoder{ctx}, proto);
#else
	ASSERT_TRUE(decode(med::octet_decoder{ctx}, proto)) << toString(ctx.error_ctx());
#endif

	MSG_MSEQ const* msg = proto.select();
	ASSERT_NE(nullptr, msg);
	check_seqof<FLD_UC>(*msg, {37, 38});
	check_seqof<FLD_U16>(*msg, {0x35D9, 0x35DA});
	check_seqof<FLD_U24>(*msg, {0xDABEEF, 0x22BEEF});
	check_seqof<FLD_IP>(*msg, {0xfee1ABBA});
	check_seqof<FLD_DW>(*msg, {0x01020304});
	ASSERT_EQ(1, msg->count<SEQOF_3<0>>());
	{
		auto it = msg->get<SEQOF_3<0>>().begin();
		EXPECT_EQ(2, it->get<FLD_U8>().get());
		EXPECT_EQ(3, it->get<FLD_U16>().get());
	}

	EXPECT_EQ(0, msg->count<VFLD1>());
	EXPECT_TRUE(msg->get<VFLD1>().empty());

	//with more mandatory and optionals
	uint8_t const encoded2[] = { 0x11
		, 37
		, 38
		, 0x21, 0x35, 0xD9
		, 0x21, 0x35, 0xDA
		, 3, 0xDA, 0xBE, 0xEF
		, 3, 0x22, 0xBE, 0xEF
		, 0x42, 4, 0xFE, 0xE1, 0xAB, 0xBA
		, 0x42, 4, 0xAB, 0xBA, 0xC0, 0x01
		, 0x51, 0x01, 0x02, 0x03, 0x04
		, 0x51, 0x12, 0x34, 0x56, 0x78
		, 0,2, 3, 0x21, 0,4,  5, 0x21, 0,6
		, 0x60, 2, 0, 33 //O< T<0x60>, L, FLD_CHO >
		, 0x61, 4 //O< T<0x61>, L, SEQOF_1 >
			, 0x12,0x23, 0x34,0x45 //M< FLD_W, med::max<3> >
		, 0x62, 7 //O< T<0x62>, L, SEQOF_2, med::max<2> >
			, 0x11,0x22 //M< FLD_W >,
			, 0x06, 3, 2, 0x33,0x44 //O< T<0x06>, L, FLD_CHO >
		, 0x62, 2 //O< T<0x62>, L, SEQOF_2, med::max<2> >
			, 0x33,0x44 //M< FLD_W >,
			//O< T<0x06>, L, FLD_CHO >

		, 9, 't', 'e', 's', 't', '.', 't', 'h', 'i', 's'
		, 7, 't', 'e', 's', 't', '.', 'i', 't'
	};
	ctx.reset(encoded2, sizeof(encoded2));
#if (MED_EXCEPTIONS)
	decode(med::octet_decoder{ctx}, proto);
#else
	ASSERT_TRUE(decode(med::octet_decoder{ctx}, proto)) << toString(ctx.error_ctx());
#endif

	msg = proto.select();
	ASSERT_NE(nullptr, msg);

	check_seqof<FLD_UC>(*msg, {37, 38});
	check_seqof<FLD_U16>(*msg, {0x35D9, 0x35DA});
	check_seqof<FLD_U24>(*msg, {0xDABEEF, 0x22BEEF});
	check_seqof<FLD_IP>(*msg, {0xfee1ABBA, 0xABBAC001});
	check_seqof<FLD_DW>(*msg, {0x01020304, 0x12345678});
	ASSERT_EQ(2, msg->count<SEQOF_3<0>>());
	{
		auto it = msg->get<SEQOF_3<0>>().begin();
		EXPECT_EQ(3, it->get<FLD_U8>().get());
		EXPECT_EQ(4, it->get<FLD_U16>().get());
		++it;
		EXPECT_EQ(5, it->get<FLD_U8>().get());
		EXPECT_EQ(6, it->get<FLD_U16>().get());
	}

	FLD_CHO const* pcho = msg->field();
	ASSERT_NE(nullptr, pcho);
	FLD_U8 const* pu8 = pcho->select();
	ASSERT_NE(nullptr, pu8);
	EXPECT_EQ(33, pu8->get());

	SEQOF_1 const* pso1 = msg->field();
	ASSERT_NE(nullptr, pso1);
	check_seqof<FLD_W>(*pso1, {0x1223, 0x3445});

	ASSERT_EQ(2, msg->count<SEQOF_2>());
	auto& rso2 = msg->get<SEQOF_2>();
	auto so2_it = rso2.begin();
	ASSERT_NE(rso2.end(), so2_it);
	EXPECT_EQ(0x1122, so2_it->get<FLD_W>().get());

	pcho = so2_it->field();
	ASSERT_NE(nullptr, pcho);
	FLD_U16 const* pu16 = pcho->select();
	ASSERT_NE(nullptr, pu16);
	EXPECT_EQ(0x3344, pu16->get());

	so2_it++;
	ASSERT_NE(rso2.end(), so2_it);
	EXPECT_EQ(0x3344, so2_it->get<FLD_W>().get());

	pcho = so2_it->field();
	EXPECT_EQ(nullptr, pcho);

	so2_it++;
	ASSERT_EQ(rso2.end(), so2_it);

	check_seqof<VFLD1>(*msg, {"test.this", "test.it"});
}

TEST(decode, mseq_fail)
{
	PROTO proto;
	med::decoder_context<> ctx;

	//wrong tag of required field
	{
		uint8_t const wrong_tag[] = { 0x11
			, 37
			, 38
			, 0x22, 0x35, 0xD9
			, 0x21, 0x35, 0xDA
			, 3, 0xDA, 0xBE, 0xEF
			, 3, 0x22, 0xBE, 0xEF
			, 0x42, 4, 0xFE, 0xE1, 0xAB, 0xBA
			, 0x51, 0x01, 0x02, 0x03, 0x04
			, 0,1, 2, 0x21, 0,3
		};
		ctx.reset(wrong_tag, sizeof(wrong_tag));
#if (MED_EXCEPTIONS)
		EXPECT_THROW(decode(med::octet_decoder{ctx}, proto), med::missing_ie);
#else
		ASSERT_FALSE(decode(med::octet_decoder{ctx}, proto));
		EXPECT_EQ(med::error::MISSING_IE, ctx.error_ctx().get_error()) << toString(ctx.error_ctx());
		EXPECT_NE(nullptr, toString(ctx.error_ctx()));
#endif
	}

	//missing required field
	{
		uint8_t const missing[] = { 0x11
			, 37
			, 38
			, 0x21, 0x35, 0xD9
			, 0x21, 0x35, 0xDA
			, 3, 0xDA, 0xBE, 0xEF
			, 3, 0x22, 0xBE, 0xEF
			, 0x42, 4, 0xFE, 0xE1, 0xAB, 0xBA
			, 0,1, 2, 0x21, 0,3
		};

		ctx.reset(missing, sizeof(missing));
#if (MED_EXCEPTIONS)
		EXPECT_THROW(decode(med::octet_decoder{ctx}, proto), med::missing_ie);
#else
		ASSERT_FALSE(decode(med::octet_decoder{ctx}, proto));
		EXPECT_EQ(med::error::MISSING_IE, ctx.error_ctx().get_error()) << toString(ctx.error_ctx());
		EXPECT_NE(nullptr, toString(ctx.error_ctx()));
#endif
	}
}

TEST(decode, mseq_fail_length)
{
	PROTO proto;
	med::decoder_context<> ctx;

	//incomplete field
	{
		uint8_t const incomplete[] = { 0x11, 37, 38, 0x21, 0x35 };
		ctx.reset(incomplete, sizeof(incomplete));
#if (MED_EXCEPTIONS)
		EXPECT_THROW(decode(med::octet_decoder{ctx}, proto), med::overflow);
#else
		ASSERT_FALSE(decode(med::octet_decoder{ctx}, proto));
		EXPECT_EQ(med::error::OVERFLOW, ctx.error_ctx().get_error()) << toString(ctx.error_ctx());
		EXPECT_NE(nullptr, toString(ctx.error_ctx()));
#endif
	}

	//invalid length of fixed field
	{
		uint8_t const bad_length[] = { 0x11
			, 37
			, 38
			, 0x21, 0x35, 0xD9
			, 0x21, 0x35, 0xDA
			, 2, 0xDA, 0xBE, 0xEF
			, 3, 0x22, 0xBE, 0xEF
			, 0x42, 4, 0xFE, 0xE1, 0xAB, 0xBA
			, 0,1, 2, 0x21, 0,3
		};
		ctx.reset(bad_length, sizeof(bad_length));
#if (MED_EXCEPTIONS)
		EXPECT_THROW(decode(med::octet_decoder{ctx}, proto), med::overflow);
#else
		ASSERT_FALSE(decode(med::octet_decoder{ctx}, proto));
		EXPECT_EQ(med::error::OVERFLOW, ctx.error_ctx().get_error()) << toString(ctx.error_ctx());
		EXPECT_NE(nullptr, toString(ctx.error_ctx()));
#endif
	}

	//invalid length of variable field (shorter)
	{
		uint8_t const bad_var_len_lo[] = { 0x11
			, 37
			, 38
			, 0x21, 0x35, 0xD9
			, 0x21, 0x35, 0xDA
			, 3, 0xDA, 0xBE, 0xEF
			, 3, 0x22, 0xBE, 0xEF
			, 0x42, 4, 0xFE, 0xE1, 0xAB, 0xBA
			, 0x42, 4, 0xAB, 0xBA, 0xC0, 0x01
			, 0x51, 0x01, 0x02, 0x03, 0x04
			, 0x51, 0x12, 0x34, 0x56, 0x78
			, 0,1, 2, 0x21, 0,3
			, 3, 't', 'e', 's'
		};
		ctx.reset(bad_var_len_lo, sizeof(bad_var_len_lo));
#if (MED_EXCEPTIONS)
		EXPECT_THROW(decode(med::octet_decoder{ctx}, proto), med::invalid_value);
#else
		ASSERT_FALSE(decode(med::octet_decoder{ctx}, proto));
		EXPECT_EQ(med::error::INVALID_VALUE, ctx.error_ctx().get_error()) << toString(ctx.error_ctx());
		EXPECT_NE(nullptr, toString(ctx.error_ctx()));
#endif
	}

	//invalid length of variable field (longer)
	{
		uint8_t const bad_var_len_hi[] = { 0x11
			, 37
			, 38
			, 0x21, 0x35, 0xD9
			, 0x21, 0x35, 0xDA
			, 3, 0xDA, 0xBE, 0xEF
			, 3, 0x22, 0xBE, 0xEF
			, 0x42, 4, 0xFE, 0xE1, 0xAB, 0xBA
			, 0x42, 4, 0xAB, 0xBA, 0xC0, 0x01
			, 0x51, 0x01, 0x02, 0x03, 0x04
			, 0x51, 0x12, 0x34, 0x56, 0x78
			, 0,1, 2, 0x21, 0,3
			, 11, 't', 'e', 's', 't', 'e', 's', 't', 'e', 's', 't', 'e'
		};
		ctx.reset(bad_var_len_hi, sizeof(bad_var_len_hi));
#if (MED_EXCEPTIONS)
		EXPECT_THROW(decode(med::octet_decoder{ctx}, proto), med::overflow);
#else
		ASSERT_FALSE(decode(med::octet_decoder{ctx}, proto));
		EXPECT_EQ(med::error::OVERFLOW, ctx.error_ctx().get_error()) << toString(ctx.error_ctx());
		EXPECT_NE(nullptr, toString(ctx.error_ctx()));
#endif
	}
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
#if (MED_EXCEPTIONS)
	decode(med::octet_decoder{ctx}, proto);
#else
	ASSERT_TRUE(decode(med::octet_decoder{ctx}, proto)) << toString(ctx.error_ctx());
#endif
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
#if (MED_EXCEPTIONS)
	decode(med::octet_decoder{ctx}, proto);
#else
	ASSERT_TRUE(decode(med::octet_decoder{ctx}, proto)) << toString(ctx.error_ctx());
#endif
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
#if (MED_EXCEPTIONS)
	decode(med::octet_decoder{ctx}, proto);
#else
	ASSERT_TRUE(decode(med::octet_decoder{ctx}, proto)) << toString(ctx.error_ctx());
#endif
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
#if (MED_EXCEPTIONS)
	EXPECT_THROW(decode(med::octet_decoder{ctx}, proto), med::missing_ie);
#else
	ASSERT_FALSE(decode(med::octet_decoder{ctx}, proto));
	EXPECT_EQ(med::error::MISSING_IE, ctx.error_ctx().get_error()) << toString(ctx.error_ctx());
	EXPECT_NE(nullptr, toString(ctx.error_ctx()));
#endif

	//extra field in set
	uint8_t const extra_field[] = { 4
		, 0, 0x49, 3, 0xDA, 0xBE, 0xEF
		, 0, 0x49, 3, 0xDA, 0xBE, 0xEF
	};
	ctx.reset(extra_field, sizeof(extra_field));
#if (MED_EXCEPTIONS)
	EXPECT_THROW(decode(med::octet_decoder{ctx}, proto), med::extra_ie);
#else
	ASSERT_FALSE(decode(med::octet_decoder{ctx}, proto));
	EXPECT_EQ(med::error::EXTRA_IE, ctx.error_ctx().get_error()) << toString(ctx.error_ctx());
	EXPECT_NE(nullptr, toString(ctx.error_ctx()));
#endif
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
#if (MED_EXCEPTIONS)
	decode(med::octet_decoder{ctx}, proto);
#else
	ASSERT_TRUE(decode(med::octet_decoder{ctx}, proto)) << toString(ctx.error_ctx());
#endif
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
#if (MED_EXCEPTIONS)
	decode(med::octet_decoder{ctx}, proto);
#else
	ASSERT_TRUE(decode(med::octet_decoder{ctx}, proto)) << toString(ctx.error_ctx());
#endif
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
#if (MED_EXCEPTIONS)
	decode(med::octet_decoder{ctx}, proto);
#else
	ASSERT_TRUE(decode(med::octet_decoder{ctx}, proto)) << toString(ctx.error_ctx());
#endif
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
#if (MED_EXCEPTIONS)
	EXPECT_THROW(decode(med::octet_decoder{ctx}, proto), med::missing_ie);
#else
	ASSERT_FALSE(decode(med::octet_decoder{ctx}, proto));
	EXPECT_EQ(med::error::MISSING_IE, ctx.error_ctx().get_error()) << toString(ctx.error_ctx());
	EXPECT_NE(nullptr, toString(ctx.error_ctx()));
#endif

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
#if (MED_EXCEPTIONS)
	EXPECT_THROW(decode(med::octet_decoder{ctx}, proto), med::extra_ie);
#else
	ASSERT_FALSE(decode(med::octet_decoder{ctx}, proto));
	EXPECT_EQ(med::error::EXTRA_IE, ctx.error_ctx().get_error()) << toString(ctx.error_ctx());
	EXPECT_NE(nullptr, toString(ctx.error_ctx()));
#endif
}

TEST(decode, msg_func)
{
	PROTO proto;
	med::decoder_context<> ctx;

	//mandatory only
	uint8_t const encoded1[] = { 0xFF
		, 0x10
		, 37, 38
	};
	ctx.reset(encoded1, sizeof(encoded1));
#if (MED_EXCEPTIONS)
	decode(med::octet_decoder{ctx}, proto);
#else
	ASSERT_TRUE(decode(med::octet_decoder{ctx}, proto)) << toString(ctx.error_ctx());
#endif

	MSG_FUNC const* msg = proto.select();
	ASSERT_NE(nullptr, msg);
	ASSERT_EQ(2, msg->count<FLD_UC>());
	{
		auto it = msg->get<FLD_UC>().begin();
		EXPECT_EQ(37, it->get());
		EXPECT_EQ(38, std::next(it)->get());
		ASSERT_EQ(0, msg->count<FLD_U8>());
	}
	FLD_U16 const* pu16 = msg->field();
	EXPECT_EQ(nullptr, pu16);
	FLD_U24 const* pu24 = msg->field();
	EXPECT_EQ(nullptr, pu24);
	EXPECT_EQ(0, msg->count<FLD_IP>());

	//with 1 optional
	uint8_t const encoded2[] = { 0xFF
		, 0x21
		, 37, 38, 39
		, 0x35, 0xD9
	};
	ctx.reset(encoded2, sizeof(encoded2));
#if (MED_EXCEPTIONS)
	decode(med::octet_decoder{ctx}, proto);
#else
	ASSERT_TRUE(decode(med::octet_decoder{ctx}, proto)) << toString(ctx.error_ctx());
#endif

	msg = proto.select();
	ASSERT_NE(nullptr, msg);
	ASSERT_EQ(3, msg->count<FLD_UC>());
	{
		auto it = msg->get<FLD_UC>().begin();
		EXPECT_EQ(37, it->get()); ++it;
		EXPECT_EQ(38, it->get()); ++it;
		EXPECT_EQ(39, it->get());
	}
	ASSERT_EQ(0, msg->count<FLD_U8>());
	EXPECT_EQ(msg->get<FLD_U8>().end(), msg->get<FLD_U8>().begin());
	pu16 = msg->field();
	ASSERT_NE(nullptr, pu16);
	EXPECT_EQ(0x35D9, pu16->get());
	pu24 = msg->field();
	EXPECT_EQ(nullptr, pu24);
	EXPECT_EQ(0, msg->count<FLD_IP>());

	//with all optionals
	uint8_t const encoded3[] = { 0xFF
		, 0xB7
		, 37, 38, 39, 40
		, 3
		, 'a', 'b'
		, 0x35, 0xD9
		, 0xDA, 0xBE, 0xEF
		, 0, 0, 0x14, 0xDF
		, 0, 0, 0x15, 0x68
		, 0, 0, 0x15, 0xF1
	};
	ctx.reset(encoded3, sizeof(encoded3));
#if (MED_EXCEPTIONS)
	decode(med::octet_decoder{ctx}, proto);
#else
	ASSERT_TRUE(decode(med::octet_decoder{ctx}, proto)) << toString(ctx.error_ctx());
#endif

	msg = proto.select();
	ASSERT_NE(nullptr, msg);
	{
		uint8_t const exp[] = {37,38,39,40};
		ASSERT_EQ(std::size(exp), msg->count<FLD_UC>());
		auto* p = exp;
		for (auto& f : msg->get<FLD_UC>()) { EXPECT_EQ(*p++, f.get()); }
	}
	{
		uint8_t const exp[] = {'a','b'};
		ASSERT_EQ(std::size(exp), msg->count<FLD_U8>());
		auto* p = exp;
		for (auto& f : msg->get<FLD_U8>()) { EXPECT_EQ(*p++, f.get()); }
	}
	pu16 = msg->field();
	ASSERT_NE(nullptr, pu16);
	EXPECT_EQ(0x35D9, pu16->get());
	pu24 = msg->field();
	ASSERT_NE(nullptr, pu24);
	EXPECT_EQ(0xDABEEF, pu24->get());
	FLD_QTY const* pqty = msg->field();
	ASSERT_NE(nullptr, pqty);
	auto const u32_qty = msg->count<FLD_IP>();
	ASSERT_EQ(pqty->get(), u32_qty);
	std::size_t i = 0;
	for (auto& f : msg->get<FLD_IP>())
	{
		EXPECT_EQ(make_hash(i), f.get()); i++;
	}

	//------------ fail cases
	uint8_t const mandatory_underflow[] = { 0xFF
		, 0x07
		, 0x35, 0xD9
		, 0xDA, 0xBE, 0xEF
		, 0xFE, 0xE1, 0xAB, 0xBA
	};
	ctx.reset(mandatory_underflow, sizeof(mandatory_underflow));
#if (MED_EXCEPTIONS)
	EXPECT_THROW(decode(med::octet_decoder{ctx}, proto), med::missing_ie);
#else
	ASSERT_FALSE(decode(med::octet_decoder{ctx}, proto));
	EXPECT_EQ(med::error::MISSING_IE, ctx.error_ctx().get_error()) << toString(ctx.error_ctx());
	EXPECT_NE(nullptr, toString(ctx.error_ctx()));
#endif

	uint8_t const conditional_overflow[] = { 0xFF
		, 0xD7
		, 37, 38
		, 'a', 'b', 'c'
		, 0x35, 0xD9
		, 0xDA, 0xBE, 0xEF
		, 0xFE, 0xE1, 0xAB, 0xBA
	};
	ctx.reset(conditional_overflow, sizeof(conditional_overflow));
#if (MED_EXCEPTIONS)
	EXPECT_THROW(decode(med::octet_decoder{ctx}, proto), med::extra_ie);
#else
	ASSERT_FALSE(decode(med::octet_decoder{ctx}, proto));
	EXPECT_EQ(med::error::EXTRA_IE, ctx.error_ctx().get_error()) << toString(ctx.error_ctx());
	EXPECT_NE(nullptr, toString(ctx.error_ctx()));
#endif
}


//TODO: add more isolated UTs on fields
TEST(field, tagged_nibble)
{
	uint8_t buffer[4];
	med::encoder_context<> ctx{ buffer };

	FLD_TN field;
	field.set(5);
#if (MED_EXCEPTIONS)
	encode(med::octet_encoder{ctx}, field);
#else
	ASSERT_TRUE(encode(med::octet_encoder{ctx}, field)) << toString(ctx.error_ctx());
#endif
	EXPECT_STRCASEEQ("E5 ", as_string(ctx.buffer()));

	decltype(field) dfield;
	med::decoder_context<> dctx;
	dctx.reset(ctx.buffer().get_start(), ctx.buffer().get_offset());
#if (MED_EXCEPTIONS)
	decode(med::octet_decoder{dctx}, dfield);
#else
	ASSERT_TRUE(decode(med::octet_decoder{dctx}, dfield)) << toString(ctx.error_ctx());
#endif
	EXPECT_EQ(field.get(), dfield.get());
}

//placeholder::_length
struct PL_HDR : med::sequence<
	M< FLD_U16 >,
	med::placeholder::_length<6>, //don't count U16+U8 (2+1 bytes) and length encoded as U24 (3 bytes)
	M< FLD_U8 >
>
{
	static constexpr char const* name()         { return "PL-Header"; }
};

struct PL_SEQ : med::sequence<
	M< PL_HDR >,
	O< T<0x62>, FLD_DW, med::max<2> >
>
{
	using length_type = FLD_U24;
	static constexpr char const* name()         { return "PL-Sequence"; }
};

TEST(placeholder, length)
{
	uint8_t buffer[1024];
	med::encoder_context<> ctx{ buffer };
	med::decoder_context<> dctx;

	PL_SEQ msg;

	{
		PL_HDR& hdr = msg.ref<PL_HDR>();
		hdr.ref<FLD_U16>().set(0x1661);
		hdr.ref<FLD_U8>().set(0x37);

#if (MED_EXCEPTIONS)
		encode(med::octet_encoder{ctx}, msg);
#else
		ASSERT_TRUE(encode(med::octet_encoder{ctx}, msg)) << toString(ctx.error_ctx());
#endif
		uint8_t const encoded[] = {
			0x16, 0x61
			, 0, 0, 0    //length 3 bytes
			, 0x37
		};
		ASSERT_EQ(sizeof(encoded), ctx.buffer().get_offset());
		ASSERT_TRUE(Matches(encoded, buffer));

		dctx.reset(ctx.buffer().get_start(), ctx.buffer().get_offset());
		PL_SEQ dmsg;
#if (MED_EXCEPTIONS)
		decode(med::octet_decoder{dctx}, dmsg);
#else
		ASSERT_TRUE(decode(med::octet_decoder{dctx}, dmsg)) << toString(ctx.error_ctx());
#endif
		PL_HDR& dhdr = dmsg.ref<PL_HDR>();
		EXPECT_EQ(hdr.get<FLD_U16>().get(), dhdr.get<FLD_U16>().get());
		EXPECT_EQ(hdr.get<FLD_U8>().get(), dhdr.get<FLD_U8>().get());
	}

	ctx.reset();
	{
		static_assert(msg.arity<FLD_DW>() == 2, "");
		msg.push_back<FLD_DW>(ctx)->set(0x01020304);
		msg.push_back<FLD_DW>(ctx)->set(0x05060708);

#if (MED_EXCEPTIONS)
		encode(med::octet_encoder{ctx}, msg);
#else
		ASSERT_TRUE(encode(med::octet_encoder{ctx}, msg)) << toString(ctx.error_ctx());
#endif
		uint8_t const encoded[] = {
			0x16, 0x61
			, 0, 0, 10    //length 3 bytes
			, 0x37
			,0x62, 1,2,3,4
			,0x62, 5,6,7,8
		};
		ASSERT_EQ(sizeof(encoded), ctx.buffer().get_offset());
		ASSERT_TRUE(Matches(encoded, buffer));

		dctx.reset(ctx.buffer().get_start(), ctx.buffer().get_offset());
		PL_SEQ dmsg;
#if (MED_EXCEPTIONS)
		decode(med::octet_decoder{dctx}, dmsg);
#else
		ASSERT_TRUE(decode(med::octet_decoder{dctx}, dmsg)) << toString(dctx.error_ctx());
#endif
		ASSERT_EQ(msg.count<FLD_DW>(), dmsg.count<FLD_DW>());
		auto it = dmsg.get<FLD_DW>().begin();
		for (auto const& v : msg.get<FLD_DW>())
		{
			EXPECT_EQ(v.get(), it->get());
			it++;
		}
	}
}
#endif

#if 1
//support of 'unlimited' sequence-ofs
//NOTE: declare a message which can't be decoded
struct MSEQ_OPEN : med::sequence<
	M< T<0x21>, FLD_U16, med::min<2>, med::inf >,  //<TV>*[2,*)
	M< T<0x42>, L, FLD_IP, med::inf >,             //<TLV>(fixed)*[1,*)
	M< CNT, FLD_W, med::inf >,                     //C<V>*[1,*)
	O< T<0x62>, L, FLD_DW, med::inf >,             //[TLV]*[0,*)
	O< L, VFLD1, med::inf >                        //[LV(var)]*[0,*) till EoF
>
{
};

//encoding-only and only some fields since different IEs are checked above
TEST(encode, mseq_open)
{
	uint8_t buffer[1024];
	med::encoder_context<> ctx{ buffer };

	MSEQ_OPEN msg;
	static_assert(msg.arity<FLD_U16>() == med::inf(), "");
	static_assert(msg.arity<FLD_IP>() == med::inf(), "");
	static_assert(msg.arity<FLD_W>() == med::inf(), "");

	{
		auto* p = msg.push_back<FLD_U16>();
		ASSERT_NE(nullptr, p);
		p->set(0x35D9);
		p = msg.push_back<FLD_U16>(ctx);
		ASSERT_NE(nullptr, p);
		p->set(0x35DA);
	}
	msg.push_back<FLD_IP>(ctx)->set(0xFee1ABBA);
	msg.push_back<FLD_W>(ctx)->set(0x7654);
	msg.push_back<FLD_W>(ctx)->set(0x9876);
	msg.push_back<FLD_DW>(ctx)->set(0x01020304);
	msg.push_back<VFLD1>(ctx)->set("test.this");
	msg.push_back<VFLD1>(ctx)->set("test.it");

//	O< T<0x80>, CNT, SEQOF_3<1>, med::inf >,      //T[CV]*[0,*)

#if (MED_EXCEPTIONS)
	encode(med::octet_encoder{ctx}, msg);
#else
	ASSERT_TRUE(encode(med::octet_encoder{ctx}, msg)) << toString(ctx.error_ctx());
#endif
	uint8_t const encoded[] = {
		  0x21, 0x35, 0xD9                //<T=0x21, FLD_U16>
		, 0x21, 0x35, 0xDA
		, 0x42, 4, 0xFE, 0xE1, 0xAB, 0xBA //<T=0x42, L, FLD_IP>
		, 0,2, 0x76, 0x54,  0x98, 0x76    //<C16, FLD_W>
		, 0x62, 4, 0x01, 0x02, 0x03, 0x04    //[T=0x51, L, FLD_DW]
		, 9, 't', 'e', 's', 't', '.', 't', 'h', 'i', 's'
		, 7, 't', 'e', 's', 't', '.', 'i', 't'
	};
	EXPECT_EQ(sizeof(encoded), ctx.buffer().get_offset());
	EXPECT_TRUE(Matches(encoded, buffer));
}

TEST(encode, alloc_fail)
{
	uint8_t buffer[1];
	med::encoder_context<> ctx{ buffer };

	MSEQ_OPEN msg;

	//check the inplace storage for unlimited field has only one slot
	auto* p = msg.push_back<FLD_IP>();
	ASSERT_NE(nullptr, p);
	p->set(1);

#if (MED_EXCEPTIONS)
	ASSERT_THROW(msg.push_back<FLD_IP>(), med::out_of_memory);
	ASSERT_THROW(msg.push_back<FLD_IP>(ctx), med::out_of_memory);
#else
	p = msg.push_back<FLD_IP>();
	ASSERT_EQ(nullptr, p);
	p = msg.push_back<FLD_IP>(ctx);
	ASSERT_EQ(nullptr, p);
#endif
}

TEST(decode, alloc_fail)
{
	uint8_t const encoded[] = {
		  0x21, 0x35, 0xD9                //<T=0x21, FLD_U16>
		, 0x21, 0x35, 0xDA
		, 0x42, 4, 0xFE, 0xE1, 0xAB, 0xBA //<T=0x42, L, FLD_IP>
		, 0,2, 0x76, 0x54,  0x98, 0x76    //<C16, FLD_W>
		, 0x62, 4, 0x01, 0x02, 0x03, 0x04 //[T=0x51, L, FLD_DW]
		, 9, 't', 'e', 's', 't', '.', 't', 'h', 'i', 's'
		, 7, 't', 'e', 's', 't', '.', 'i', 't'
	};

	med::decoder_context<> ctx{ encoded };

	MSEQ_OPEN msg;

#if (MED_EXCEPTIONS)
	ASSERT_THROW(decode(med::octet_decoder{ctx}, msg), med::out_of_memory);
#else
	ASSERT_FALSE(decode(med::octet_decoder{ctx}, msg));
	EXPECT_EQ(med::error::OUT_OF_MEMORY, ctx.error_ctx().get_error()) <<
		[&]()
		{
//			dummy_sink d{2};
//			med::print(d, msg);
			if (auto* err = toString(ctx.error_ctx()))
			{
				return err;
			}
			else
			{
				return "NO ERRORS";
			}
		}();
	EXPECT_NE(nullptr, toString(ctx.error_ctx()));
#endif
}

#endif

#if 1
//setter with length calc
struct SHDR : med::value<uint8_t>
{
	template <class FLD>
	struct setter
	{
		template <class T>
		bool operator()(SHDR& shdr, T const& ies) const
		{
			if (auto const qty = med::field_count(ies.template as<FLD>()))
			{
				shdr.set(qty);
				return true;
			}

			return false;
		}
	};

};
struct SFLD : med::sequence<
	M<SHDR, SHDR::setter<FLD_U16>>,
	M<FLD_U16>
>{};
struct SMFLD : med::sequence<
	M<SHDR, SHDR::setter<FLD_U8>>,
	M<FLD_U8, med::max<7>>
>{};
struct SLEN : med::sequence<
	M<L, SFLD>,
	M<L, SMFLD>
>
{
};

TEST(encode, setter_with_length)
{
	SLEN msg;
	msg.ref<SFLD>().ref<FLD_U16>().set(0x55AA);
	for (std::size_t i = 0; i < 7; ++i)
	{
		msg.ref<SMFLD>().push_back<FLD_U8>()->set(i);
	}

	uint8_t buffer[1024];
	med::encoder_context<> ctx{ buffer };

#if (MED_EXCEPTIONS)
	encode(med::octet_encoder{ctx}, msg);
#else
	ASSERT_TRUE(encode(med::octet_encoder{ctx}, msg)) << toString(ctx.error_ctx());
#endif
	uint8_t const encoded[] = {
		3, 1, 0x55, 0xAA,
		8, 7, 0,1,2,3,4,5,6
	};
	EXPECT_EQ(sizeof(encoded), ctx.buffer().get_offset());
	EXPECT_TRUE(Matches(encoded, buffer));
}
#endif

#if 1
//update
struct UFLD : med::value<uint32_t>, med::with_snapshot
{
	static constexpr char const* name() { return "Updatalbe-Field"; }
};
struct UMSG : med::sequence<
	M<T<7>, L, UFLD>
>
{
};

TEST(update, fixed)
{
	UMSG msg;
	auto& ufld = msg.ref<UFLD>();
	ufld.set(0x12345678);

	uint8_t buffer[1024];
	med::encoder_context<> ctx{ buffer };
	auto encoder = make_octet_encoder(ctx);

#if (MED_EXCEPTIONS)
	encode(encoder, msg);
#else
	ASSERT_TRUE(encode(encoder, msg)) << toString(ctx.error_ctx());
#endif
	uint8_t const encoded[] = {7, 4, 0x12,0x34,0x56,0x78};
	EXPECT_EQ(sizeof(encoded), ctx.buffer().get_offset());
	EXPECT_TRUE(Matches(encoded, buffer));

	ufld.set(0x3456789A);
#if (MED_EXCEPTIONS)
	med::update(encoder, ufld);
#else
	ASSERT_TRUE(med::update(encoder, ufld)) << toString(ctx.error_ctx());
#endif
	uint8_t const updated[] = {7, 4, 0x34,0x56,0x78,0x9A};
	EXPECT_TRUE(Matches(updated, buffer));
}
#endif


int main(int argc, char **argv)
{
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
