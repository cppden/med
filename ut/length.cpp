#include "encode.hpp"
#include "decode.hpp"
#include "encoder_context.hpp"
#include "decoder_context.hpp"
#include "octet_encoder.hpp"
#include "octet_decoder.hpp"

#include "ut.hpp"

namespace len {

struct L : med::length_t<med::value<uint8_t>>{};

template <std::size_t TAG>
using T = med::value<med::fixed<TAG, uint8_t>>;

struct U8  : med::value<uint8_t> {};
struct U16 : med::value<uint16_t> {};
struct U24 : med::value<med::bytes<3>> {};
struct U32 : med::value<uint32_t> {};

struct CHOICE : med::choice< med::value<uint8_t>
	, med::tag<T<1>, U8>
	, med::tag<T<2>, U16>
	, med::tag<T<4>, U32>
>{};

struct SEQ2 : med::sequence<
	M< U16 >,
	O< T<1>, L, CHOICE >
>{};

struct VAR : med::ascii_string<med::min<3>, med::max<10>> {};

struct MSG : med::sequence<
	M< T<1>, L, U8, med::max<2>>,
	O< T<2>, U16>,
	O< T<3>, U24>,
	O< T<4>, L, U32, med::max<2>>
>{};

struct SEQ : med::sequence<
	M< L, MSG >
>{};

struct SET : med::set< med::value<uint8_t>,
	M< T<1>, L, MSG >
>{};

//mandatory LMV (length multi-value)
struct MV : med::sequence<
	M<U8, med::pmax<3>>
>{};
struct M_LMV : med::sequence<
	M<L, MV>
>{};

//conditional LV
struct true_cond
{
	template <class IES>
	bool operator()(IES const&) const   { return true; }
};

struct C_LV : med::sequence<
	O<L, VAR, true_cond>
>{};

} //namespace len

TEST(length, m_lmv)
{
	uint8_t const encoded[] = {
		3, //len
		1, 2, 3
	};

	std::size_t alloc[32];
	med::decoder_context<> ctx{encoded};
	len::M_LMV msg;

#if (MED_EXCEPTIONS)
	EXPECT_THROW(decode(med::octet_decoder{ctx}, msg), med::out_of_memory);
#else
	ASSERT_FALSE(decode(med::octet_decoder{ctx}, msg));
	EXPECT_EQ(med::error::OUT_OF_MEMORY, ctx.error_ctx().get_error()) << toString(ctx.error_ctx());
#endif

	ctx.get_allocator().reset(alloc); //assign allocation space
	ctx.reset(); //reset from previous errors
	msg.clear(); //clear message from previous decode

#if (MED_EXCEPTIONS)
	decode(med::octet_decoder{ctx}, msg);
#else
	ASSERT_TRUE(decode(med::octet_decoder{ctx}, msg)) << toString(ctx.error_ctx());
#endif

	auto const& vals = msg.get<len::MV>().get<len::U8>();
	uint8_t const exp[] = {1,2,3};
	ASSERT_EQ(std::size(exp), vals.count());
	auto* p = exp;
	for (auto const& u : vals)
	{
		EXPECT_EQ(*p++, u.get());
	}

	uint8_t const shorter[] = {
		4, //len
		1, 2, 3,
		0,0,0//padding to avoid compiler warnings
	};

	//ctx.get_allocator().reset(alloc); //assign allocation space
	ctx.reset(shorter, 4); //reset to new data
	msg.clear(); //clear message from previous decode

#if (MED_EXCEPTIONS)
	EXPECT_THROW(decode(med::octet_decoder{ctx}, msg), med::overflow);
#else
	ASSERT_FALSE(decode(med::octet_decoder{ctx}, msg));
	EXPECT_EQ(med::error::OVERFLOW, ctx.error_ctx().get_error()) << toString(ctx.error_ctx());
#endif
}

TEST(length, c_lv)
{
	uint8_t const encoded[] = {
		3, //len
		1, 2, 3
	};

	med::decoder_context<> ctx{encoded};
	len::C_LV msg;

#if (MED_EXCEPTIONS)
	decode(med::octet_decoder{ctx}, msg);
#else
	ASSERT_TRUE(decode(med::octet_decoder{ctx}, msg)) << toString(ctx.error_ctx());
#endif

	auto const* var = msg.get<len::VAR>();
	ASSERT_NE(nullptr, var);
	uint8_t const exp[] = {1,2,3};
	ASSERT_EQ(std::size(exp), var->size());
	EXPECT_TRUE(Matches(exp, var->data()));

	//constraints violation: length shorter
	uint8_t const shorter[] = {
		2, //len
		1, 2
	};
	ctx.reset(shorter,sizeof(shorter));
#if (MED_EXCEPTIONS)
	EXPECT_THROW(decode(med::octet_decoder{ctx}, msg), med::invalid_value);
#else
	ASSERT_FALSE(decode(med::octet_decoder{ctx}, msg));
	EXPECT_EQ(med::error::INCORRECT_VALUE, ctx.error_ctx().get_error()) << toString(ctx.error_ctx());
#endif
}

#if 0
TEST(length, seq)
{
	uint8_t const encoded[] = {
		4, //total len
		1, 2, 3, 4, //U8=3, U8=4
	};

	med::decoder_context<> ctx{encoded};
	len::SEQ seq;
#if (MED_EXCEPTIONS)
	decode(med::octet_decoder{ctx}, seq);
#else
	if (!decode(med::octet_decoder{ctx}, seq)) { FAIL() << toString(ctx.error_ctx()); }
#endif

	len::MSG const& msg = seq.cfield();
	auto& u8s = msg.get<len::U8>();
	ASSERT_EQ(2, u8s.count());
	uint8_t const u8_exp[] = {3,4};
	auto* p = u8_exp;
	for (auto const& u : msg.get<len::U8>())
	{
		EXPECT_EQ(*p++, u.get());
	}
}
#endif
