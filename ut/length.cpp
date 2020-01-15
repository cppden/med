#include "ut.hpp"

using namespace std::string_view_literals;

namespace len {

struct L : med::length_t<med::value<uint8_t>>{};

template <std::size_t TAG>
using T = med::value<med::fixed<TAG, uint32_t>>;

struct U8  : med::value<uint8_t>
{
	bool set_encoded(value_type v) { return (v < 0xF0) ? ((void)base_t::set_encoded(v), true) : false; }
};
struct U16 : med::value<uint16_t>
{
	bool set_encoded(value_type v) { return (v < 0xF000) ? ((void)base_t::set_encoded(v), true) : false; }
};
struct U24 : med::value<med::bytes<3>>
{
	bool set_encoded(value_type v) { return (v < 0xF00000) ? ((void)base_t::set_encoded(v), true) : false; }
};
struct U32 : med::value<uint32_t>
{
	bool set_encoded(value_type v) { return (v < 0xF0000000) ? ((void)base_t::set_encoded(v), true) : false; }
};

struct CHOICE : med::choice<
	M<T<1>, U8>,
	M<T<2>, U16>,
	M<T<4>, U32>
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

struct VLARR : med::sequence<
	M<VLVAR, med::max<2>>
>{};

struct LVLARR : med::sequence<
	M<L, VLARR>
>{};

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

//24.501
// 9.11.2.8 (v.15.2.1) S-NSSAI
struct mapped_sst : med::value<uint8_t> {};
struct mapped_sd : med::value<med::bits<24>> {};
struct sst : med::value<uint8_t> {};
struct sd : med::value<med::bits<24>> {};

struct s_nssai_length : med::value<uint8_t>
{
	value_type get_length() const   { return get_encoded(); }
	bool set_length(value_type v)   { set_encoded(v); return true; }

	/*
	sst[1], sd[3], 1st sst is M
	1 = 1 : sst
	2 = 1 + 1: sst + m-sst
	4 = 1 + 3: sst + sd
	5 = 1 + 3 + 1: sst + sd + m-sst
	7 = 1 + 3 + 3: sst + sd + m-sd
	8 = 1 + 3 + 1 + 3: sst + sd + m-sst + m-sd
	*/

	struct has_sd
	{
		template <class T> bool operator()(T const& ies) const
		{
			return ies.template as<s_nssai_length>().get() >= 4;
		}
	};

	struct has_mapped_sst
	{
		template <class T> bool operator()(T const& ies) const
		{
			auto const len = ies.template as<s_nssai_length>().get();
			return len == 2 or len == 5 or len == 8;
		}
	};

