#include "traits.hpp"
#include "compound.hpp"
#include "ut.hpp"

namespace cho {

//low nibble selector
//struct LT : med::peek<med::value<uint8_t>>
//{
//	void set_encoded(value_type v)            { base_t::set_encoded(v & 0xF); }
//};
template <std::size_t TAG>
struct LT : med::peek<med::value<med::fixed<TAG, uint8_t>>>
{
	static_assert(0 == (TAG & 0xF0), "LOW NIBBLE TAG EXPECTED");
	static constexpr bool match(uint8_t v)    { return TAG == (v & 0xF); }
};

//NOTE: low nibble of 1st octet is a tag
template <uint8_t TAG>
struct BCD : med::octet_string<med::octets_var_intern<3>, med::min<1>>
		, med::add_meta_info< med::mi<med::mik::TAG, LT<TAG>> >
{
	bool set(std::size_t len, void const* data)
	{
		//need additional nibble for the tag
		std::size_t const num_octets = (len + 1);// / 2;
		if (num_octets >= traits::min_octets && num_octets <= traits::max_octets)
		{
			m_value.resize(num_octets);
			uint8_t* p = m_value.data();
			uint8_t const* in = static_cast<uint8_t const*>(data);

			*p++ = (*in & 0xF0) | TAG;
			uint8_t o = (*in++ << 4);
			for (; len > 1; --len)
			{
				*p++ = o | (*in >> 4);
				o = *in++ << 4;
			}
			*p++ = o | 0xF;
			return true;
		}
		return false;
	}
};

struct BCD_1 : BCD<1>
{
	static constexpr char const* name() { return "BCD-1"; }
};
struct BCD_2 : BCD<2>
{
	static constexpr char const* name() { return "BCD-2"; }
};

//nibble selected choice field
struct FLD_NSCHO : med::choice<
	M< BCD_1 >,
	M< BCD_2 >
>
{
};


//choice based on plain value selector
struct PLAIN : med::choice<
	M<C<0x00>, cmp::U8>,
	M<C<0x02>, cmp::U16>,
	M<C<0x04>, cmp::U32>
>
{};


} //end: namespace cho

TEST(choice, plain)
{
	uint8_t buffer[32];
	med::encoder_context<> ctx{ buffer };
	med::octet_encoder encoder{ctx};

	using namespace cho;
	using namespace cmp;

	PLAIN msg;
	EXPECT_EQ(0, msg.calc_length(encoder));
	msg.ref<cmp::U8>().set(0);
	EXPECT_EQ(2, msg.calc_length(encoder));
	msg.ref<cmp::U16>().set(0);
	EXPECT_EQ(3, msg.calc_length(encoder));

	msg.ref<U16>().set(0x1234);
	encode(encoder, msg);
	EXPECT_STRCASEEQ("02 12 34 ", as_string(ctx.buffer()));

	decltype(msg) dmsg;
	med::decoder_context<> dctx;
	dctx.reset(ctx.buffer().get_start(), ctx.buffer().get_offset());
	decode(med::octet_decoder{dctx}, dmsg);
	ASSERT_NE(nullptr, dmsg.get<U16>());
	EXPECT_EQ(msg.get<U16>()->get(), dmsg.get<U16>()->get());
}

TEST(choice, peek)
{
	uint8_t buffer[32];
	med::encoder_context<> ctx{ buffer };
	med::octet_encoder encoder{ctx};

	using namespace cho;
	using namespace cmp;

	uint8_t const bcd[] = {0x34, 0x56};
	FLD_NSCHO msg;
	msg.ref<BCD_1>().set(2, bcd);
	encode(encoder, msg);
	EXPECT_STRCASEEQ("31 45 6F ", as_string(ctx.buffer()));

	decltype(msg) dmsg;
	med::decoder_context<> dctx;
	dctx.reset(ctx.buffer().get_start(), ctx.buffer().get_offset());
	decode(med::octet_decoder{dctx}, dmsg);
	ASSERT_NE(nullptr, dmsg.get<BCD_1>());
	EXPECT_EQ(msg.get<BCD_1>()->size(), dmsg.get<BCD_1>()->size());
}

TEST(choice, compound)
{
	using namespace std::string_view_literals;

	uint8_t buffer[128];
	med::encoder_context<> ctx{ buffer };

	cmp::CHOICE msg;
	msg.ref<cmp::string>().set("12345678"sv);

	encode(med::octet_encoder{ctx}, msg);
	EXPECT_STRCASEEQ(
		"00 10 00 01 00 0C 00 01 31 32 33 34 35 36 37 38 ",
		as_string(ctx.buffer())
	);

	decltype(msg) dmsg;
	med::decoder_context<> dctx;
	dctx.reset(ctx.buffer().get_start(), ctx.buffer().get_offset());
	decode(med::octet_decoder{dctx}, dmsg);

	ASSERT_NE(nullptr, dmsg.get<cmp::string>());
	EXPECT_EQ(msg.get<cmp::string>()->get(), dmsg.get<cmp::string>()->get());
}

