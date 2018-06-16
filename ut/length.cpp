#include "ut.hpp"

using namespace std::string_view_literals;

namespace len {

struct L : med::length_t<med::value<uint8_t>>{};

template <std::size_t TAG>
using T = med::value<med::fixed<TAG, uint32_t>>;

struct U8  : med::value<uint8_t>
{
	bool set_encoded(value_type v) { return (v < 0xF0) ? base_t::set_encoded(v), true : false; }
};
struct U16 : med::value<uint16_t>
{
	bool set_encoded(value_type v) { return (v < 0xF000) ? base_t::set_encoded(v), true : false; }
};
struct U24 : med::value<med::bytes<3>>
{
	bool set_encoded(value_type v) { return (v < 0xF00000) ? base_t::set_encoded(v), true : false; }
};
struct U32 : med::value<uint32_t>
{
	bool set_encoded(value_type v) { return (v < 0xF0000000) ? base_t::set_encoded(v), true : false; }
};

struct CHOICE : med::choice< med::value<uint32_t>
	, med::tag<T<1>, U8>
	, med::tag<T<2>, U16>
	, med::tag<T<4>, U32>
>{};

struct SEQ2 : med::sequence<
	M< U16 >,
	O< T<1>, L, CHOICE >
>{};


struct SEQ : med::sequence<
	M< T<1>, L, U8, med::max<2>>,
	O< T<2>, U16, med::max<2>>,
	O< T<3>, U24, med::max<2>>,
	O< T<4>, L, U32, med::max<2>>
>{};

struct MSGSEQ : med::sequence<
	M< L, SEQ >,
	M< L, SEQ2 >
>{};

struct SET : med::set< med::value<uint32_t>,
	M< T<1>, L, U8, med::max<2>>,
	O< T<2>, U16, med::max<2>>,
	O< T<3>, U24, med::max<2>>,
	O< T<4>, L, U32, med::max<2>>
>{};

//mandatory LMV (length multi-value)
struct MV : med::sequence<
	M<U8, med::pmax<3>>
>{};
struct M_LMV : med::sequence<
	M<L, MV>
>{};

struct VAR : med::ascii_string<med::min<3>, med::max<15>> {};

//conditional LV
struct true_cond
{
	template <class IES>
	bool operator()(IES const&) const   { return true; }
};

struct C_LV : med::sequence<
	O<L, VAR, true_cond>
>{};

//value with length
struct VL : med::value<uint8_t>
{
	//value part
	value_type get() const          { return get_encoded() & 0xF; }
	void set(value_type v)          { set_encoded(v & 0xF); }
	//length part
	value_type get_length() const   { return get_encoded() >> 4; }
	bool set_length(value_type v)   { return (v < 0x10) ? set_encoded((v << 4)|get()), true : false; }
};

struct VLVAR : med::sequence<
	M<VL>,
	M<VAR>
>
{
	using length_type = VL;
};

struct VMSG : med::sequence<
	M< T<1>, VLVAR, med::max<2>>,
	O< T<2>, U16, med::max<2>>,
	O< T<4>, L, U32, med::max<2>>
>{};

//placeholder::_length
struct PL_HDR : med::sequence<
	M< U16 >,
	med::placeholder::_length<6>, //don't count U16+U8 (2+1 bytes) and length encoded as U24 (3 bytes)
	M< U8 >
>
{
	static constexpr char const* name()         { return "PL-Header"; }
};

struct PL_SEQ : med::sequence<
	M< PL_HDR >,
	O< T<0x62>, U32, med::max<2> >
>
{
	using length_type = U24;
	static constexpr char const* name()         { return "PL-Sequence"; }
};

} //namespace len

TEST(length, placeholder)
{
	uint8_t buffer[1024];
	med::encoder_context<> ctx{ buffer };
	med::decoder_context<> dctx;
	using namespace len;

	PL_SEQ msg;

	{
		PL_HDR& hdr = msg.ref<PL_HDR>();
		hdr.ref<U16>().set(0x1661);
		hdr.ref<U8>().set(0x37);

#if (MED_EXCEPTIONS)
		encode(med::octet_encoder{ctx}, msg);
#else
		ASSERT_TRUE(encode(med::octet_encoder{ctx}, msg)) << toString(ctx.error_ctx());
#endif
		uint8_t const encoded[] = {
			0x16, 0x61
			, 0, 0, 0    //length 3 bytes
			, 0x37
		};
		ASSERT_EQ(sizeof(encoded), ctx.buffer().get_offset());
		ASSERT_TRUE(Matches(encoded, buffer));

		dctx.reset(ctx.buffer().get_start(), ctx.buffer().get_offset());
		PL_SEQ dmsg;
#if (MED_EXCEPTIONS)
		decode(med::octet_decoder{dctx}, dmsg);
#else
		ASSERT_TRUE(decode(med::octet_decoder{dctx}, dmsg)) << toString(ctx.error_ctx());
#endif
		PL_HDR& dhdr = dmsg.ref<PL_HDR>();
		EXPECT_EQ(hdr.get<U16>().get(), dhdr.get<U16>().get());
		EXPECT_EQ(hdr.get<U8>().get(), dhdr.get<U8>().get());
	}

	ctx.reset();
	{
		static_assert(msg.arity<U32>() == 2, "");
		msg.push_back<U32>(ctx)->set(0x01020304);
		msg.push_back<U32>(ctx)->set(0x05060708);

#if (MED_EXCEPTIONS)
		encode(med::octet_encoder{ctx}, msg);
#else
		ASSERT_TRUE(encode(med::octet_encoder{ctx}, msg)) << toString(ctx.error_ctx());
#endif
		uint8_t const encoded[] = {
			0x16, 0x61
			, 0, 0, 16    //length 3 bytes
			, 0x37
			,0,0,0,0x62, 1,2,3,4
			,0,0,0,0x62, 5,6,7,8
		};
		ASSERT_EQ(sizeof(encoded), ctx.buffer().get_offset());
		ASSERT_TRUE(Matches(encoded, buffer));

		dctx.reset(ctx.buffer().get_start(), ctx.buffer().get_offset());
		PL_SEQ dmsg;
#if (MED_EXCEPTIONS)
		decode(med::octet_decoder{dctx}, dmsg);
#else
		ASSERT_TRUE(decode(med::octet_decoder{dctx}, dmsg)) << toString(dctx.error_ctx());
#endif
		ASSERT_EQ(msg.count<U32>(), dmsg.count<U32>());
		auto it = dmsg.get<U32>().begin();
		for (auto const& v : msg.get<U32>())
		{
			EXPECT_EQ(v.get(), it->get());
			it++;
		}
	}
}

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
	EXPECT_EQ(med::error::INVALID_VALUE, ctx.error_ctx().get_error()) << toString(ctx.error_ctx());