	struct has_mapped_sd
	{
		template <class T> bool operator()(T const& ies) const
		{
			auto const len = ies.template as<s_nssai_length>().get();
			return len >= 7;
		}
	};
};

struct s_nssai : med::sequence<
	M< s_nssai_length >
	, M< sst >
	, O< sd, s_nssai_length::has_sd >
	, O< mapped_sst, s_nssai_length::has_mapped_sst >
	, O< mapped_sd, s_nssai_length::has_mapped_sd >
>
{
	using length_type = s_nssai_length;
	static constexpr char const* name() { return "S-NSSAI"; }
};

namespace ppp {
//RFC1994 4.1.  Challenge and Response
/*
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|     Code      |  Identifier   |            Length             |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|  Value-Size   |  Value ...
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|  Name ...
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

Code
	1 for Challenge;
	2 for Response.
*/
struct code : med::value<uint8_t>{};
struct id : med::value<uint8_t>{};
struct cval : med::ascii_string<med::min<1>, med::max<255>> {};
struct rval : med::ascii_string<med::min<1>, med::max<255>> {};
struct name : med::ascii_string<med::min<1>, med::max<255>> {};
struct hdr : med::sequence<
	M<code>,
	M<id>,
	med::placeholder::_length<0>
>
{
	auto get_tag() const    { return get<code>().get(); }
	void set_tag(uint8_t v) { return ref<code>().set(v); }
};

template <std::size_t TAG>
using T = med::value<med::fixed<TAG, uint8_t>>;

struct challenge : med::sequence<
	M<L, cval>,
	O<name>
>
{
};

struct response : med::sequence<
	O<L, rval>
>
{
};

struct proto : med::choice< hdr
	, M<T<1>, challenge>
	, M<T<2>, response>
>
{
	using length_type = med::value<uint16_t>;
};

} //end: namespace ppp

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
	M<SHDR, SHDR::setter<U16>>,
	M<U16>
>{};
struct SMFLD : med::sequence<
	M<SHDR, SHDR::setter<U8>>,
	M<U8, med::max<7>>
>{};
struct SLEN : med::sequence<
	M<L, SFLD>,
	M<L, SMFLD>
>
{
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

		encode(med::octet_encoder{ctx}, msg);

		uint8_t const encoded[] = {
			0x16, 0x61
			, 0, 0, 0    //length 3 bytes
			, 0x37
		};
		ASSERT_EQ(sizeof(encoded), ctx.buffer().get_offset());
		ASSERT_TRUE(Matches(encoded, buffer));

		dctx.reset(ctx.buffer().get_start(), ctx.buffer().get_offset());
		PL_SEQ dmsg;
		decode(med::octet_decoder{dctx}, dmsg);
		PL_HDR& dhdr = dmsg.ref<PL_HDR>();
		EXPECT_EQ(hdr.get<U16>().get(), dhdr.get<U16>().get());
		EXPECT_EQ(hdr.get<U8>().get(), dhdr.get<U8>().get());
	}

	ctx.reset();
	{
		static_assert(msg.arity<U32>() == 2, "");
		msg.push_back<U32>(ctx)->set(0x01020304);
		msg.push_back<U32>(ctx)->set(0x05060708);

		encode(med::octet_encoder{ctx}, msg);
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
		decode(med::octet_decoder{dctx}, dmsg);
		ASSERT_EQ(msg.count<U32>(), dmsg.count<U32>());
		auto it = dmsg.get<U32>().begin();
		for (auto const& v : msg.get<U32>())
		{
			EXPECT_EQ(v.get(), it->get());
			it++;
		}

		msg.clear();
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

	EXPECT_THROW(decode(med::octet_decoder{ctx}, msg), med::out_of_memory);

	ctx.get_allocator().reset(alloc); //assign allocation space
	ctx.reset(); //reset from previous errors
	msg.clear(); //clear message from previous decode

	decode(med::octet_decoder{ctx}, msg);

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

	EXPECT_THROW(decode(med::octet_decoder{ctx}, msg), med::overflow);
}

TEST(length, c_lv)
{
	uint8_t const encoded[] = {
		3, //len
		1, 2, 3
	};

	med::decoder_context<> ctx{encoded};
	len::C_LV msg;

	decode(med::octet_decoder{ctx}, msg);

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
	EXPECT_THROW(decode(med::octet_decoder{ctx}, msg), med::invalid_value);
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

	encode(med::octet_encoder{ctx}, msg);

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

	decode(med::octet_decoder{dctx}, msg);

	std::size_t index = 1;
	for (auto const& v : msg.get<len::VLVAR>())
	{
		EXPECT_EQ(index, v.get<len::VL>().get());
		EXPECT_EQ(index*3, v.get<len::VL>().get_length());
		EXPECT_EQ(data[index-1], v.get<len::VAR>().get());
		++index;
	}
}

TEST(length, lvlarr)
{
	uint8_t buffer[128];

	med::encoder_context<> ctx{ buffer };
	len::LVLARR msg;

	std::string_view const data[] = {"123"sv, "123456"sv};
	for (std::size_t i = 0; i < std::size(data); ++i)
	{
		auto* vlvar = msg.ref<len::VLARR>().push_back<len::VLVAR>();
		vlvar->ref<len::VL>().set(i + 1);
		vlvar->ref<len::VAR>().set(data[i]);
	}

	encode(med::octet_encoder{ctx}, msg);
	size_t size = ctx.buffer().get_offset();

	std::cout << "{";
	for(std::size_t i = 0; i < size; i++)
		std::cout << "0x" << std::hex << (uint32_t)buffer[i] << ((i == size-1) ? "}\n" : ", ");

	uint8_t encoded[] = {
		0x0B, //len=11
		0x31, //len=3, val=1
		'1','2','3',
		0x62, //len=6, val=2
		'1','2','3','4','5','6',
	};

	ASSERT_EQ(sizeof(encoded), ctx.buffer().get_offset());
	ASSERT_TRUE(Matches(encoded, buffer));

	//check decode
	med::decoder_context<> dctx{encoded};
	msg.clear();

	decode(med::octet_decoder{dctx}, msg);

	std::size_t index = 1;
	for (auto const& v : msg.get<len::VLARR>().get<len::VLVAR>())
	{
		EXPECT_EQ(index, v.get<len::VL>().get());
		EXPECT_EQ(index*3, v.get<len::VL>().get_length());
		EXPECT_EQ(data[index-1], v.get<len::VAR>().get());
		++index;
	}
}

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
	decode(med::octet_decoder{ctx}, msg);
	EXPECT_EQ(0x160, msg.get<len::SEQ2>().get<len::U16>().get());

