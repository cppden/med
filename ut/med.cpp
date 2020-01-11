#include "ut.hpp"
#include "ut_proto.hpp"

#include "update.hpp"


TEST(name, tag)
{
	PROTO proto;
	med::encoder_context<> ctx{nullptr, 0};
	med::octet_encoder enc{ctx};

	EXPECT_NE(nullptr, PROTO::name_tag(1, enc));
	auto atag = 1;
	EXPECT_STREQ(med::name<MSG_SEQ>(), PROTO::name_tag(atag, enc));
	atag = 0x55;
	EXPECT_EQ(nullptr, PROTO::name_tag(atag, enc));

	EXPECT_NE(nullptr, MSG_SET::name_tag(0x0b, enc));

	atag = 0x21;
	EXPECT_STREQ(med::name<FLD_U16>(), MSG_SET::name_tag(atag, enc));
	atag = 0x55;
	EXPECT_EQ(nullptr, MSG_SET::name_tag(atag, enc));
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

	encode(med::octet_encoder{ctx}, proto);

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

	encode(med::octet_encoder{ctx}, proto);

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

	encode(med::octet_encoder{ctx}, proto);

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

#if 1
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
	decode(med::octet_decoder{ctx}, proto);

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
	decode(med::octet_decoder{ctx}, proto);

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
	decode(med::octet_decoder{ctx}, proto);

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
	EXPECT_THROW(decode(med::octet_decoder{ctx}, proto), med::missing_ie);

	uint8_t const conditional_overflow[] = { 0xFF
		, 0xD7
		, 37, 38
		, 'a', 'b', 'c'
		, 0x35, 0xD9
		, 0xDA, 0xBE, 0xEF
		, 0xFE, 0xE1, 0xAB, 0xBA
	};
	ctx.reset(conditional_overflow, sizeof(conditional_overflow));
	EXPECT_THROW(decode(med::octet_decoder{ctx}, proto), med::extra_ie);
}
#endif

#if 1
TEST(field, tagged_nibble)
{
	uint8_t buffer[4];
	med::encoder_context<> ctx{ buffer };

	FLD_TN field;
	field.set(5);
	encode(med::octet_encoder{ctx}, field);
	EXPECT_STRCASEEQ("E5 ", as_string(ctx.buffer()));

	decltype(field) dfield;
	med::decoder_context<> dctx;
	dctx.reset(ctx.buffer().get_start(), ctx.buffer().get_offset());
	decode(med::octet_decoder{dctx}, dfield);
	EXPECT_EQ(field.get(), dfield.get());
}

TEST(field, empty)
{
	uint8_t buffer[16];
	med::encoder_context<> ctx{ buffer };

	FLD_CHO field;
	field.ref<NO_THING>();
	encode(med::octet_encoder{ctx}, field);
	EXPECT_STRCASEEQ("06 ", as_string(ctx.buffer()));

	decltype(field) dfield;
	med::decoder_context<> dctx;
	dctx.reset(ctx.buffer().get_start(), ctx.buffer().get_offset());
	decode(med::octet_decoder{dctx}, dfield);
	EXPECT_NE(nullptr, dfield.get<NO_THING>());
}
#endif

#if 1
namespace init {

struct MSG1 : med::sequence<
	M< med::value<med::init<0,uint8_t>> >, //spare
	M< FLD_UC >
>{};

struct MSG2 : med::sequence<
	M< FLD_UC >
>{};

struct PROTO : med::sequence<
	O< T<1>, L, MSG1>,
	O< T<2>, L, MSG2>
>{};

} //end: namespace init

TEST(field, init)
{
	using namespace init;

	auto dec = [](auto& ctx)
	{
		init::PROTO msg;
		med::decoder_context<> dctx;
		dctx.reset(ctx.buffer().get_start(), ctx.buffer().get_offset());
		decode(med::octet_decoder{dctx}, msg);
		FLD_UC const* v = nullptr;
		if (auto* p = msg.get<MSG1>()) { v = &p->get<FLD_UC>(); }
		else if (auto* p = msg.get<MSG2>()) { v = &p->get<FLD_UC>(); }
		ASSERT_NE(nullptr, v);
		EXPECT_EQ(7, v->get());
	};

	uint8_t buffer[16];
	med::encoder_context<> ctx{ buffer };

	init::PROTO msg;
	msg.ref<MSG1>().ref<FLD_UC>().set(7);
	encode(med::octet_encoder{ctx}, msg);
	EXPECT_STRCASEEQ("01 02 00 07 ", as_string(ctx.buffer()));
	dec(ctx);

	msg.clear(); ctx.reset();
	msg.ref<MSG2>().ref<FLD_UC>().set(7);
	encode(med::octet_encoder{ctx}, msg);
	EXPECT_STRCASEEQ("02 01 07 ", as_string(ctx.buffer()));
	dec(ctx);
}
#endif

#if 1
//update
struct UFLD : med::value<uint32_t>, med::with_snapshot
{
	static constexpr auto name() { return "Updatable-Field"; }
};
struct UMSG : med::sequence< M<T<7>, L, UFLD> > {};

TEST(update, fixed)
{
	UMSG msg;
	auto& ufld = msg.ref<UFLD>();
	ufld.set(0x12345678);

	uint8_t buffer[1024];
	med::encoder_context<> ctx{ buffer };
	med::octet_encoder encoder{ctx};

	encode(encoder, msg);

	uint8_t const encoded[] = {7, 4, 0x12,0x34,0x56,0x78};
	EXPECT_EQ(sizeof(encoded), ctx.buffer().get_offset());
	EXPECT_TRUE(Matches(encoded, buffer));

	ufld.set(0x3456789A);
	med::update(encoder, ufld);

	uint8_t const updated[] = {7, 4, 0x34,0x56,0x78,0x9A};
	EXPECT_TRUE(Matches(updated, buffer));
}
#endif

int main(int argc, char **argv)
{
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
