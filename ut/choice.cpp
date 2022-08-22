#include "traits.hpp"
//#include "compound.hpp"
#include "ut.hpp"

namespace cho {

struct nid_type : med::value<uint8_t>
{
	enum : value_type { IPv4 = 0x00, IPv6 = 0x01 };
	value_type get() const { return get_encoded() & 0x0f; }
};

struct ipv4_addr : med::octet_string<med::octets_fix_intern<4>> {};
struct ipv6_addr : med::octet_string<med::octets_fix_intern<16>> {};

struct nid : med::choice<
	M<med::value<med::fixed<nid_type::IPv4, uint8_t>>, ipv4_addr>,
	M<med::value<med::fixed<nid_type::IPv6, uint8_t>>, ipv6_addr>
> {};



//low nibble selector
template <std::size_t TAG>
struct LT : med::value<med::fixed<TAG, uint8_t>>, med::peek_t
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
struct PLAIN : med::choice<
	M< C<0x00>, L, U8  >,
	M< C<0x02>, L, U16 >,
	M< C<0x04>, L, U32 >,
	M< ANY_TAG, L, UNKNOWN >
>
{};

using PLAIN_MSG = M<L, PLAIN>;

struct code : U16 {};

template <uint16_t CODE>
struct fixed : med::value<med::fixed<CODE, code::value_type>>
{
};

template <code::value_type CODE = 0, class BODY = med::empty<>>
struct hdr : med::sequence<
	med::placeholder::_length<>,
	M< fixed<CODE> >,
	M< BODY >
>
{
	BODY const& body() const              { return this->template get<BODY>(); }
	BODY& body()                          { return this->template ref<BODY>(); }
};

template <>
struct hdr<0, med::empty<>> : med::sequence<
	med::placeholder::_length<>,
	M<code>
>
{
	std::size_t get_tag() const           { return this->template get<code>().get(); }
	void set_tag(std::size_t val)         { this->template ref<code>().set(val); }
};


template <code::value_type CODE, class BODY>
struct avp : hdr<CODE, BODY>, med::add_meta_info< med::mi<med::mik::TAG, fixed<CODE>> >
{
	using length_type = U16;

	bool is_set() const                   { return this->body().is_set(); }
	//using hdr<CODE, BODY>::get;
	decltype(auto) get() const            { return this->body().get(); }
	template <typename... ARGS>
	auto set(ARGS&&... args)              { return this->body().set(std::forward<ARGS>(args)...); }
};

struct string : avp<0x1, med::ascii_string<>> {};
struct number : avp<0x2, U32> {};

//choice based on compound selector
struct CHOICE : med::choice< hdr<>
	, M< string >
	, M< number >
>
{
	using length_type = U16;
};


} //end: namespace cho

using namespace std::string_view_literals;
using namespace cho;

TEST(choice, plain)
{
	uint8_t buffer[32];
	med::encoder_context<> ctx{ buffer };
	med::octet_encoder encoder{ctx};

	PLAIN_MSG msg;
	EXPECT_EQ(0, msg.calc_length(encoder));
	msg.ref<U8>().set(0);
	EXPECT_EQ(3, msg.calc_length(encoder));
	msg.ref<U16>().set(0);
	EXPECT_EQ(4, msg.calc_length(encoder));

	constexpr uint16_t magic = 0x1234;
	msg.ref<U16>().set(magic);
	encode(encoder, msg);
	EXPECT_STRCASEEQ("04 02 02 12 34 ", as_string(ctx.buffer()));

	PLAIN_MSG dmsg;
	med::decoder_context<> dctx;
	dctx.reset(ctx.buffer().get_start(), ctx.buffer().get_offset());
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

TEST(choice, DISABLED_plain2)
{
	uint8_t buffer[32];
	med::encoder_context<> ctx{buffer};
	med::octet_encoder encoder{ctx};

	cho::nid msg;
	msg.ref<cho::ipv6_addr>().set();
	encode(encoder, msg);
	EXPECT_STRCASEEQ("02 02 12 34 ", as_string(ctx.buffer()));

	cho::nid dmsg;
	med::decoder_context<> dctx;
	dctx.reset(ctx.buffer().get_start(), ctx.buffer().get_offset());
	decode(med::octet_decoder{dctx}, dmsg);
	ctx.buffer().reset(buffer, sizeof(buffer));
	encode(encoder, dmsg);
	EXPECT_STRCASEEQ("02 02 12 34 ", as_string(ctx.buffer()));
}

TEST(choice, any)
{
	uint8_t encoded[] = {3, 4, 5,6,7,8};
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
	EXPECT_STRCASEEQ("03 04 05 06 07 08 ", as_string(ectx.buffer()));
}

TEST(choice, peek)
{
	uint8_t buffer[32];
	med::encoder_context<> ctx{ buffer };
	med::octet_encoder encoder{ctx};

	uint8_t const bcd[] = {0x34, 0x56};
	FLD_NSCHO msg;
	msg.ref<BCD_1>().set(2, bcd);
	encode(encoder, msg);
	EXPECT_STRCASEEQ("31 45 6F ", as_string(ctx.buffer()));

	decltype(msg) dmsg;
	med::decoder_context<> dctx;
	dctx.reset(ctx.buffer().get_start(), ctx.buffer().get_offset());
	decode(med::octet_decoder{dctx}, dmsg);
	auto* pf = dmsg.get<BCD_1>();
	ASSERT_NE(nullptr, pf);
	EXPECT_EQ(msg.get<BCD_1>()->size(), dmsg.get<BCD_1>()->size());

	EXPECT_TRUE(pf->is_set());
	dmsg.clear();
	EXPECT_FALSE(pf->is_set());
}

TEST(choice, compound)
{
	uint8_t buffer[128];
	med::encoder_context<> ctx{ buffer };

	CHOICE msg;
	msg.ref<string>().set("12345678"sv);

	encode(med::octet_encoder{ctx}, msg);
	EXPECT_STRCASEEQ(
		"00 10 00 01 00 0C 00 01 31 32 33 34 35 36 37 38 ",
		as_string(ctx.buffer())
	);

	decltype(msg) dmsg;
	med::decoder_context<> dctx;
	dctx.reset(ctx.buffer().get_start(), ctx.buffer().get_offset());
	decode(med::octet_decoder{dctx}, dmsg);

	auto* pf = dmsg.get<string>();
	ASSERT_NE(nullptr, pf);
	EXPECT_EQ(msg.get<string>()->get(), dmsg.get<string>()->get());

	EXPECT_TRUE(pf->is_set());
	dmsg.clear();
	EXPECT_FALSE(pf->is_set());
}