	uint8_t const short_total_len[] = {
		11, //SEQ total len
		0,0,0,1, 1, 1, //U8
		0,0,0,2, 1,2, //U16
	};
	msg.clear(); ctx.reset(short_total_len);
	EXPECT_THROW(decode(med::octet_decoder{ctx}, msg), med::overflow);

	uint8_t const inv_value[] = {
		12, //SEQ total len
		0,0,0,1, 1, 0xF1, //U8
		0,0,0,2, 1,2, //U16
	};
	msg.clear(); ctx.reset(inv_value);
	EXPECT_THROW(decode(med::octet_decoder{ctx}, msg), med::invalid_value);
}

TEST(length, val_cond)
{
	uint8_t buffer[16];
	med::encoder_context<> ctx{ buffer };
	med::decoder_context<> dctx;

	len::s_nssai msg;

	{
		msg.ref<len::sst>().set(2);
		encode(med::octet_encoder{ctx}, msg);
		uint8_t const encoded[] = {1, 0x02};
		ASSERT_EQ(sizeof(encoded), ctx.buffer().get_offset());
		ASSERT_TRUE(Matches(encoded, buffer));

		dctx.reset(ctx.buffer().get_start(), ctx.buffer().get_offset());
		len::s_nssai dmsg;
		decode(med::octet_decoder{dctx}, dmsg);
		EXPECT_EQ(1, dmsg.get<len::s_nssai_length>().get());
		EXPECT_EQ(msg.get<len::sst>().get(), dmsg.get<len::sst>().get());
	}
	msg.clear();
	ctx.reset();
	{
		msg.ref<len::sst>().set(2);
		msg.ref<len::sd>().set(3);
		encode(med::octet_encoder{ctx}, msg);
		uint8_t const encoded[] = {4, 0x02, 0, 0, 0x03};
		ASSERT_EQ(sizeof(encoded), ctx.buffer().get_offset());
		ASSERT_TRUE(Matches(encoded, buffer));

		dctx.reset(ctx.buffer().get_start(), ctx.buffer().get_offset());
		len::s_nssai dmsg;
		decode(med::octet_decoder{dctx}, dmsg);
		EXPECT_EQ(4, dmsg.get<len::s_nssai_length>().get());
		EXPECT_EQ(msg.get<len::sst>().get(), dmsg.get<len::sst>().get());
		ASSERT_EQ(msg.count<len::sd>(), dmsg.count<len::sd>());
		EXPECT_EQ(msg.get<len::sd>()->get(), dmsg.get<len::sd>()->get());
	}
	msg.clear();
	ctx.reset();
	{
		msg.ref<len::sst>().set(2);
		msg.ref<len::sd>().set(3);
		msg.ref<len::mapped_sst>().set(4);
		msg.ref<len::mapped_sd>().set(5);
		encode(med::octet_encoder{ctx}, msg);
		uint8_t const encoded[] = {8, 0x02, 0,0,0x03, 0x04, 0,0,0x05};
		ASSERT_EQ(sizeof(encoded), ctx.buffer().get_offset());
		ASSERT_TRUE(Matches(encoded, buffer));

		dctx.reset(ctx.buffer().get_start(), ctx.buffer().get_offset());
		len::s_nssai dmsg;
		decode(med::octet_decoder{dctx}, dmsg);
		EXPECT_EQ(8, dmsg.get<len::s_nssai_length>().get());
		EXPECT_EQ(msg.get<len::sst>().get(), dmsg.get<len::sst>().get());
		ASSERT_EQ(msg.count<len::sd>(), dmsg.count<len::sd>());
		EXPECT_EQ(msg.get<len::sd>()->get(), dmsg.get<len::sd>()->get());
		ASSERT_EQ(msg.count<len::mapped_sst>(), dmsg.count<len::mapped_sst>());
		EXPECT_EQ(msg.get<len::mapped_sst>()->get(), dmsg.get<len::mapped_sst>()->get());
		ASSERT_EQ(msg.count<len::mapped_sd>(), dmsg.count<len::mapped_sd>());
		EXPECT_EQ(msg.get<len::mapped_sd>()->get(), dmsg.get<len::mapped_sd>()->get());
	}
}