#endif
}

TEST(length, vlvar)
{
	uint8_t buffer[128];
	med::encoder_context<> ctx{ buffer };

	len::VMSG msg;
	std::string_view const data[] = {"123"sv, "123456"sv};

	for (std::size_t i = 0; i < std::size(data); ++i)
	{
		auto* vlvar = msg.push_back<len::VLVAR>();
		ASSERT_NE(nullptr, vlvar);
		vlvar->ref<len::VL>().set(i + 1);
		vlvar->ref<len::VAR>().set(data[i]);
	}

#if (MED_EXCEPTIONS)
	encode(med::octet_encoder{ctx}, msg);
#else
	ASSERT_TRUE(encode(med::octet_encoder{ctx}, msg)) << toString(ctx.error_ctx());
#endif

	uint8_t encoded[] = {
		0,0,0,1, 0x31, //len=3, val=1
		'1','2','3',
		0,0,0,1, 0x62, //len=6, val=2
		'1','2','3','4','5','6',
	};
	ASSERT_EQ(sizeof(encoded), ctx.buffer().get_offset());
	ASSERT_TRUE(Matches(encoded, buffer));

	//check decode
	med::decoder_context<> dctx{encoded};
	msg.clear();

#if (MED_EXCEPTIONS)
	decode(med::octet_decoder{dctx}, msg);
#else
	ASSERT_TRUE(decode(med::octet_decoder{dctx}, msg)) << toString(dctx.error_ctx());
#endif

	std::size_t index = 1;
	for (auto const& v : msg.get<len::VLVAR>())
	{
		EXPECT_EQ(index, v.get<len::VL>().get());
		EXPECT_EQ(index*3, v.get<len::VL>().get_length());
		EXPECT_EQ(data[index-1], v.get<len::VAR>().get());
		++index;
	}
}

#if 1
TEST(length, seq)
{
	med::decoder_context<> ctx;
	len::MSGSEQ msg;

//	M< U16 >,
//	O< T<1>, L, CHOICE >

	uint8_t const ok[] = {
		12, //SEQ total len
		0,0,0,1, 1, 1, //U8
		0,0,0,2, 1,2, //U16
		2, //SEQ2 total len
		0x1,0x60
	};
	msg.clear(); ctx.reset(ok);
#if (MED_EXCEPTIONS)
	decode(med::octet_decoder{ctx}, msg);
#else
	ASSERT_TRUE(decode(med::octet_decoder{ctx}, msg)) << toString(ctx.error_ctx());
#endif
	EXPECT_EQ(0x160, msg.get<len::SEQ2>().get<len::U16>().get());

	uint8_t const short_total_len[] = {
		11, //SEQ total len
		0,0,0,1, 1, 1, //U8
		0,0,0,2, 1,2, //U16
	};
	msg.clear(); ctx.reset(short_total_len);
#if (MED_EXCEPTIONS)
	EXPECT_THROW(decode(med::octet_decoder{ctx}, msg), med::overflow);
#else
	ASSERT_FALSE(decode(med::octet_decoder{ctx}, msg));
	EXPECT_EQ(med::error::OVERFLOW, ctx.error_ctx().get_error()) << toString(ctx.error_ctx());
#endif

	uint8_t const inv_value[] = {
		12, //SEQ total len
		0,0,0,1, 1, 0xF1, //U8
		0,0,0,2, 1,2, //U16
	};
	msg.clear(); ctx.reset(inv_value);
#if (MED_EXCEPTIONS)
	EXPECT_THROW(decode(med::octet_decoder{ctx}, msg), med::invalid_value);
#else
	ASSERT_FALSE(decode(med::octet_decoder{ctx}, msg));
	EXPECT_EQ(med::error::INVALID_VALUE, ctx.error_ctx().get_error()) << toString(ctx.error_ctx());
#endif

}
#endif
