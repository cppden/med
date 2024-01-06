#include "traits.hpp"
#include "ut.hpp"

namespace cho {

//low nibble selector
template <std::size_t TAG>
struct LT : med::value<med::fixed<TAG, med::bits<4>>> {};

struct HI : med::value<med::bits<4, 0>> {};
struct LO : med::value<med::bits<4, 4>> {};
struct HiLo : med::sequence<
	M< HI >,
	M< LO >
>
{};

template <uint8_t TAG>
struct BCD : med::sequence<
	M< LT<TAG> >, // explicit TAG
	M< LO >,
	O< HiLo, med::max<2> >
>, med::add_meta_info< med::add_tag<LT<TAG>> >
{
	static_assert(0 == (TAG & 0xF0), "LOW NIBBLE TAG EXPECTED");

	bool set(std::span<uint8_t const> data) // nibble per byte
	{
		if (1 <= data.size() && data.size() <= 5)
		{
			auto it = data.begin(), ite = data.end();
			this->template ref<LO>().set(*it++);
			while (it != ite)
			{
				auto* p = this->template ref<HiLo>().push_back();
				p->template ref<HI>().set(*it++);
				if (it != ite)
				{
					p->template ref<LO>().set(*it++);
				}
				else
				{
					p->template ref<LO>().set(0xF);
				}
			}
			return true;
		}
		return false;
	}
};

#if 0
//NOTE: low nibble of 1st octet is a tag
//binary coded decimal: 0x21,0x43 is 1,2,3,4
template <uint8_t TAG>
struct BCD : med::octet_string<med::octets_var_intern<3>, med::min<1>>
	, med::add_meta_info< med::add_tag< LT<TAG> > >
{
	static_assert(0 == (TAG & 0xF0), "LOW NIBBLE TAG EXPECTED");
	static constexpr bool match(uint8_t v)    { return TAG == (v & 0xF); }

	bool set(std::size_t len, void const* data)
	{
		//need additional nibble for the tag
		std::size_t const num_octets = (len + 1);
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

	template <std::size_t N>
	void print(char (&sz)[N]) const
	{
		char* psz = sz;

		auto to_char = [&psz](uint8_t nibble)
		{
			if (nibble != 0xF)
			{
				*psz++ = static_cast<char>(nibble > 9 ? (nibble+0x57) : (nibble+0x30));
			}
		};

		bool b1st = true;
		for (uint8_t digit : *this)
		{
			to_char(uint8_t(digit >> 4));
			//not 1st octet - print both nibbles
			if (not b1st) to_char(digit & 0xF);
			b1st = false;
		}
		*psz = 0;
	}
};
#endif

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

//choice based on plain value selector
struct plain : med::choice<
	M< C<0x00>, L, U8  >,
	M< C<0x02>, L, U16 >,
	M< C<0x04>, L, U32 >,
	M< ANY_TAG, L, UNKNOWN >
>
{};

using PLAIN = M<L, plain>;

} //end: namespace cho

using namespace std::string_view_literals;
using namespace cho;

#if 0
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
TEST(choice, explicit_tag)
{
	uint8_t buffer[32];
	med::encoder_context<> ctx{ buffer };
	med::octet_encoder encoder{ctx};

	FLD_NSCHO msg;
	uint8_t const bcd[] = {0x34, 0x56};
	msg.ref<BCD_1>().set(bcd);
	encode(encoder, msg);
	EXPECT_STRCASEEQ("31 45 6F ", as_string(ctx.buffer()));

#if 0
	decltype(msg) dmsg;
	med::decoder_context<> dctx;
	dctx.reset(ctx.buffer().used());
	decode(med::octet_decoder{dctx}, dmsg);
	auto* pf = dmsg.get<BCD_1>();
	ASSERT_NE(nullptr, pf);
	EXPECT_EQ(msg.get<BCD_1>()->size(), dmsg.get<BCD_1>()->size());

	EXPECT_TRUE(pf->is_set());
	dmsg.clear();
	EXPECT_FALSE(pf->is_set());
#endif
}
#endif
//NOTE: choice compound is tested in length.cpp ppp::proto
