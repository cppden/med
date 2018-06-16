#include "ut.hpp"

namespace multi {

struct L : med::length_t<med::value<uint8_t>>{
	static constexpr char const* name() { return "LEN"; }
};

template <std::size_t TAG>
using T = med::value<med::fixed<TAG, uint8_t>>;

struct U8  : med::value<uint8_t> {};
struct U16 : med::value<uint16_t> {};
struct U24 : med::value<med::bytes<3>> {};
struct U32 : med::value<uint32_t> {};

struct M1 : med::sequence<
	M< T<1>, U8, med::max<3>>
>{};

} //end: namespace multi

TEST(multi, pop_back)
{
	uint8_t buffer[16];
	med::encoder_context<> ctx{ buffer };

	multi::M1 msg;
	auto const& mie = msg.get<multi::U8>();
	msg.push_back<multi::U8>()->set(1);
	EXPECT_EQ(1, mie.count());
	EXPECT_EQ(1, std::distance(mie.begin(), mie.end()));
	msg.pop_back<multi::U8>();
	EXPECT_EQ(0, mie.count());
	EXPECT_EQ(0, std::distance(mie.begin(), mie.end()));
	msg.push_back<multi::U8>()->set(1);
	msg.push_back<multi::U8>()->set(2);
	EXPECT_EQ(2, mie.count());
	EXPECT_EQ(2, std::distance(mie.begin(), mie.end()));
	msg.pop_back<multi::U8>();
	EXPECT_EQ(1, mie.count());
	EXPECT_EQ(1, std::distance(mie.begin(), mie.end()));
	msg.push_back<multi::U8>()->set(2);
	msg.push_back<multi::U8>()->set(3);
	EXPECT_EQ(3, mie.count());
	EXPECT_EQ(3, std::distance(mie.begin(), mie.end()));
	msg.pop_back<multi::U8>();

#if (MED_EXCEPTIONS)
	encode(med::octet_encoder{ctx}, msg);
#else
	ASSERT_TRUE(encode(med::octet_encoder{ctx}, msg)) << toString(ctx.error_ctx());
#endif //MED_EXCEPTIONS

	uint8_t const encoded[] = {
		1, 1,
		1, 2,
	};
	EXPECT_EQ(sizeof(encoded), ctx.buffer().get_offset());
	EXPECT_TRUE(Matches(encoded, buffer));
}

