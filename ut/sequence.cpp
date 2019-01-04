#include "ut.hpp"
#include "ut_proto.hpp"

TEST(ooo, seq) //out-of-order
{
	OOO_SEQ msg;
	msg.ref<FLD_U8>().set(1);
	msg.ref<FLD_U24>().set(3);

	uint8_t buffer[32];
	med::encoder_context<> ctx{ buffer };
	encode(med::octet_encoder{ctx}, msg);
	uint8_t const encoded1[] = {1,1, 3,0,0,3 };

	EXPECT_EQ(sizeof(encoded1), ctx.buffer().get_offset());
	EXPECT_TRUE(Matches(encoded1, buffer));


	msg.clear();
	med::decoder_context<> dctx;
	dctx.reset(encoded1, sizeof(encoded1));
	decode(med::octet_decoder{dctx}, msg);
	EXPECT_EQ(1, msg.get<FLD_U8>().get());
	EXPECT_EQ(3, msg.get<FLD_U24>().get());
}

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

	encode(med::octet_encoder{ctx}, proto);

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

	encode(med::octet_encoder{ctx}, proto);

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

	encode(med::octet_encoder{ctx}, proto);

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

TEST(encode, seq_fail_mandatory)
{
	PROTO proto;
	uint8_t buffer[1024];
	med::encoder_context<> ctx{ buffer };

	MSG_SEQ& msg = proto.select();
	msg.ref<FLD_UC>().set(0);
	msg.ref<FLD_U24>().set(0);
	//ctx.reset();

	EXPECT_THROW(encode(med::octet_encoder{ctx}, proto), med::missing_ie);
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

	encode(med::octet_encoder{ctx}, proto);

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

	encode(med::octet_encoder{ctx}, proto);

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
		EXPECT_THROW(encode(med::octet_encoder{ctx}, proto), med::missing_ie);
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
		EXPECT_THROW(encode(med::octet_encoder{ctx}, proto), med::missing_ie);
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
		EXPECT_THROW(encode(med::octet_encoder{ctx}, proto), med::missing_ie);
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
		EXPECT_THROW(encode(med::octet_encoder{ctx}, proto), med::missing_ie);
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
		EXPECT_THROW(encode(med::octet_encoder{ctx}, proto), med::overflow);
	}

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
	decode(med::octet_decoder{ctx}, proto);

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
	decode(med::octet_decoder{ctx}, proto);

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
	decode(med::octet_decoder{ctx}, proto);

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

struct OPT_FLAGS : med::value<uint8_t>
{
	template <value_type BITS>
	struct has_bits
	{
		template <class T> bool operator()(T const& ies) const
		{
			return ies.template as<OPT_FLAGS>().get() & BITS;
		}
	};
};

struct SEQ_OPT : med::sequence<
	M<OPT_FLAGS>,
	O<T<1>, FLD_DW>,
	O<FLD_U8, OPT_FLAGS::has_bits<1> >,
	O<FLD_U16> //can't have 1 as MSB due to tag above
>
{};

TEST(decode, seq_opt)
{
	SEQ_OPT msg;

	uint8_t const encoded1[] = {0, 1, 0x11, 0x22, 0x33, 0x44};
	med::decoder_context<> ctx;

	ctx.reset(encoded1, sizeof(encoded1));
	decode(med::octet_decoder{ctx}, msg);

	FLD_DW const* p1 = msg.cfield();
	ASSERT_NE(nullptr, p1);
	EXPECT_EQ(0x11223344, p1->get());
	FLD_U16 const* p2 = msg.cfield();
	EXPECT_EQ(nullptr, p2);

	msg.clear();
	uint8_t const encoded2[] = {0, 0x11, 0x22};
	ctx.reset(encoded2, sizeof(encoded2));
	decode(med::octet_decoder{ctx}, msg);

	p1 = msg.cfield();
	EXPECT_EQ(nullptr, p1);
	p2 = msg.cfield();
	ASSERT_NE(nullptr, p2);
	EXPECT_EQ(0x1122, p2->get());
}

