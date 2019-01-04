#include "compound.hpp"

namespace cho {

//choice based on plain value selector
struct PLAIN : med::choice< med::value<uint8_t>
	, med::tag<C<0x00>, cmp::U8>
	, med::tag<C<0x02>, cmp::U16>
	, med::tag<C<0x04>, cmp::U32>
>
{};

//choice based on compound selector
struct CMP : med::choice< cmp::hdr<>
	, cmp::string
	, cmp::number
>
{};

} //end: namespace cho

TEST(choice, plain)
{
	uint8_t buffer[32];
	med::encoder_context<> ctx{ buffer };

	using namespace cho;
	using namespace cmp;
	PLAIN msg;
	msg.ref<U16>().set(0x1234);
	encode(med::octet_encoder{ctx}, msg);
	EXPECT_STRCASEEQ("02 12 34 ", as_string(ctx.buffer()));

	decltype(msg) dmsg;
	med::decoder_context<> dctx;
	dctx.reset(ctx.buffer().get_start(), ctx.buffer().get_offset());
	decode(med::octet_decoder{dctx}, dmsg);
	ASSERT_NE(nullptr, dmsg.get<U16>());
	EXPECT_EQ(msg.get<U16>()->get(), dmsg.get<U16>()->get());
}

#if 0
TEST(choice, compound)
{
	using namespace std::string_view_literals;

	uint8_t buffer[128];
	med::encoder_context<> ctx{ buffer };

	cho::CMP msg;
	msg.ref<cmp::string>().set("12345678"sv);

	encode(med::octet_encoder{ctx}, msg);
	EXPECT_STRCASEEQ(
		"00 0C 00 01 31 32 33 34 35 36 37 38 ",
//		"00 08 00 02 12 34 56 78 ",
		as_string(ctx.buffer())
	);

	decltype(msg) dmsg;
	med::decoder_context<> dctx;
	dctx.reset(ctx.buffer().get_start(), ctx.buffer().get_offset());
	decode(med::octet_decoder{dctx}, dmsg);

	ASSERT_NE(nullptr, dmsg.get<cmp::string>());
	EXPECT_EQ(msg.get<cmp::string>()->get(), dmsg.get<cmp::string>()->get());
}
#endif
