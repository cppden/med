#include "traits.hpp"
#include "ut.hpp"

namespace cho {

//low nibble selector
template <std::size_t TAG>
struct LT : med::value<med::fixed<TAG, med::bits<4, 0>>> {};

struct HI : med::value<med::bits<4, 0>> {};
struct LO : med::value<med::bits<4, 4>> {};
struct HL : med::sequence<
	M< HI >,
	M< LO >
>
{};

/* Binary Coded Decimal, i.e. each nibble is 0..9
xxxx.xxxx
TAG  1
xxxx.xxxx|xxxx.xxxx
TAG  1    2    F
xxxx.xxxx|xxxx.xxxx
TAG  1    2    3
xxxx.xxxx|xxxx.xxxx|xxxx.xxxx
TAG  1    2    3    4    F
xxxx.xxxx|xxxx.xxxx|xxxx.xxxx
TAG  1    2    3    4    5
*/
template <uint8_t TAG>
struct BCD : med::sequence<
	M< LO >,
	O< HL, med::max<2> >
>, med::add_meta_info< med::add_tag<LT<TAG>> >
{
	static_assert(0 == (TAG & 0xF0), "LOW NIBBLE TAG EXPECTED");
	static constexpr uint8_t END = 0xF;

	bool set(std::span<uint8_t const> data)
	{
		if (1 <= data.size() && data.size() <= 5)
		{
			auto it = data.begin(), ite = data.end();
			this->template ref<LO>().set(*it++);
			while (it != ite)
			{
				auto* p = this->template ref<HL>().push_back();
				p->template ref<HI>().set(*it++);
				p->template ref<LO>().set(it != ite ? *it++ : END);
			}
			return true;
		}
		return false;
	}

	size_t size() const noexcept
	{
		size_t res = this->template get<LO>().is_set() ? 1 : 0;
		if (auto* p = this->template get<HL>())
		{
			res += 2 * p->count();
			res -= ((*p->last() & END) == END);
		}
		return res;
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

struct ANY_TAG : med::value<uint8_t>
{
	static constexpr char const* name()     { return "ANY"; }
	static constexpr bool match(value_type) { return true; }
};

struct ANY_DATA : med::octet_string<>
{
	static constexpr char const* name() { return "RAW data"; }
};

struct UNKNOWN : med::sequence<
	M< ANY_TAG >,
	M< ANY_DATA >
>
{
	using container_t::get;

	ANY_TAG::value_type get() const             { return this->get<ANY_TAG>().get(); }
	void set(ANY_TAG::value_type v)             { this->ref<ANY_TAG>().set(v); }

	std::size_t size() const                    { return this->get<ANY_DATA>().size(); }
	uint8_t const* data() const                 { return this->get<ANY_DATA>().data(); }
	void set(std::size_t len, void const* data) { this->ref<ANY_DATA>().set(len, data); }
};


struct U8  : med::value<uint8_t>{};
struct U16 : med::value<uint16_t>{};
struct U32 : med::value<uint32_t>{};
struct string : med::ascii_string<> {};

//choice based on plain value selector
struct plain : med::choice<
	M< C<0x00>, L, U8  >,
	M< C<0x02>, L, U16 >,
	M< C<0x04>, L, U32 >,
	M< ANY_TAG, L, UNKNOWN >
>
{};

using PLAIN = M<L, plain>;

struct hdr : med::sequence<
	M<U8>,
	O<U16>
>
{
	bool is_tag_set() const { return get<U8>().is_set(); }
	auto get_tag() const    { return get<U8>().get(); }
	void set_tag(uint8_t v) { return ref<U8>().set(v); }
};

//choice based on a compound header selector
struct compound : med::choice< hdr,
	M< C<0x00>, U8  >,
	M< C<0x02>, U16 >,
	M< C<0x04>, string >
>
{};

struct SEQ : med::sequence<
	  O< U32 >
	, M< U16 >
	, M< compound >
>
{};

} //end: namespace cho

using namespace std::string_view_literals;
using namespace cho;

#if 1
TEST(choice, plain)
{
	uint8_t buffer[32];
	med::encoder_context<> ctx{ buffer };
	med::octet_encoder encoder{ctx};

	PLAIN msg;
	EXPECT_EQ(0, msg.calc_length(encoder));
	msg.ref<U8>().set(0);
	EXPECT_EQ(3, msg.calc_length(encoder));
	msg.ref<U16>().set(0);
	EXPECT_EQ(4, msg.calc_length(encoder));

	constexpr uint16_t magic = 0x1234;
	msg.ref<U16>().set(magic);
	encode(encoder, msg);
	EXPECT_STRCASEEQ("04 02 02 12 34 ", as_string(ctx.buffer()));

	PLAIN dmsg;
	med::decoder_context<> dctx;
	dctx.reset(ctx.buffer().used());
	decode(med::octet_decoder{dctx}, dmsg);
	auto* pf = dmsg.get<U16>();
	ASSERT_NE(nullptr, pf);
	EXPECT_EQ(magic, pf->get());

	EXPECT_TRUE(pf->is_set());

	ctx.buffer().reset(buffer, sizeof(buffer));
	encode(encoder, dmsg);
	EXPECT_STRCASEEQ("04 02 02 12 34 ", as_string(ctx.buffer()));

	dmsg.clear();
//TODO: gcc-11.1.0 bug?
//	EXPECT_FALSE(pf->is_set());
}

TEST(choice, any)
{
	uint8_t encoded[] = {6, 3, 4, 5,6,7,8};
	med::decoder_context<> ctx{encoded};
	PLAIN msg;
	decode(med::octet_decoder{ctx}, msg);

	auto* pf = msg.get<UNKNOWN>();
	ASSERT_NE(nullptr, pf);
	EXPECT_EQ(3, pf->get<ANY_TAG>().get());
	EXPECT_EQ(4, pf->get<ANY_DATA>().size());

	uint8_t buffer[32];
	med::encoder_context<> ectx{ buffer };
	med::octet_encoder encoder{ectx};
	encode(encoder, msg);
	EXPECT_STRCASEEQ("06 03 04 05 06 07 08 ", as_string(ectx.buffer()));
}
#endif
#if 1
TEST(choice, nibble_tag)
{
	uint8_t buffer[32];
	med::encoder_context<> ctx{ buffer };
	med::octet_encoder encoder{ctx};

	FLD_NSCHO msg;
	uint8_t const bcd[] = {3, 4, 5, 6};
	auto& c = msg.ref<BCD_1>();
	//c.ref<LT<1>>().set(3);
	//EXPECT_EQ(1, c.ref<LT<1>>().get());
	c.set(bcd);
	encode(encoder, msg);
	EXPECT_STRCASEEQ("13 45 6F ", as_string(ctx.buffer()));
	decltype(msg) dmsg;
	med::decoder_context<> dctx{ctx.buffer().used()};
	decode(med::octet_decoder{dctx}, dmsg);
	auto* pf = dmsg.get<BCD_1>();
	ASSERT_NE(nullptr, pf);
	//EXPECT_EQ(msg.get<BCD_1>()->size(), pf->size());
	EXPECT_TRUE(msg == dmsg);

	EXPECT_TRUE(pf->is_set());
	dmsg.clear();
	EXPECT_FALSE(pf->is_set());
}
#endif
TEST(choice, tag_in_compound)
{
	uint8_t buffer[128];
	med::encoder_context ctx{ buffer };

	cho::compound msg;
	msg.ref<string>().set("12345678"sv);
	ASSERT_TRUE(msg.header().is_tag_set());

	msg.header().clear();
	ASSERT_FALSE(msg.header().is_tag_set());

	msg.ref<string>();
	ASSERT_TRUE(msg.header().is_tag_set());

	msg.header().clear();
	msg.ref<string>().set("12345678"sv);
	ASSERT_TRUE(msg.header().is_tag_set());

	msg.header().ref<U16>().set(0xff);

	encode(med::octet_encoder{ctx}, msg);
	EXPECT_STREQ(
		"04 00 FF 31 32 33 34 35 36 37 38 ",
		as_string(ctx.buffer())
	);

	msg.clear();
	ASSERT_FALSE(msg.header().is_tag_set());

	med::decoder_context dctx;
	dctx.reset(ctx.buffer().get_start(), ctx.buffer().get_offset());
	decode(med::octet_decoder{dctx}, msg);

	auto* pf = msg.get<string>();
	ASSERT_NE(nullptr, pf);
	ASSERT_TRUE(msg.header().is_tag_set());
}

TEST(choice, m_choice_in_seq)
{
	uint8_t buffer[128];
	med::encoder_context ctx{ buffer };

	SEQ msg;

	msg.ref<U16>().set(1);
	msg.ref<compound>().ref<string>().set("12345678"sv);

	encode(med::octet_encoder{ctx}, msg);
	EXPECT_STREQ(
		"00 01 04 31 32 33 34 35 36 37 38 ",
		as_string(ctx.buffer())
	);
}
//NOTE: choice compound is tested in length.cpp ppp::proto