TEST(decode, seq_fail)
{
	PROTO proto;

	//wrong choice tag
	uint8_t const wrong_choice[] = { 100, 37, 0x49 };
	med::decoder_context<> ctx{ wrong_choice };
	EXPECT_THROW(decode(med::octet_decoder{ctx}, proto), med::unknown_tag);

	//wrong tag of required field 2
	uint8_t const wrong_tag[] = { 2, 37, 0x49, 0xDA, 0xBE, 0xEF };
	ctx.reset(wrong_tag, sizeof(wrong_tag));
	EXPECT_THROW(decode(med::octet_decoder{ctx}, proto), med::unknown_tag);

	//missing required field
	uint8_t const missing[] = { 1, 37 };
	ctx.reset(missing, sizeof(missing));
	EXPECT_THROW(decode(med::octet_decoder{ctx}, proto), med::overflow);

	//incomplete field
	uint8_t const incomplete[] = { 1, 37, 0x21, 0x35 };
	ctx.reset(incomplete, sizeof(incomplete));
	EXPECT_THROW(decode(med::octet_decoder{ctx}, proto), med::overflow);

	//invalid length of fixed field
	uint8_t const bad_length[] = { 1
		, 37
		, 0x21, 0x35, 0xD9
		, 2, 0xDA, 0xBE, 0xEF
		, 0x42, 4, 0xFE, 0xE1, 0xAB, 0xBA
	};
	ctx.reset(bad_length, sizeof(bad_length));
	EXPECT_THROW(decode(med::octet_decoder{ctx}, proto), med::overflow);

	//invalid length of variable field (shorter)
	uint8_t const bad_var_len_lo[] = { 1
		, 37
		, 0x21, 0x35, 0xD9
		, 3, 0xDA, 0xBE, 0xEF
		, 0x42, 4, 0xFE, 0xE1, 0xAB, 0xBA
		, 0x12, 1, 't','e','s','t'
	};
	ctx.reset(bad_var_len_lo, sizeof(bad_var_len_lo));
	EXPECT_THROW(decode(med::octet_decoder{ctx}, proto), med::invalid_value);

	//invalid length of variable field (longer)
	uint8_t const bad_var_len_hi[] = { 1
		, 37
		, 0x21, 0x35, 0xD9
		, 3, 0xDA, 0xBE, 0xEF
		, 0x42, 4, 0xFE, 0xE1, 0xAB, 0xBA
		, 0x12, 5, 't', 'e', 's', 't', 'e', 's', 't', 'e', 's', 't', 'e'
	};
	ctx.reset(bad_var_len_hi, sizeof(bad_var_len_hi));
	EXPECT_THROW(decode(med::octet_decoder{ctx}, proto), med::invalid_value);
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
	decode(med::octet_decoder{ctx}, proto);

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
	decode(med::octet_decoder{ctx}, proto);

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
		EXPECT_THROW(decode(med::octet_decoder{ctx}, proto), med::missing_ie);
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
		EXPECT_THROW(decode(med::octet_decoder{ctx}, proto), med::missing_ie);
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
		EXPECT_THROW(decode(med::octet_decoder{ctx}, proto), med::overflow);
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
		EXPECT_THROW(decode(med::octet_decoder{ctx}, proto), med::overflow);
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
		EXPECT_THROW(decode(med::octet_decoder{ctx}, proto), med::invalid_value);
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
		EXPECT_THROW(decode(med::octet_decoder{ctx}, proto), med::overflow);
	}
}


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

	encode(med::octet_encoder{ctx}, msg);

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

	ASSERT_THROW(msg.push_back<FLD_IP>(), med::out_of_memory);
	ASSERT_THROW(msg.push_back<FLD_IP>(ctx), med::out_of_memory);
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

	ASSERT_THROW(decode(med::octet_decoder{ctx}, msg), med::out_of_memory);
}

#endif

