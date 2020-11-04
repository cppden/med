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
	M< T<1>, U8, med::max<3>>,
	O< T<2>, U16, med::inf>
>{};

} //end: namespace multi

TEST(multi, pop_back)
{
	uint8_t buffer[16];
	med::encoder_context<> ctx{ buffer };

	using namespace multi;
	M1 msg;
	auto const& mie = msg.get<U8>();
	msg.push_back<U8>()->set(1);
	msg.pop_back<U8>();
	EXPECT_TRUE(mie.empty());
	EXPECT_EQ(0, mie.count());

	msg.push_back<U8>()->set(1);
	msg.push_back<U8>()->set(2);
	msg.push_back<U8>()->set(3);
	EXPECT_FALSE(mie.empty());
	EXPECT_EQ(3, mie.count());
	EXPECT_EQ(3, std::distance(mie.begin(), mie.end()));

	msg.pop_back<U8>();

	encode(med::octet_encoder{ctx}, msg);

	uint8_t const encoded[] = {
		1, 1,
		1, 2,
	};
	EXPECT_EQ(sizeof(encoded), ctx.buffer().get_offset());
	EXPECT_TRUE(Matches(encoded, buffer));
}

TEST(multi, push_back)
{
	uint8_t buffer[1];
	med::encoder_context<> ctx{ buffer };

	using namespace multi;
	M1 msg;

	//check the inplace storage for unlimited field has only one slot
	auto* p = msg.push_back<U16>();
	ASSERT_NE(nullptr, p);
	p->set(1);

	ASSERT_THROW(msg.push_back<U16>(), med::out_of_memory);
	ASSERT_THROW(msg.push_back<U16>(ctx), med::out_of_memory);
}

TEST(multi, clear)
{
	using namespace multi;
	M1 msg;

	auto const& mie = msg.get<U8>();
	EXPECT_TRUE(mie.empty());
	msg.push_back<U8>()->set(1);
	msg.push_back<U8>()->set(2);
	msg.push_back<U8>()->set(3);
	EXPECT_FALSE(mie.empty());
	EXPECT_EQ(3, mie.count());
	EXPECT_EQ(3, std::distance(mie.begin(), mie.end()));

	msg.clear<U8>();
	msg.push_back<U8>()->set(1);
	msg.push_back<U8>()->set(2);
	msg.push_back<U8>()->set(3);
	EXPECT_FALSE(mie.empty());
	EXPECT_EQ(3, mie.count());
	EXPECT_EQ(3, std::distance(mie.begin(), mie.end()));
}

