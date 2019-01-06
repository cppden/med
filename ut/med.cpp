#include "ut.hpp"
#include "ut_proto.hpp"

#include "update.hpp"


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

//represents IE with byte value in LSBs with other bits reserved
template <uint8_t BITS>
struct LS_BITS : med::value<uint8_t>
{
	static constexpr value_type mask = (1u << BITS) - 1;
	value_type get_encoded() const      { return base_t::get_encoded() & mask; }
	void set(value_type v)              { base_t::set_encoded(v & mask); }
};

struct NO_THING : med::empty
{
	static constexpr char const* name() { return "Nothing"; }
};

struct FLD_INFO : med::choice< LS_BITS<3>
	, med::tag<med::value<med::fixed<0b000, uint8_t>>, NO_THING>
	, med::tag<med::value<med::fixed<0b001, uint8_t>>, FLD_DW>
>
{
	static constexpr char const* name() { return "Info"; }
};

TEST(field, empty)
{
	uint8_t buffer[16];
	med::encoder_context<> ctx{ buffer };

	FLD_INFO field;
	field.ref<NO_THING>();
	encode(med::octet_encoder{ctx}, field);
	EXPECT_STRCASEEQ("00 ", as_string(ctx.buffer()));

	decltype(field) dfield;
	med::decoder_context<> dctx;
	dctx.reset(ctx.buffer().get_start(), ctx.buffer().get_offset());
	decode(med::octet_decoder{dctx}, dfield);
	EXPECT_NE(nullptr, dfield.get<NO_THING>());

	ctx.reset();
	field.clear();
	field.ref<FLD_DW>().set(1);
	encode(med::octet_encoder{ctx}, field);
	EXPECT_STRCASEEQ("01 00 00 00 01 ", as_string(ctx.buffer()));
	dctx.reset(ctx.buffer().get_start(), ctx.buffer().get_offset());
	decode(med::octet_decoder{dctx}, dfield);
	EXPECT_EQ(1, dfield.get<FLD_DW>()->get());
}

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

	encode(med::octet_encoder{ctx}, msg);

	uint8_t const encoded[] = {
		3, 1, 0x55, 0xAA,
		8, 7, 0,1,2,3,4,5,6
	};
	EXPECT_EQ(sizeof(encoded), ctx.buffer().get_offset());
	EXPECT_TRUE(Matches(encoded, buffer));
}

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


int main(int argc, char **argv)
{
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