TEST(length, ppp)
{
	{
		uint8_t const encoded[] = {
			1, //code
			3, //id
			0, 16, //len
			1, 'a', //challenge value
			'n','a','m','e','-','v','a','l','u','e'
		};

		med::decoder_context<> ctx{encoded};
		len::ppp::proto msg;

		decode(med::octet_decoder{ctx}, msg);

		EXPECT_EQ(3, msg.header().get<len::ppp::id>().get());
		len::ppp::challenge const* c = msg.cselect();
		ASSERT_NE(nullptr, c);
		EXPECT_EQ(1, c->get<len::ppp::cval>().size());
		EXPECT_EQ('a', c->get<len::ppp::cval>().data()[0]);
		len::ppp::name const* name = c->field();
		ASSERT_NE(nullptr, name);
		EXPECT_EQ(10, name->size());
	}
	{
		uint8_t const encoded[] = {
			2, //code
			5, //id
			0, 6, //len
			1, 'r', //response value
		};

		med::decoder_context<> ctx{encoded};
		len::ppp::proto msg;

		decode(med::octet_decoder{ctx}, msg);

		EXPECT_EQ(5, msg.header().get<len::ppp::id>().get());
		len::ppp::response const* r = msg.cselect();
		ASSERT_NE(nullptr, r);
		len::ppp::rval const* rval = r->field();
		ASSERT_NE(nullptr, rval);
		EXPECT_EQ(1, rval->size());
	}
	{
		uint8_t const encoded[] = {
			2, //code
			5, //id
			0, 4, //len
		};

		med::decoder_context<> ctx{encoded};
		len::ppp::proto msg;

		decode(med::octet_decoder{ctx}, msg);

		EXPECT_EQ(5, msg.header().get<len::ppp::id>().get());
		len::ppp::response const* r = msg.cselect();
		ASSERT_NE(nullptr, r);
		len::ppp::rval const* rval = r->field();
		EXPECT_EQ(nullptr, rval);
	}
}

TEST(length, setter_with_length)
{
	using namespace len;
	SLEN msg;
	msg.ref<SFLD>().ref<U16>().set(0x55AA);
	for (std::size_t i = 0; i < 7; ++i)
	{
		msg.ref<SMFLD>().push_back<U8>()->set(i);
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

