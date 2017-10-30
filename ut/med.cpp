#include "med.hpp"
#include "encode.hpp"
#include "decode.hpp"
#include "printer.hpp"
#include "encoder_context.hpp"
#include "decoder_context.hpp"
#include "octet_encoder.hpp"
#include "octet_decoder.hpp"
#include "mandatory.hpp"

#include "ut.hpp"

template <typename ...T>
using M = med::mandatory<T...>;
template <typename ...T>
using O = med::optional<T...>;

#ifdef CODEC_TRACE_ENABLE
// Length
struct L : med::length_t<med::value<uint8_t>>
{
	static constexpr char const* name() { return "Length"; }
};
// Counter
struct CNT : med::counter_t<med::value<uint16_t>>
{
	static constexpr char const* name() { return "Counter"; }
};
// Tag
template <std::size_t TAG>
struct T : med::value<med::fixed<TAG, uint8_t>>
{
	static constexpr char const* name() { return "Tag"; }
};

// Tag/Case in choice
template <std::size_t TAG>
struct C : med::value<med::fixed<TAG>>
{
	static constexpr char const* name() { return "Case"; }
};
// Header
template <uint8_t BITS>
struct HDR : med::value<med::bits<BITS>>
{
	static constexpr char const* name() { return "Header"; }
};
#else
using L = med::length_t<med::value<uint8_t>>;
using CNT = med::counter_t<med::value<uint16_t>>;
template <std::size_t TAG>
using T = med::value<med::fixed<TAG, uint8_t>>;
template <std::size_t TAG>
using C = med::value<med::fixed<TAG>>;
template <uint8_t BITS>
using HDR = med::value<med::bits<BITS>>;
#endif

struct FLD_UC : med::value<uint8_t>
{
	static constexpr char const* name() { return "UC"; }
};
struct FLD_U8 : med::value<uint8_t>
{
	static constexpr char const* name() { return "U8"; }
};

struct FLD_U16 : med::value<uint16_t>
{
	static constexpr char const* name() { return "U16"; }
};
struct FLD_W : med::value<uint16_t>
{
	static constexpr char const* name() { return "Word"; }
};

struct FLD_U24 : med::value<med::bytes<3>>
{
	static constexpr char const* name() { return "U24"; }
};

struct FLD_IP : med::value<uint32_t>
{
	static constexpr char const* name() { return "IP-Address"; }
	template <std::size_t N>
	void print(char (&sz)[N]) const
	{
		uint32_t ip = get();
		std::snprintf(sz, sizeof(sz), "%u.%u.%u.%u", uint8_t(ip >> 24), uint8_t(ip >> 16), uint8_t(ip >> 8), uint8_t(ip));
	}
};

struct FLD_DW : med::value<uint32_t>
{
	static constexpr char const* name() { return "Double-Word"; }
};

//struct VFLD1 : med::octet_string<med::min<5>, med::max<10>, std::vector<uint8_t>>
//struct VFLD1 : med::octet_string<med::min<5>, med::max<10>, boost::container::static_vector<uint8_t, 10>>
struct VFLD1 : med::ascii_string<med::min<5>, med::max<10>>, med::with_snapshot
{
	static constexpr char const* name() { return "url"; }
};

struct custom_length : med::value<uint8_t>
{
	static bool value_to_length(std::size_t& v)
	{
		if (v < 5)
		{
			v = 2*(v + 1);
			return true;
		}
		return false;
	}

	static bool length_to_value(std::size_t& v)
	{
		if (0 == (v & 1)) //should be even
		{
			v = (v - 2) / 2;
			return true;
		}
		return false;
	}

	static constexpr char const* name() { return "Custom-Length"; }
};
using CLEN = med::length_t<custom_length>;

struct MSG_SEQ : med::sequence<
	M< FLD_UC >,              //<V>
	M< T<0x21>, FLD_U16 >,    //<TV>
	M< L, FLD_U24 >,          //<LV>(fixed)
	M< T<0x42>, L, FLD_IP >, //<TLV>(fixed)
	O< T<0x51>, FLD_DW >,     //[TV]
	O< T<0x12>, CLEN, VFLD1 > //TLV(var)
>
{
	static constexpr char const* name() { return "Msg-Seq"; }
};

struct FLD_CHO : med::choice< HDR<8>
	, med::tag<C<0x01>, FLD_U8>
	, med::tag<C<0x02>, FLD_U16>
	, med::tag<C<0x04>, FLD_IP>
>
{
};

struct SEQOF_1 : med::sequence<
	M< FLD_W, med::max<3> >
>
{
	static constexpr char const* name() { return "Seq-Of-1"; }
};

struct SEQOF_2 : med::sequence<
	M< FLD_W >,
	O< T<0x06>, L, FLD_CHO >
>
{
	static constexpr char const* name() { return "Seq-Of-2"; }
};

template <int INSTANCE>
struct SEQOF_3 : med::sequence<
	M< FLD_U8 >,
	M< T<0x21>, FLD_U16 >  //<TV>
>
{
	static constexpr char const* name() { return "Seq-Of-3"; }
	template <std::size_t N>
	void print(char (&sz)[N]) const
	{
		std::snprintf(sz, N, "%u:%04X (%d)"
			, this->template get<FLD_U8>().get(), this->template get<FLD_U16>().get()
			, INSTANCE);
	}
};

template <std::size_t TAG>
struct HT : med::peek<med::value<med::fixed<TAG, uint8_t>>>
{
	static_assert(0 == (TAG & 0xF), "HIGH NIBBLE TAG EXPECTED");
	static constexpr bool match(uint8_t v)    { return TAG == (v & 0xF0); }
};

//low nibble selector
struct LT : med::peek<med::value<uint8_t>>
{
	//value_type get() const                    { return base_t::get() & 0xF; }
	void set_encoded(value_type v)            { base_t::set_encoded(v & 0xF); }
};

//tagged nibble
struct FLD_TN : med::value<uint8_t>, med::tag_t<HT<0xE0>>
{
	enum : value_type
	{
		tag_mask = 0xF0,
		mask     = 0x0F,
	};

	value_type get() const                { return base_t::get() & mask; }
	void set(value_type v)                { set_encoded( tag_type::get() | (v & mask) ); }

	static constexpr char const* name()   { return "Tagged-Bits"; }
	template <std::size_t N>
	void print(char (&sz)[N]) const       { std::snprintf(sz, N, "%02X", get()); }
};


//binary coded decimal: 0x21,0x43 is 1,2,3,4
template <uint8_t tag>
struct BCD : med::octet_string<med::octets_var_intern<3>, med::min<1>>
{
	//NOTE: low nibble of 1st octet is a tag
	using tag_type = med::value<med::fixed<tag, uint8_t>>;

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
			if (!b1st) to_char(digit & 0xF);
			b1st = false;
		}
		*psz = 0;
	}

	bool set(std::size_t len, void const* data)
	{
		//need additional nibble for the tag
		std::size_t num_octets = (len + 1);// / 2;
		if (num_octets >= min_octets && num_octets <= max_octets)
		{
			m_value.resize(num_octets);
			uint8_t* p = m_value.data();
			uint8_t const* in = static_cast<uint8_t const*>(data);

			*p++ = (*in & 0xF0) | tag_type::get_encoded();
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
struct FLD_NSCHO : med::choice<LT
	, med::tag<BCD_1::tag_type, BCD_1>
	, med::tag<BCD_2::tag_type, BCD_2>
>
{
};

struct MSG_MSEQ : med::sequence<
	M< FLD_UC, med::arity<2>>,            //<V>*2
	M< T<0x21>, FLD_U16, med::arity<2>>,  //<TV>*2
	M< L, FLD_U24, med::arity<2>>,        //<LV>*2
	M< T<0x42>, L, FLD_IP, med::max<2>>, //<TLV>(fixed)*[1,2]
	M< T<0x51>, FLD_DW, med::max<2>>,     //<TV>*[1,2]
	M< CNT, SEQOF_3<0>, med::max<2>>,
	O< FLD_TN >,
	O< T<0x60>, L, FLD_CHO>,
	O< T<0x61>, L, SEQOF_1>,
	O< T<0x62>, L, SEQOF_2, med::max<2>>,
	O< T<0x70>, L, FLD_NSCHO >,
//	O< T<0x80>, CNT, SEQOF_3<1>, med::max<3>>, //T[CV]*[0,3]
	O< L, VFLD1, med::max<3> >           //[LV(var)]*[0,3] till EoF
>
{
	static constexpr char const* name() { return "Msg-Multi-Seq"; }
};


struct MSG_SET : med::set< HDR<16>,
	M< T<0x0b>,    FLD_UC >, //<TV>
	M< T<0x21>, L, FLD_U16 >, //<TLV>
	O< T<0x49>, L, FLD_U24 >, //[TLV]
	O< T<0x89>,    FLD_IP >, //[TV]
	O< T<0x22>, L, VFLD1 >   //[TLV(var)]
>
{
	static constexpr char const* name() { return "Msg-Set"; }
};

struct MSG_MSET : med::set< HDR<16>,
	M< T<0x0b>,    FLD_UC, med::arity<2> >, //<TV>*2
	M< T<0x0c>,    FLD_U8, med::max<2> >, //<TV>*[1,2]
	M< T<0x21>, L, FLD_U16, med::max<3> >, //<TLV>*[1,3]
	O< T<0x49>, L, FLD_U24, med::arity<2> >, //[TLV]*2
	O< T<0x89>,    FLD_IP, med::arity<2> >, //[TV]*2
	O< T<0x22>, L, VFLD1, med::max<3> > //[TLV(var)]*[1,3]
>
{
	static constexpr char const* name() { return "Msg-Multi-Set"; }
};

struct FLD_QTY : med::value<uint8_t>
{
	struct counter
	{
		template <class T>
		std::size_t operator()(T const& ies) const
		{
			return static_cast<FLD_QTY const&>(ies).get();
		}
	};
};

struct FLD_FLAGS : med::value<uint8_t>
{
	enum : value_type
	{
		//bits for field presence
		U16 = 1 << 0,
		U24 = 1 << 1,
		QTY = 1 << 2,

		//counters for multi-instance fields
		QTY_MASK = 0x03, // 2 bits to encode 0..3
		UC_QTY = 4,
		U8_QTY = 6,
	};

	template <value_type BITS>
	struct has_bits
	{
		template <class T> bool operator()(T const& ies) const  { return static_cast<FLD_FLAGS const&>(ies).get() & BITS; }
	};

	template <value_type QTY, int DELTA = 0>
	struct counter
	{
		template <class T>
		std::size_t operator()(T const& ies) const
		{
			return std::size_t(((static_cast<FLD_FLAGS const&>(ies).get() >> QTY) & QTY_MASK) + DELTA);
		}
	};

	struct setter
	{
		template <class T>
		void operator()(T& ies) const
		{
			value_type bits =
				(static_cast<FLD_U16&>(ies).is_set() ? U16 : 0 ) |
				(static_cast<FLD_U24&>(ies).is_set() ? U24 : 0 );

			auto const uc_qty = med::field_count(ies.template as<FLD_UC>());
			auto const u8_qty = med::field_count(ies.template as<FLD_U8>());


			CODEC_TRACE("*********** bits=%02X uc=%zu u8=%zu", bits, uc_qty, u8_qty);

			if (std::size_t const u32_qty = med::field_count(ies.template as<FLD_IP>()))
			{
				CODEC_TRACE("*********** u32_qty = %zu", u32_qty);
				bits |= QTY;
				static_cast<FLD_QTY&>(ies).set(u32_qty);
			}

			bits |= (uc_qty-1) << UC_QTY;
			bits |= u8_qty << U8_QTY;
			CODEC_TRACE("*********** setting bits = %02X", bits);

			static_cast<FLD_FLAGS&>(ies).set(bits);
		}
	};
};

//optional fields with functors
struct MSG_FUNC : med::sequence<
	M< FLD_FLAGS, FLD_FLAGS::setter >,
	M< FLD_UC, med::min<2>, med::max<4>, FLD_FLAGS::counter<FLD_FLAGS::UC_QTY, 1> >,
	O< FLD_QTY, FLD_FLAGS::has_bits<FLD_FLAGS::QTY> >, //note: this field is also set in FLD_FLAGS::setter
	O< FLD_U8, med::max<2>, FLD_FLAGS::counter<FLD_FLAGS::U8_QTY> >,
	O< FLD_U16, FLD_FLAGS::has_bits<FLD_FLAGS::U16> >,
	O< FLD_U24, FLD_FLAGS::has_bits<FLD_FLAGS::U24> >,
	O< FLD_IP, med::max<8>, FLD_QTY::counter >
>
{
	static constexpr char const* name() { return "Msg-With-Functors"; }
};


struct PROTO : med::choice< HDR<8>
	, med::tag<C<0x01>, MSG_SEQ>
	, med::tag<C<0x11>, MSG_MSEQ>
	, med::tag<C<0x04>, MSG_SET>
	, med::tag<C<0x14>, MSG_MSET>
	, med::tag<C<0xFF>, MSG_FUNC>
>
{
};

#if 1
TEST(encode, seq_ok)
{
	PROTO proto;

	//mandatory only
	MSG_SEQ& msg = proto.select();
	msg.ref<FLD_UC>().set(37);
	msg.ref<FLD_U16>().set(0x35D9);
	msg.ref<FLD_U24>().set(0xDABEEF);
	msg.ref<FLD_IP>().set(0xFee1ABBA);

	uint8_t buffer[1024];
	med::encoder_context<> ctx{ buffer };

#ifdef MED_NO_EXCEPTION
	if (!encode(make_octet_encoder(ctx), proto)) { FAIL() << toString(ctx.error_ctx()); }
#else
	encode(make_octet_encoder(ctx), proto);
#endif
	uint8_t const encoded1[] = { 1
		, 37
		, 0x21, 0x35, 0xD9
		, 3, 0xDA, 0xBE, 0xEF
		, 0x42, 4, 0xFE, 0xE1, 0xAB, 0xBA
	};
	EXPECT_EQ(sizeof(encoded1), ctx.buffer().get_offset());
	EXPECT_TRUE(Matches(encoded1, buffer));

	//with 1 optional
	ctx.reset();
	msg.ref<FLD_DW>().set(0x01020304);

#ifdef MED_NO_EXCEPTION
	if (!encode(med::make_octet_encoder(ctx), proto)) { FAIL() << toString(ctx.error_ctx()); }
#else
	encode(med::make_octet_encoder(ctx), proto);
#endif
	uint8_t const encoded2[] = { 1
		, 37
		, 0x21, 0x35, 0xD9
		, 3, 0xDA, 0xBE, 0xEF
		, 0x42, 4, 0xFE, 0xE1, 0xAB, 0xBA
		, 0x51, 0x01, 0x02, 0x03, 0x04
	};
	EXPECT_EQ(sizeof(encoded2), ctx.buffer().get_offset());
	EXPECT_TRUE(Matches(encoded2, buffer));

	//with 2 optionals
	ctx.reset();
	msg.ref<VFLD1>().set("test.this!");

#ifdef MED_NO_EXCEPTION
	if (!encode(med::make_octet_encoder(ctx), proto)) { FAIL() << toString(ctx.error_ctx()); }
#else
	encode(med::make_octet_encoder(ctx), proto);
#endif
	uint8_t const encoded3[] = { 1
		, 37
		, 0x21, 0x35, 0xD9
		, 3, 0xDA, 0xBE, 0xEF
		, 0x42, 4, 0xFE, 0xE1, 0xAB, 0xBA
		, 0x51, 0x01, 0x02, 0x03, 0x04
		, 0x12, 4, 't', 'e', 's', 't', '.', 't', 'h', 'i', 's', '!'
	};
	EXPECT_EQ(sizeof(encoded3), ctx.buffer().get_offset());
	EXPECT_TRUE(Matches(encoded3, buffer));

	//check RO access (compile test)
	{
		MSG_SEQ const& cmsg = msg;
		FLD_DW const* cpf = cmsg.field();
		ASSERT_NE(nullptr, cpf);
		FLD_DW& rf = msg.field();
		ASSERT_EQ(0x01020304, rf.get());
		//FLD_DW* pf = msg.field(); //invalid access
	}
}
#endif

#if 1
TEST(encode, seq_fail)
{
	PROTO proto;
	uint8_t buffer[1024];
	med::encoder_context<> ctx{ buffer };

	MSG_SEQ& msg = proto.select();
	msg.ref<FLD_UC>().set(0);
	msg.ref<FLD_U24>().set(0);
	ctx.reset();
#ifdef MED_NO_EXCEPTION
	if (encode(med::make_octet_encoder(ctx), proto)) { FAIL() << toString(ctx.error_ctx()); }
	EXPECT_EQ(med::error::MISSING_IE, ctx.error_ctx().get_error()) << toString(ctx.error_ctx());
#else //!MED_NO_EXCEPTION
	try
	{
		encode(med::make_octet_encoder(ctx), proto);
		FAIL();
	}
	catch (med::exception const& ex)
	{
		EXPECT_EQ(med::error::MISSING_IE, ex.error());
	}
#endif //MED_NO_EXCEPTION
}

TEST(encode, mseq_ok)
{
	uint8_t buffer[1024];
	med::encoder_context<> ctx{ buffer };

	PROTO proto;

	//mandatory only
	MSG_MSEQ& msg = proto.select();
	static_assert(MSG_MSEQ::arity<FLD_UC>() == 2, "");
	msg.push_back<FLD_UC>(ctx)->set(37);
	msg.push_back<FLD_UC>(ctx)->set(38);
	msg.push_back<FLD_U16>(ctx)->set(0x35D9);
	msg.push_back<FLD_U16>(ctx)->set(0x35DA);
	msg.push_back<FLD_U24>(ctx)->set(0xDABEEF);
	msg.push_back<FLD_U24>(ctx)->set(0x22BEEF);
	msg.push_back<FLD_IP>(ctx)->set(0xFee1ABBA);
	msg.push_back<FLD_DW>(ctx)->set(0x01020304);
	{
		auto* s = msg.push_back<SEQOF_3<0>>();
		ASSERT_NE(nullptr, s);
		s->ref<FLD_U8>().set(1);
		s->ref<FLD_U16>().set(2);
	}

#ifdef MED_NO_EXCEPTION
	if (!encode(med::make_octet_encoder(ctx), proto)) { FAIL() << toString(ctx.error_ctx()); }
#else //!MED_NO_EXCEPTION
	encode(med::make_octet_encoder(ctx), proto);
#endif //MED_NO_EXCEPTION

	uint8_t const encoded1[] = { 0x11
		, 37                              //M< FLD_UC, med::arity<2> >
		, 38
		, 0x21, 0x35, 0xD9                //M< T<0x21>, FLD_U16, med::arity<2> >
		, 0x21, 0x35, 0xDA
		, 3, 0xDA, 0xBE, 0xEF             //M< L, FLD_U24, med::arity<2> >
		, 3, 0x22, 0xBE, 0xEF
		, 0x42, 4, 0xFE, 0xE1, 0xAB, 0xBA //M< T<0x42>, L, FLD_IP, med::max<2> >
		, 0x51, 0x01, 0x02, 0x03, 0x04    //M< T<0x51>, FLD_DW, med::max<2> >
		, 0,1, 1, 0x21, 0,2 //M<C16, Seq>  <=2
	};
	EXPECT_EQ(sizeof(encoded1), ctx.buffer().get_offset());
	EXPECT_TRUE(Matches(encoded1, buffer));

	//with more mandatory and optionals
	ctx.reset();

	msg.push_back<FLD_IP>(ctx)->set(0xABBAc001);
	msg.push_back<FLD_DW>(ctx)->set(0x12345678);

	msg.ref<FLD_TN>().set(2);

	msg.ref<FLD_CHO>().ref<FLD_U8>().set(33);
	SEQOF_1& s1 = msg.ref<SEQOF_1>();
	s1.push_back<FLD_W>(ctx)->set(0x1223);
	s1.push_back<FLD_W>(ctx)->set(0x3445);

	SEQOF_2* s2 = msg.push_back<SEQOF_2>(ctx);
	s2->ref<FLD_W>().set(0x1122);
	s2->ref<FLD_CHO>().ref<FLD_U16>().set(0x3344);

	msg.push_back<SEQOF_2>(ctx)->ref<FLD_W>().set(0x3344);

	uint8_t const bcd[] = {0x34, 0x56};
	msg.ref<FLD_NSCHO>().ref<BCD_1>().set(2, bcd);

	//O< T<0x80>, CNT, SEQOF_3<1>, med::max<3>>, //T[CV]*[0,3]
//	{
//		auto* s = msg.push_back<SEQOF_3<1>>(ctx);
//		s->ref<FLD_U8>().set(7);
//		s->ref<FLD_U16>().set(0x7654);
//		s = msg.push_back<SEQOF_3<1>>(ctx);
//		s->ref<FLD_U8>().set(9);
//		s->ref<FLD_U16>().set(0x9876);
//	}

	msg.push_back<VFLD1>(ctx)->set("test.this");
	msg.push_back<VFLD1>(ctx)->set("test.it");

#ifdef MED_NO_EXCEPTION
	if (!encode(med::make_octet_encoder(ctx), proto)) { FAIL() << toString(ctx.error_ctx()); }
#else //!MED_NO_EXCEPTION
	encode(med::make_octet_encoder(ctx), proto);
#endif //MED_NO_EXCEPTION
	uint8_t const encoded2[] = { 0x11
		, 37, 38 //2*<FLD_UC>
		, 0x21, 0x35, 0xD9 //2*<T=0x21, FLD_U16>
		, 0x21, 0x35, 0xDA
		, 3, 0xDA, 0xBE, 0xEF //2*<L, FLD_U24>
		, 3, 0x22, 0xBE, 0xEF
		, 0x42, 4, 0xFE, 0xE1, 0xAB, 0xBA //2*<T=0x42, L, FLD_IP>
		, 0x42, 4, 0xAB, 0xBA, 0xC0, 0x01
		, 0x51, 0x01, 0x02, 0x03, 0x04 //2*<T=0x51, FLD_DW>
		, 0x51, 0x12, 0x34, 0x56, 0x78
		, 0,1, 1, 0x21, 0,2 //1*<C16, SEQOF_3<u8, 21h=u16>
		, 0xE2 //[FLD_TN]
		, 0x60, 2, 1, 33 //[T=0x60, L, FLD_CHO]
		, 0x61, 4 //[T=0x61, L, SEQOF_1]
			, 0x12,0x23, 0x34,0x45 //2*<FLD_W>
		, 0x62, 7 //[T=0x62, L, SEQOF_2]
			, 0x11,0x22 //<FLD_W>
			, 0x06, 3, 2, 0x33,0x44 //O[T=0x06, L, FLD_CHO]
		, 0x62, 2 //[T=0x62, L, SEQOF_2]
			, 0x33,0x44 //<FLD_W>
			//O< T<0x06>, L, FLD_CHO >
		, 0x70, 3, 0x31, 0x45, 0x6F //[T=0x70, L, FLD_NSCHO]
//		, 0x80, 0,2,  7, 0x76,0x54,  9, 0x98, 0x76 //[T=0x80, C16, SEQOF_3<u8, 21h=u16>]
		, 9, 't', 'e', 's', 't', '.', 't', 'h', 'i', 's' //2*[L, VFLD1]
		, 7, 't', 'e', 's', 't', '.', 'i', 't'
	};

	EXPECT_EQ(sizeof(encoded2), ctx.buffer().get_offset());
	EXPECT_TRUE(Matches(encoded2, buffer));

	//check RO access (compile test)
	{
		MSG_MSEQ const& cmsg = msg;
		FLD_DW const* cpf = cmsg.field(1);
		ASSERT_NE(nullptr, cpf);
		ASSERT_EQ(0x12345678, cpf->get());
		FLD_DW const* rf = msg.field(1);
		ASSERT_EQ(0x12345678, rf->get());
		//FLD_DW& rpf = msg.field(); //invalid access
		//FLD_DW* pf = msg.field(); //invalid access
	}
}

TEST(encode, mseq_fail)
{
	uint8_t buffer[1024];
	med::encoder_context<> ctx{ buffer };

	//arity of <V> violation
	{
		PROTO proto;
		MSG_MSEQ& msg = proto.select();
		msg.push_back<FLD_UC>(ctx)->set(37);
		msg.push_back<FLD_U16>(ctx)->set(0x35D9);
		msg.push_back<FLD_U16>(ctx)->set(0x35DA);
		msg.push_back<FLD_U24>(ctx)->set(0xDABEEF);
		msg.push_back<FLD_U24>(ctx)->set(0x22BEEF);
		msg.push_back<FLD_IP>(ctx)->set(0xFee1ABBA);
		msg.push_back<FLD_IP>(ctx)->set(0xABBAc001);
		msg.push_back<FLD_DW>(ctx)->set(0x01020304);
		msg.push_back<FLD_DW>(ctx)->set(0x12345678);
		msg.push_back<VFLD1>(ctx)->set("test.this");

		ctx.reset();
#ifdef MED_NO_EXCEPTION
		if (encode(med::make_octet_encoder(ctx), proto)) { FAIL() << toString(ctx.error_ctx()); }
		EXPECT_EQ(med::error::MISSING_IE, ctx.error_ctx().get_error()) << toString(ctx.error_ctx());
#else //!MED_NO_EXCEPTION
		try
		{
			encode(med::make_octet_encoder(ctx), proto);
			FAIL();
		}
		catch (med::exception const& ex)
		{
			EXPECT_EQ(med::error::MISSING_IE, ex.error());
		}
#endif //MED_NO_EXCEPTION
	}

	//arity of <TV> violation
	{
		PROTO proto;
		MSG_MSEQ& msg = proto.select();
		msg.push_back<FLD_UC>(ctx)->set(37);
		msg.push_back<FLD_UC>(ctx)->set(38);
		msg.push_back<FLD_U16>(ctx)->set(0x35D9);
		msg.push_back<FLD_U24>(ctx)->set(0xDABEEF);
		msg.push_back<FLD_U24>(ctx)->set(0x22BEEF);
		msg.push_back<FLD_IP>(ctx)->set(0xFee1ABBA);
		msg.push_back<FLD_IP>(ctx)->set(0xABBAc001);
		msg.push_back<FLD_DW>(ctx)->set(0x01020304);
		msg.push_back<FLD_DW>(ctx)->set(0x12345678);
		msg.push_back<VFLD1>(ctx)->set("test.this");

		ctx.reset();
#ifdef MED_NO_EXCEPTION
		if (encode(med::make_octet_encoder(ctx), proto)) { FAIL() << toString(ctx.error_ctx()); }
		EXPECT_EQ(med::error::MISSING_IE, ctx.error_ctx().get_error()) << toString(ctx.error_ctx());
#else //!MED_NO_EXCEPTION
		try
		{
			encode(med::make_octet_encoder(ctx), proto);
			FAIL();
		}
		catch (med::exception const& ex)
		{
			EXPECT_EQ(med::error::MISSING_IE, ex.error());
		}
#endif //MED_NO_EXCEPTION
	}

	//arity of <LV> violation
	{
		PROTO proto;
		MSG_MSEQ& msg = proto.select();
		msg.push_back<FLD_UC>(ctx)->set(37);
		msg.push_back<FLD_UC>(ctx)->set(38);
		msg.push_back<FLD_U16>(ctx)->set(0x35D9);
		msg.push_back<FLD_U16>(ctx)->set(0x35DA);
		msg.push_back<FLD_U24>(ctx)->set(0xDABEEF);
		msg.push_back<FLD_IP>(ctx)->set(0xFee1ABBA);
		msg.push_back<FLD_IP>(ctx)->set(0xABBAc001);
		msg.push_back<FLD_DW>(ctx)->set(0x01020304);
		msg.push_back<FLD_DW>(ctx)->set(0x12345678);
		msg.push_back<VFLD1>(ctx)->set("test.this");

		ctx.reset();
#ifdef MED_NO_EXCEPTION
		if (encode(med::make_octet_encoder(ctx), proto)) { FAIL() << toString(ctx.error_ctx()); }
		EXPECT_EQ(med::error::MISSING_IE, ctx.error_ctx().get_error()) << toString(ctx.error_ctx());
#else //!MED_NO_EXCEPTION
		try
		{
			encode(med::make_octet_encoder(ctx), proto);
			FAIL();
		}
		catch (med::exception const& ex)
		{
			EXPECT_EQ(med::error::MISSING_IE, ex.error());
		}
#endif //MED_NO_EXCEPTION
	}

	//arity of <TLV> violation
	{
		PROTO proto;
		MSG_MSEQ& msg = proto.select();
		msg.push_back<FLD_UC>(ctx)->set(37);
		msg.push_back<FLD_UC>(ctx)->set(38);
		msg.push_back<FLD_U16>(ctx)->set(0x35D9);
		msg.push_back<FLD_U16>(ctx)->set(0x35DA);
		msg.push_back<FLD_U24>(ctx)->set(0xDABEEF);
		msg.push_back<FLD_U24>(ctx)->set(0x22BEEF);
		msg.push_back<FLD_DW>(ctx)->set(0x01020304);
		msg.push_back<FLD_DW>(ctx)->set(0x12345678);
		msg.push_back<VFLD1>(ctx)->set("test.this");

		ctx.reset();
#ifdef MED_NO_EXCEPTION
		if (encode(med::make_octet_encoder(ctx), proto)) { FAIL() << toString(ctx.error_ctx()); }
		EXPECT_EQ(med::error::MISSING_IE, ctx.error_ctx().get_error()) << toString(ctx.error_ctx());
#else //!MED_NO_EXCEPTION
		try
		{
			encode(med::make_octet_encoder(ctx), proto);
			FAIL();
		}
		catch (med::exception const& ex)
		{
			EXPECT_EQ(med::error::MISSING_IE, ex.error());
		}
#endif //MED_NO_EXCEPTION
	}

	//output buffer overflow
	{
		PROTO proto;
		MSG_MSEQ& msg = proto.select();
		msg.push_back<FLD_UC>(ctx)->set(37);
		msg.push_back<FLD_UC>(ctx)->set(38);
		msg.push_back<FLD_U16>(ctx)->set(0x35D9);
		msg.push_back<FLD_U16>(ctx)->set(0x35DA);
		msg.push_back<FLD_U24>(ctx)->set(0xDABEEF);
		msg.push_back<FLD_U24>(ctx)->set(0x22BEEF);
		msg.push_back<FLD_IP>(ctx)->set(0xFee1ABBA);
		msg.push_back<FLD_DW>(ctx)->set(0x01020304);

		uint8_t buffer[10];
		med::encoder_context<> ctx{ buffer };

		ctx.reset();
#ifdef MED_NO_EXCEPTION
		if (encode(med::make_octet_encoder(ctx), proto)) { FAIL() << toString(ctx.error_ctx()); }
		EXPECT_EQ(med::error::OVERFLOW, ctx.error_ctx().get_error()) << toString(ctx.error_ctx());
#else //!MED_NO_EXCEPTION
		try
		{
			encode(med::make_octet_encoder(ctx), proto);
			FAIL();
		}
		catch (med::exception const& ex)
		{
			EXPECT_EQ(med::error::OVERFLOW, ex.error());
		}
#endif //MED_NO_EXCEPTION
	}

}

TEST(encode, set_ok)
{
	PROTO proto;

	MSG_SET& msg = proto.select();

	//mandatory fields
	msg.ref<FLD_UC>().set(0x11);
	msg.ref<FLD_U16>().set(0x35D9);

	uint8_t buffer[1024];
	med::encoder_context<> ctx{ buffer };

#ifdef MED_NO_EXCEPTION
	if (!encode(med::make_octet_encoder(ctx), proto)) { FAIL() << toString(ctx.error_ctx()); }
#else //!MED_NO_EXCEPTION
	encode(med::make_octet_encoder(ctx), proto);
#endif //MED_NO_EXCEPTION

	uint8_t const encoded1[] = { 4
		, 0, 0x0b, 0x11
		, 0, 0x21, 2, 0x35, 0xD9
	};
	EXPECT_EQ(sizeof(encoded1), ctx.buffer().get_offset());
	EXPECT_TRUE(Matches(encoded1, buffer));

	//optional fields
	ctx.reset();
	msg.ref<VFLD1>().set("test.this");

#ifdef MED_NO_EXCEPTION
	if (!encode(med::make_octet_encoder(ctx), proto)) { FAIL() << toString(ctx.error_ctx()); }
#else //!MED_NO_EXCEPTION
	encode(med::make_octet_encoder(ctx), proto);
#endif //MED_NO_EXCEPTION

	uint8_t const encoded2[] = { 4
		, 0, 0x0b, 0x11
		, 0, 0x21, 2, 0x35, 0xD9
		, 0, 0x22, 9, 't', 'e', 's', 't', '.', 't', 'h', 'i', 's'
	};
	EXPECT_EQ(sizeof(encoded2), ctx.buffer().get_offset());
	EXPECT_TRUE(Matches(encoded2, buffer));

	ctx.reset();
	msg.ref<FLD_U24>().set(0xDABEEF);
	msg.ref<FLD_IP>().set(0xfee1ABBA);

	uint8_t const encoded3[] = { 4
		, 0, 0x0b, 0x11
		, 0, 0x21, 2, 0x35, 0xD9
		, 0, 0x49, 3, 0xDA, 0xBE, 0xEF
		, 0, 0x89, 0xFE, 0xE1, 0xAB, 0xBA
		, 0, 0x22, 9, 't', 'e', 's', 't', '.', 't', 'h', 'i', 's'
	};

#ifdef MED_NO_EXCEPTION
	if (!encode(med::make_octet_encoder(ctx), proto)) { FAIL() << toString(ctx.error_ctx()); }
#else //!MED_NO_EXCEPTION
	encode(med::make_octet_encoder(ctx), proto);
#endif //MED_NO_EXCEPTION

	EXPECT_EQ(sizeof(encoded3), ctx.buffer().get_offset());
	EXPECT_TRUE(Matches(encoded3, buffer));
}

TEST(encode, set_fail)
{
	PROTO proto;
	uint8_t buffer[1024];
	med::encoder_context<> ctx{ buffer };

	//missing mandatory fields in set
	MSG_SET& msg = proto.select();
	msg.ref<FLD_UC>().set(0);
	msg.ref<FLD_U24>().set(0);
	ctx.reset();
#ifdef MED_NO_EXCEPTION
	if (encode(med::make_octet_encoder(ctx), proto)) { FAIL() << toString(ctx.error_ctx()); }
	EXPECT_EQ(med::error::MISSING_IE, ctx.error_ctx().get_error()) << toString(ctx.error_ctx());
#else //!MED_NO_EXCEPTION
	try
	{
		encode(med::make_octet_encoder(ctx), proto);
		FAIL();
	}
	catch (med::exception const& ex)
	{
		EXPECT_EQ(med::error::MISSING_IE, ex.error());
	}
#endif //MED_NO_EXCEPTION
}

TEST(encode, mset_ok)
{
	uint8_t buffer[1024];
	med::encoder_context<> ctx{ buffer };

	PROTO proto;

	MSG_MSET& msg = proto.select();

	//mandatory fields
	msg.push_back<FLD_UC>(ctx)->set(0x11);
	msg.push_back<FLD_UC>(ctx)->set(0x12);
	msg.push_back<FLD_U8>(ctx)->set(0x13);
	msg.push_back<FLD_U16>(ctx)->set(0x35D9);
	msg.push_back<FLD_U16>(ctx)->set(0x35DA);

#ifdef MED_NO_EXCEPTION
	if (!encode(med::make_octet_encoder(ctx), proto)) { FAIL() << toString(ctx.error_ctx()); }
#else //!MED_NO_EXCEPTION
	encode(med::make_octet_encoder(ctx), proto);
#endif //MED_NO_EXCEPTION

	uint8_t const encoded1[] = { 0x14
		, 0, 0x0b, 0x11
		, 0, 0x0b, 0x12
		, 0, 0x0c, 0x13
		, 0, 0x21, 2, 0x35, 0xD9
		, 0, 0x21, 2, 0x35, 0xDA
	};
	EXPECT_EQ(sizeof(encoded1), ctx.buffer().get_offset());
	EXPECT_TRUE(Matches(encoded1, buffer));

	//optional fields
	ctx.reset();
	msg.push_back<VFLD1>(ctx)->set("test.this");

#ifdef MED_NO_EXCEPTION
	if (!encode(med::make_octet_encoder(ctx), proto)) { FAIL() << toString(ctx.error_ctx()); }
#else //!MED_NO_EXCEPTION
	encode(med::make_octet_encoder(ctx), proto);
#endif //MED_NO_EXCEPTION

	uint8_t const encoded2[] = { 0x14
		, 0, 0x0b, 0x11
		, 0, 0x0b, 0x12
		, 0, 0x0c, 0x13
		, 0, 0x21, 2, 0x35, 0xD9
		, 0, 0x21, 2, 0x35, 0xDA
		, 0, 0x22, 9, 't', 'e', 's', 't', '.', 't', 'h', 'i', 's'
	};
	EXPECT_EQ(sizeof(encoded2), ctx.buffer().get_offset());
	EXPECT_TRUE(Matches(encoded2, buffer));

	ctx.reset();
	msg.push_back<FLD_U8>(ctx)->set(0x14);
	msg.push_back<FLD_U24>(ctx)->set(0xDABEEF);
	msg.push_back<FLD_U24>(ctx)->set(0x22BEEF);
	msg.push_back<FLD_IP>(ctx)->set(0xfee1ABBA);
	msg.push_back<FLD_IP>(ctx)->set(0xABBAc001);
	msg.push_back<VFLD1>(ctx)->set("test.it");

	uint8_t const encoded3[] = { 0x14
		, 0, 0x0b, 0x11
		, 0, 0x0b, 0x12
		, 0, 0x0c, 0x13
		, 0, 0x0c, 0x14
		, 0, 0x21, 2, 0x35, 0xD9
		, 0, 0x21, 2, 0x35, 0xDA
		, 0, 0x49, 3, 0xDA, 0xBE, 0xEF
		, 0, 0x49, 3, 0x22, 0xBE, 0xEF
		, 0, 0x89, 0xFE, 0xE1, 0xAB, 0xBA
		, 0, 0x89, 0xAB, 0xBA, 0xc0, 0x01
		, 0, 0x22, 9, 't', 'e', 's', 't', '.', 't', 'h', 'i', 's'
		, 0, 0x22, 7, 't', 'e', 's', 't', '.', 'i', 't'
	};

#ifdef MED_NO_EXCEPTION
	if (!encode(med::make_octet_encoder(ctx), proto)) { FAIL() << toString(ctx.error_ctx()); }
#else //!MED_NO_EXCEPTION
	encode(med::make_octet_encoder(ctx), proto);
#endif //MED_NO_EXCEPTION

	EXPECT_EQ(sizeof(encoded3), ctx.buffer().get_offset());
	EXPECT_TRUE(Matches(encoded3, buffer));
}

TEST(encode, mset_fail)
{
	PROTO proto;
	uint8_t buffer[1024];
	med::encoder_context<> ctx{ buffer };

	//arity violation in optional
	MSG_MSET& msg = proto.select();
	msg.push_back<FLD_UC>(ctx)->set(0);
	msg.push_back<FLD_UC>(ctx)->set(0);
	msg.push_back<FLD_U8>(ctx)->set(0);
	msg.push_back<FLD_U16>(ctx)->set(0);
#ifdef MED_NO_EXCEPTION
	if (!encode(med::make_octet_encoder(ctx), proto)) { FAIL() << toString(ctx.error_ctx()); }
#else //!MED_NO_EXCEPTION
	encode(med::make_octet_encoder(ctx), proto);
#endif //MED_NO_EXCEPTION

	msg.push_back<FLD_U24>(ctx)->set(0);
	ctx.reset();
#ifdef MED_NO_EXCEPTION
	if (encode(med::make_octet_encoder(ctx), proto)) { FAIL() << toString(ctx.error_ctx()); }
	EXPECT_EQ(med::error::MISSING_IE, ctx.error_ctx().get_error()) << toString(ctx.error_ctx());
#else //!MED_NO_EXCEPTION
	try
	{
		encode(med::make_octet_encoder(ctx), proto);
		FAIL();
	}
	catch (med::exception const& ex)
	{
		EXPECT_EQ(med::error::MISSING_IE, ex.error());
	}
#endif //MED_NO_EXCEPTION
}

constexpr std::size_t make_hash(std::size_t v) { return 137*(v + 39); }

TEST(encode, msg_func)
{
	uint8_t buffer[1024];
	med::encoder_context<> ctx{ buffer };

	PROTO proto;

	//mandatory only
	MSG_FUNC& msg = proto.select();
	msg.push_back<FLD_UC>(ctx)->set(37);
	msg.push_back<FLD_UC>(ctx)->set(38);

#ifdef MED_NO_EXCEPTION
	if (!encode(med::make_octet_encoder(ctx), proto)) { FAIL() << toString(ctx.error_ctx()); }
#else //!MED_NO_EXCEPTION
	encode(med::make_octet_encoder(ctx), proto);
#endif //MED_NO_EXCEPTION

	uint8_t const encoded1[] = { 0xFF
		, 0x10
		, 37, 38
	};
	EXPECT_EQ(sizeof(encoded1), ctx.buffer().get_offset());
	EXPECT_TRUE(Matches(encoded1, buffer));

	//with 1 optional and 1 optional counted
	ctx.reset();
	msg.push_back<FLD_UC>(ctx)->set(39);
	msg.push_back<FLD_U8>(ctx)->set('a');
	msg.ref<FLD_U16>().set(0x35D9);

#ifdef MED_NO_EXCEPTION
	if (!encode(med::make_octet_encoder(ctx), proto)) { FAIL() << toString(ctx.error_ctx()); }
#else //!MED_NO_EXCEPTION
	encode(med::make_octet_encoder(ctx), proto);
#endif //MED_NO_EXCEPTION

	uint8_t const encoded2[] = { 0xFF
		, 0x61
		, 37, 38, 39
		, 'a'
		, 0x35, 0xD9
	};
	EXPECT_EQ(sizeof(encoded2), ctx.buffer().get_offset());
	EXPECT_TRUE(Matches(encoded2, buffer));

	//with all optionals
	ctx.reset();
	msg.push_back<FLD_UC>(ctx)->set(40);
	msg.push_back<FLD_U8>(ctx)->set('b');
	msg.ref<FLD_U24>().set(0xDABEEF);
	for (uint8_t i = 0; i < 3; ++i)
	{
		FLD_IP* p = msg.push_back<FLD_IP>(ctx);
		ASSERT_NE(nullptr, p);
		p->set(make_hash(i));
	}

#ifdef MED_NO_EXCEPTION
	if (!encode(med::make_octet_encoder(ctx), proto)) { FAIL() << toString(ctx.error_ctx()); }
#else //!MED_NO_EXCEPTION
	encode(med::make_octet_encoder(ctx), proto);
#endif //MED_NO_EXCEPTION

	uint8_t const encoded3[] = { 0xFF
		, 0xB7
		, 37, 38, 39, 40
		, 3
		, 'a', 'b'
		, 0x35, 0xD9
		, 0xDA, 0xBE, 0xEF
		, 0, 0, 0x14, 0xDF
		, 0, 0, 0x15, 0x68
		, 0, 0, 0x15, 0xF1
	};
	EXPECT_EQ(sizeof(encoded3), ctx.buffer().get_offset());
	EXPECT_TRUE(Matches(encoded3, buffer));
}

TEST(decode, seq_ok)
{
	PROTO proto;

	//mandatory only
	uint8_t const encoded1[] = { 1
		, 37
		, 0x21, 0x35, 0xD9
		, 3, 0xDA, 0xBE, 0xEF
		, 0x42, 4, 0xFE, 0xE1, 0xAB, 0xBA
	};
	med::decoder_context<> ctx;

	ctx.reset(encoded1, sizeof(encoded1));
#ifdef MED_NO_EXCEPTION
	if (!decode(med::make_octet_decoder(ctx), proto)) { FAIL() << toString(ctx.error_ctx()); }
#else //!MED_NO_EXCEPTION
	decode(med::make_octet_decoder(ctx), proto);
#endif //MED_NO_EXCEPTION

	//check RO access (compile test)
	{
		PROTO const& cproto = proto;
		MSG_SEQ const* cmsg = cproto.select();
		ASSERT_NE(nullptr, cmsg);
	}

	MSG_SEQ const* msg = proto.select();
	ASSERT_NE(nullptr, msg);
	EXPECT_EQ(37, msg->get<FLD_UC>().get());
	EXPECT_EQ(1, msg->count<FLD_UC>());
	EXPECT_EQ(0x35D9, msg->get<FLD_U16>().get());
	EXPECT_EQ(0xDABEEF, msg->get<FLD_U24>().get());
	EXPECT_EQ(0xfee1ABBA, msg->get<FLD_IP>().get());

	EXPECT_EQ(0, msg->count<FLD_DW>());
	FLD_DW const* fld5 = msg->field();
	EXPECT_EQ(nullptr, fld5);

	EXPECT_EQ(0, msg->count<VFLD1>());
	VFLD1 const* vfld1 = msg->field();
	EXPECT_EQ(nullptr, vfld1);

	//with 1 optional
	uint8_t const encoded2[] = { 1
		, 37
		, 0x21, 0x35, 0xD9
		, 3, 0xDA, 0xBE, 0xEF
		, 0x42, 4, 0xFE, 0xE1, 0xAB, 0xBA
		, 0x51, 0x01, 0x02, 0x03, 0x04
	};
	ctx.reset(encoded2, sizeof(encoded2));
#ifdef MED_NO_EXCEPTION
	if (!decode(med::make_octet_decoder(ctx), proto)) { FAIL() << toString(ctx.error_ctx()); }
#else //!MED_NO_EXCEPTION
	decode(med::make_octet_decoder(ctx), proto);
#endif //MED_NO_EXCEPTION

	msg = proto.select();
	ASSERT_NE(nullptr, msg);
	EXPECT_EQ(37, msg->get<FLD_UC>().get());
	EXPECT_EQ(0x35D9, msg->get<FLD_U16>().get());
	EXPECT_EQ(0xDABEEF, msg->get<FLD_U24>().get());
	EXPECT_EQ(0xfee1ABBA, msg->get<FLD_IP>().get());

	fld5 = msg->field();
	EXPECT_EQ(1, msg->count<FLD_DW>());
	ASSERT_NE(nullptr, fld5);
	EXPECT_EQ(0x01020304, fld5->get());

	//with 2 optionals
	uint8_t const encoded3[] = { 1
		, 37
		, 0x21, 0x35, 0xD9
		, 3, 0xDA, 0xBE, 0xEF
		, 0x42, 4, 0xFE, 0xE1, 0xAB, 0xBA
		, 0x51, 0x01, 0x02, 0x03, 0x04
		, 0x12, 4, 't', 'e', 's', 't', '.', 't', 'h', 'i', 's', '!'
	};
	ctx.reset(encoded3, sizeof(encoded3));
#ifdef MED_NO_EXCEPTION
	if (!decode(med::make_octet_decoder(ctx), proto)) { FAIL() << toString(ctx.error_ctx()); }
#else //!MED_NO_EXCEPTION
	decode(med::make_octet_decoder(ctx), proto);
#endif //MED_NO_EXCEPTION

	msg = proto.select();
	ASSERT_NE(nullptr, msg);
	EXPECT_EQ(37, msg->get<FLD_UC>().get());
	EXPECT_EQ(0x35D9, msg->get<FLD_U16>().get());
	EXPECT_EQ(0xDABEEF, msg->get<FLD_U24>().get());
	EXPECT_EQ(0xfee1ABBA, msg->get<FLD_IP>().get());

	fld5 = msg->field();
	EXPECT_EQ(1, msg->count<FLD_DW>());
	ASSERT_NE(nullptr, fld5);
	EXPECT_EQ(0x01020304, fld5->get());

	vfld1 = msg->field();
	ASSERT_EQ(1, msg->count<VFLD1>());
	EQ_STRING_O(VFLD1, "test.this!");
}

TEST(decode, seq_fail)
{
	PROTO proto;

	//wrong choice tag
	uint8_t const wrong_choice[] = { 100, 37, 0x49 };
	med::decoder_context<> ctx{ wrong_choice };
#ifdef MED_NO_EXCEPTION
	if (decode(make_octet_decoder(ctx), proto)) { FAIL() << toString(ctx.error_ctx()); }
	EXPECT_EQ(med::error::INCORRECT_TAG, ctx.error_ctx().get_error()) << toString(ctx.error_ctx());
#else //!MED_NO_EXCEPTION
	try
	{
		decode(make_octet_decoder(ctx), proto);
		FAIL();
	}
	catch (med::exception const& ex)
	{
		EXPECT_EQ(med::error::INCORRECT_TAG, ex.error());
	}
#endif //MED_NO_EXCEPTION

	//wrong tag of required field 2
	uint8_t const wrong_tag[] = { 2, 37, 0x49, 0xDA, 0xBE, 0xEF };
	ctx.reset(wrong_tag, sizeof(wrong_tag));
#ifdef MED_NO_EXCEPTION
	if (decode(make_octet_decoder(ctx), proto)) { FAIL() << toString(ctx.error_ctx()); }
	EXPECT_EQ(med::error::INCORRECT_TAG, ctx.error_ctx().get_error()) << toString(ctx.error_ctx());
#else //!MED_NO_EXCEPTION
	try
	{
		decode(make_octet_decoder(ctx), proto);
		FAIL();
	}
	catch (med::exception const& ex)
	{
		EXPECT_EQ(med::error::INCORRECT_TAG, ex.error());
	}
#endif //MED_NO_EXCEPTION

	//missing required field
	uint8_t const missing[] = { 1, 37 };
	ctx.reset(missing, sizeof(missing));
#ifdef MED_NO_EXCEPTION
	if (decode(make_octet_decoder(ctx), proto)) { FAIL() << toString(ctx.error_ctx()); }
	EXPECT_EQ(med::error::OVERFLOW, ctx.error_ctx().get_error()) << toString(ctx.error_ctx());
#else //!MED_NO_EXCEPTION
	try
	{
		decode(make_octet_decoder(ctx), proto);
		FAIL();
	}
	catch (med::exception const& ex)
	{
		EXPECT_EQ(med::error::OVERFLOW, ex.error());
	}
#endif //MED_NO_EXCEPTION

	//incomplete field
	uint8_t const incomplete[] = { 1, 37, 0x21, 0x35 };
	ctx.reset(incomplete, sizeof(incomplete));
#ifdef MED_NO_EXCEPTION
	if (decode(make_octet_decoder(ctx), proto)) { FAIL() << toString(ctx.error_ctx()); }
	EXPECT_EQ(med::error::OVERFLOW, ctx.error_ctx().get_error()) << toString(ctx.error_ctx());
#else //!MED_NO_EXCEPTION
	try
	{
		decode(make_octet_decoder(ctx), proto);
		FAIL();
	}
	catch (med::exception const& ex)
	{
		EXPECT_EQ(med::error::OVERFLOW, ex.error());
	}
#endif //MED_NO_EXCEPTION

	//invalid length of fixed field
	uint8_t const bad_length[] = { 1
		, 37
		, 0x21, 0x35, 0xD9
		, 2, 0xDA, 0xBE, 0xEF
		, 0x42, 4, 0xFE, 0xE1, 0xAB, 0xBA
	};
	ctx.reset(bad_length, sizeof(bad_length));
#ifdef MED_NO_EXCEPTION
	if (decode(make_octet_decoder(ctx), proto)) { FAIL() << toString(ctx.error_ctx()); }
	EXPECT_EQ(med::error::OVERFLOW, ctx.error_ctx().get_error()) << toString(ctx.error_ctx());
#else //!MED_NO_EXCEPTION
	try
	{
		decode(make_octet_decoder(ctx), proto);
		FAIL();
	}
	catch (med::exception const& ex)
	{
		EXPECT_EQ(med::error::OVERFLOW, ex.error());
	}
#endif //MED_NO_EXCEPTION

	//invalid length of variable field (shorter)
	uint8_t const bad_var_len_lo[] = { 1
		, 37
		, 0x21, 0x35, 0xD9
		, 3, 0xDA, 0xBE, 0xEF
		, 0x42, 4, 0xFE, 0xE1, 0xAB, 0xBA
		, 0x12, 3, 't', 'e', 's'
	};
	ctx.reset(bad_var_len_lo, sizeof(bad_var_len_lo));
#ifdef MED_NO_EXCEPTION
	if (decode(make_octet_decoder(ctx), proto)) { FAIL() << toString(ctx.error_ctx()); }
	EXPECT_EQ(med::error::INCORRECT_VALUE, ctx.error_ctx().get_error()) << toString(ctx.error_ctx());
#else //!MED_NO_EXCEPTION
	try
	{
		decode(make_octet_decoder(ctx), proto);
		FAIL();
	}
	catch (med::exception const& ex)
	{
		EXPECT_EQ(med::error::INCORRECT_VALUE, ex.error());
	}
#endif //MED_NO_EXCEPTION

	//invalid length of variable field (longer)
	uint8_t const bad_var_len_hi[] = { 1
		, 37
		, 0x21, 0x35, 0xD9
		, 3, 0xDA, 0xBE, 0xEF
		, 0x42, 4, 0xFE, 0xE1, 0xAB, 0xBA
		, 0x12, 5, 't', 'e', 's', 't', 'e', 's', 't', 'e', 's', 't', 'e'
	};
	ctx.reset(bad_var_len_hi, sizeof(bad_var_len_hi));
#ifdef MED_NO_EXCEPTION
	if (decode(make_octet_decoder(ctx), proto)) { FAIL() << toString(ctx.error_ctx()); }
	EXPECT_EQ(med::error::INCORRECT_VALUE, ctx.error_ctx().get_error()) << toString(ctx.error_ctx());
#else //!MED_NO_EXCEPTION
	try
	{
		decode(make_octet_decoder(ctx), proto);
		FAIL();
	}
	catch (med::exception const& ex)
	{
		EXPECT_EQ(med::error::INCORRECT_VALUE, ex.error());
	}
#endif //MED_NO_EXCEPTION
}


TEST(decode, mseq_ok)
{
	PROTO proto;
	med::decoder_context<> ctx;

	//mandatory only
	uint8_t const encoded1[] = { 0x11
		, 37
		, 38
		, 0x21, 0x35, 0xD9
		, 0x21, 0x35, 0xDA
		, 3, 0xDA, 0xBE, 0xEF
		, 3, 0x22, 0xBE, 0xEF
		, 0x42, 4, 0xFE, 0xE1, 0xAB, 0xBA
		, 0x51, 0x01, 0x02, 0x03, 0x04
		, 0,1, 2, 0x21, 0,3
	};
	ctx.reset(encoded1, sizeof(encoded1));
#ifdef MED_NO_EXCEPTION
	if (!decode(med::make_octet_decoder(ctx), proto)) { FAIL() << toString(ctx.error_ctx()); }
#else //!MED_NO_EXCEPTION
	decode(med::make_octet_decoder(ctx), proto);
#endif //MED_NO_EXCEPTION

	MSG_MSEQ const* msg = proto.select();
	ASSERT_NE(nullptr, msg);
	check_seqof<FLD_UC>(*msg, {37, 38});
	check_seqof<FLD_U16>(*msg, {0x35D9, 0x35DA});
	check_seqof<FLD_U24>(*msg, {0xDABEEF, 0x22BEEF});
	check_seqof<FLD_IP>(*msg, {0xfee1ABBA});
	check_seqof<FLD_DW>(*msg, {0x01020304});
	ASSERT_EQ(1, msg->count<SEQOF_3<0>>());
	{
		auto it = msg->get<SEQOF_3<0>>().begin();
		EXPECT_EQ(2, it->get<FLD_U8>().get());
		EXPECT_EQ(3, it->get<FLD_U16>().get());
	}

	EXPECT_EQ(0, msg->count<VFLD1>());
	VFLD1 const* vfld1 = msg->field(0);
	EXPECT_EQ(nullptr, vfld1);

	//with more mandatory and optionals
	uint8_t const encoded2[] = { 0x11
		, 37
		, 38
		, 0x21, 0x35, 0xD9
		, 0x21, 0x35, 0xDA
		, 3, 0xDA, 0xBE, 0xEF
		, 3, 0x22, 0xBE, 0xEF
		, 0x42, 4, 0xFE, 0xE1, 0xAB, 0xBA
		, 0x42, 4, 0xAB, 0xBA, 0xC0, 0x01
		, 0x51, 0x01, 0x02, 0x03, 0x04
		, 0x51, 0x12, 0x34, 0x56, 0x78
		, 0,2, 3, 0x21, 0,4,  5, 0x21, 0,6
		, 0x60, 2, 1, 33 //O< T<0x60>, L, FLD_CHO >
		, 0x61, 4 //O< T<0x61>, L, SEQOF_1 >
			, 0x12,0x23, 0x34,0x45 //M< FLD_W, med::max<3> >
		, 0x62, 7 //O< T<0x62>, L, SEQOF_2, med::max<2> >
			, 0x11,0x22 //M< FLD_W >,
			, 0x06, 3, 2, 0x33,0x44 //O< T<0x06>, L, FLD_CHO >
		, 0x62, 2 //O< T<0x62>, L, SEQOF_2, med::max<2> >
			, 0x33,0x44 //M< FLD_W >,
			//O< T<0x06>, L, FLD_CHO >

		, 9, 't', 'e', 's', 't', '.', 't', 'h', 'i', 's'
		, 7, 't', 'e', 's', 't', '.', 'i', 't'
	};
	ctx.reset(encoded2, sizeof(encoded2));
#ifdef MED_NO_EXCEPTION
	if (!decode(med::make_octet_decoder(ctx), proto)) { FAIL() << toString(ctx.error_ctx()); }
#else //!MED_NO_EXCEPTION
	decode(med::make_octet_decoder(ctx), proto);
#endif //MED_NO_EXCEPTION

	msg = proto.select();
	ASSERT_NE(nullptr, msg);

	check_seqof<FLD_UC>(*msg, {37, 38});
	check_seqof<FLD_U16>(*msg, {0x35D9, 0x35DA});
	check_seqof<FLD_U24>(*msg, {0xDABEEF, 0x22BEEF});
	check_seqof<FLD_IP>(*msg, {0xfee1ABBA, 0xABBAC001});
	check_seqof<FLD_DW>(*msg, {0x01020304, 0x12345678});
	ASSERT_EQ(2, msg->count<SEQOF_3<0>>());
	{
		auto it = msg->get<SEQOF_3<0>>().begin();
		EXPECT_EQ(3, it->get<FLD_U8>().get());
		EXPECT_EQ(4, it->get<FLD_U16>().get());
		++it;
		EXPECT_EQ(5, it->get<FLD_U8>().get());
		EXPECT_EQ(6, it->get<FLD_U16>().get());
	}

	FLD_CHO const* pcho = msg->field();
	ASSERT_NE(nullptr, pcho);
	FLD_U8 const* pu8 = pcho->select();
	ASSERT_NE(nullptr, pu8);
	EXPECT_EQ(33, pu8->get());

	SEQOF_1 const* pso1 = msg->field();
	ASSERT_NE(nullptr, pso1);
	check_seqof<FLD_W>(*pso1, {0x1223, 0x3445});

	ASSERT_EQ(2, msg->count<SEQOF_2>());
	SEQOF_2 const* pso2 = msg->field(0);
	ASSERT_NE(nullptr, pso2);
	EXPECT_EQ(0x1122, pso2->get<FLD_W>().get());

	pcho = pso2->field();
	ASSERT_NE(nullptr, pcho);
	FLD_U16 const* pu16 = pcho->select();
	ASSERT_NE(nullptr, pu16);
	EXPECT_EQ(0x3344, pu16->get());

	pso2 = msg->field(1);
	ASSERT_NE(nullptr, pso2);
	EXPECT_EQ(0x3344, pso2->get<FLD_W>().get());

	pcho = pso2->field();
	EXPECT_EQ(nullptr, pcho);

	pso2 = msg->field(2);
	EXPECT_EQ(nullptr, pso2);

	check_seqof<VFLD1>(*msg, {"test.this", "test.it"});
}

TEST(decode, mseq_fail)
{
	PROTO proto;
	med::decoder_context<> ctx;

	//wrong tag of required field
	uint8_t const wrong_tag[] = { 0x11
		, 37
		, 38
		, 0x22, 0x35, 0xD9
		, 0x21, 0x35, 0xDA
		, 3, 0xDA, 0xBE, 0xEF
		, 3, 0x22, 0xBE, 0xEF
		, 0x42, 4, 0xFE, 0xE1, 0xAB, 0xBA
		, 0x51, 0x01, 0x02, 0x03, 0x04
		, 0,1, 2, 0x21, 0,3
	};
	ctx.reset(wrong_tag, sizeof(wrong_tag));
#ifdef MED_NO_EXCEPTION
	if (decode(make_octet_decoder(ctx), proto)) { FAIL() << toString(ctx.error_ctx()); }
	EXPECT_EQ(med::error::MISSING_IE, ctx.error_ctx().get_error()) << toString(ctx.error_ctx());
#else //!MED_NO_EXCEPTION
	try
	{
		decode(make_octet_decoder(ctx), proto);
		FAIL();
	}
	catch (med::exception const& ex)
	{
		EXPECT_EQ(med::error::MISSING_IE, ex.error());
	}
#endif //MED_NO_EXCEPTION

	//missing required field
	uint8_t const missing[] = { 0x11
		, 37
		, 38
		, 0x21, 0x35, 0xD9
		, 0x21, 0x35, 0xDA
		, 3, 0xDA, 0xBE, 0xEF
		, 3, 0x22, 0xBE, 0xEF
		, 0x42, 4, 0xFE, 0xE1, 0xAB, 0xBA
		, 0,1, 2, 0x21, 0,3
	};

	ctx.reset(missing, sizeof(missing));
#ifdef MED_NO_EXCEPTION
	if (decode(make_octet_decoder(ctx), proto)) { FAIL() << toString(ctx.error_ctx()); }
	EXPECT_EQ(med::error::MISSING_IE, ctx.error_ctx().get_error()) << toString(ctx.error_ctx());
#else //!MED_NO_EXCEPTION
	try
	{
		decode(make_octet_decoder(ctx), proto);
		FAIL();
	}
	catch (med::exception const& ex)
	{
		EXPECT_EQ(med::error::MISSING_IE, ex.error());
	}
#endif //MED_NO_EXCEPTION

	//incomplete field
	uint8_t const incomplete[] = { 0x11, 37, 38, 0x21, 0x35 };
	ctx.reset(incomplete, sizeof(incomplete));
#ifdef MED_NO_EXCEPTION
	if (decode(make_octet_decoder(ctx), proto)) { FAIL() << toString(ctx.error_ctx()); }
	EXPECT_EQ(med::error::OVERFLOW, ctx.error_ctx().get_error()) << toString(ctx.error_ctx());
#else //!MED_NO_EXCEPTION
	try
	{
		decode(make_octet_decoder(ctx), proto);
		FAIL();
	}
	catch (med::exception const& ex)
	{
		EXPECT_EQ(med::error::OVERFLOW, ex.error());
	}
#endif //MED_NO_EXCEPTION

	//invalid length of fixed field
	uint8_t const bad_length[] = { 0x11
		, 37
		, 38
		, 0x21, 0x35, 0xD9
		, 0x21, 0x35, 0xDA
		, 2, 0xDA, 0xBE, 0xEF
		, 3, 0x22, 0xBE, 0xEF
		, 0x42, 4, 0xFE, 0xE1, 0xAB, 0xBA
		, 0,1, 2, 0x21, 0,3
	};
	ctx.reset(bad_length, sizeof(bad_length));
#ifdef MED_NO_EXCEPTION
	if (decode(make_octet_decoder(ctx), proto)) { FAIL() << toString(ctx.error_ctx()); }
	EXPECT_EQ(med::error::OVERFLOW, ctx.error_ctx().get_error()) << toString(ctx.error_ctx());
#else //!MED_NO_EXCEPTION
	try
	{
		decode(make_octet_decoder(ctx), proto);
		FAIL();
	}
	catch (med::exception const& ex)
	{
		EXPECT_EQ(med::error::OVERFLOW, ex.error());
	}
#endif //MED_NO_EXCEPTION

	//invalid length of variable field (shorter)
	uint8_t const bad_var_len_lo[] = { 0x11
		, 37
		, 38
		, 0x21, 0x35, 0xD9
		, 0x21, 0x35, 0xDA
		, 3, 0xDA, 0xBE, 0xEF
		, 3, 0x22, 0xBE, 0xEF
		, 0x42, 4, 0xFE, 0xE1, 0xAB, 0xBA
		, 0x42, 4, 0xAB, 0xBA, 0xC0, 0x01
		, 0x51, 0x01, 0x02, 0x03, 0x04
		, 0x51, 0x12, 0x34, 0x56, 0x78
		, 0,1, 2, 0x21, 0,3
		, 3, 't', 'e', 's'
	};
	ctx.reset(bad_var_len_lo, sizeof(bad_var_len_lo));
#ifdef MED_NO_EXCEPTION
	if (decode(make_octet_decoder(ctx), proto)) { FAIL() << toString(ctx.error_ctx()); }
	EXPECT_EQ(med::error::INCORRECT_VALUE, ctx.error_ctx().get_error()) << toString(ctx.error_ctx());
#else //!MED_NO_EXCEPTION
	try
	{
		decode(make_octet_decoder(ctx), proto);
		FAIL();
	}
	catch (med::exception const& ex)
	{
		EXPECT_EQ(med::error::INCORRECT_VALUE, ex.error());
	}
#endif //MED_NO_EXCEPTION

	//invalid length of variable field (longer)
	uint8_t const bad_var_len_hi[] = { 0x11
		, 37
		, 38
		, 0x21, 0x35, 0xD9
		, 0x21, 0x35, 0xDA
		, 3, 0xDA, 0xBE, 0xEF
		, 3, 0x22, 0xBE, 0xEF
		, 0x42, 4, 0xFE, 0xE1, 0xAB, 0xBA
		, 0x42, 4, 0xAB, 0xBA, 0xC0, 0x01
		, 0x51, 0x01, 0x02, 0x03, 0x04
		, 0x51, 0x12, 0x34, 0x56, 0x78
		, 0,1, 2, 0x21, 0,3
		, 11, 't', 'e', 's', 't', 'e', 's', 't', 'e', 's', 't', 'e'
	};
	ctx.reset(bad_var_len_hi, sizeof(bad_var_len_hi));
#ifdef MED_NO_EXCEPTION
	if (decode(make_octet_decoder(ctx), proto)) { FAIL() << toString(ctx.error_ctx()); }
	EXPECT_EQ(med::error::OVERFLOW, ctx.error_ctx().get_error()) << toString(ctx.error_ctx());
#else //!MED_NO_EXCEPTION
	try
	{
		decode(make_octet_decoder(ctx), proto);
		FAIL();
	}
	catch (med::exception const& ex)
	{
		EXPECT_EQ(med::error::OVERFLOW, ex.error());
	}
#endif //MED_NO_EXCEPTION
}

TEST(decode, set_ok)
{
	PROTO proto;

	//mandatory fields
	uint8_t const encoded1[] = { 4
		, 0, 0x0b, 0x11
		, 0, 0x21, 2, 0x35, 0xD9
	};
	med::decoder_context<> ctx{ encoded1 };
#ifdef MED_NO_EXCEPTION
	if (!decode(med::make_octet_decoder(ctx), proto)) { FAIL() << toString(ctx.error_ctx()); }
#else //!MED_NO_EXCEPTION
	decode(med::make_octet_decoder(ctx), proto);
#endif //MED_NO_EXCEPTION
	MSG_SET const* msg = proto.select();
	ASSERT_NE(nullptr, msg);

	ASSERT_EQ(0x11, msg->get<FLD_UC>().get());
	ASSERT_EQ(0x35D9, msg->get<FLD_U16>().get());
	FLD_U24 const* fld3 = msg->field();
	FLD_IP const* fld4 = msg->field();
	VFLD1 const* vfld1 = msg->field();
	ASSERT_EQ(nullptr, fld3);
	ASSERT_EQ(nullptr, fld4);
	ASSERT_EQ(nullptr, vfld1);

	//optional fields
	uint8_t const encoded2[] = { 4
		, 0, 0x0b, 0x11
		, 0, 0x21, 2, 0x35, 0xD9
		, 0, 0x22, 9, 't', 'e', 's', 't', '.', 't', 'h', 'i', 's'
	};
	ctx.reset(encoded2, sizeof(encoded2));
#ifdef MED_NO_EXCEPTION
	if (!decode(med::make_octet_decoder(ctx), proto)) { FAIL() << toString(ctx.error_ctx()); }
#else //!MED_NO_EXCEPTION
	decode(med::make_octet_decoder(ctx), proto);
#endif //MED_NO_EXCEPTION
	msg = proto.select();
	ASSERT_NE(nullptr, msg);

	fld3 = msg->field();
	fld4 = msg->field();
	ASSERT_EQ(nullptr, fld3);
	ASSERT_EQ(nullptr, fld4);

	EQ_STRING_O(VFLD1, "test.this");

	//all fields out of order
	uint8_t const encoded3[] = { 4
		, 0, 0x22, 9, 't', 'e', 's', 't', '.', 't', 'h', 'i', 's'
		, 0, 0x89, 0xFE, 0xE1, 0xAB, 0xBA
		, 0, 0x49, 3, 0xDA, 0xBE, 0xEF
		, 0, 0x21, 2, 0x35, 0xD9
		, 0, 0x0b, 0x11
	};
	ctx.reset(encoded3, sizeof(encoded3));
#ifdef MED_NO_EXCEPTION
	if (!decode(med::make_octet_decoder(ctx), proto)) { FAIL() << toString(ctx.error_ctx()); }
#else //!MED_NO_EXCEPTION
	decode(med::make_octet_decoder(ctx), proto);
#endif //MED_NO_EXCEPTION
	msg = proto.select();
	ASSERT_NE(nullptr, msg);

	fld3 = msg->field();
	fld4 = msg->field();
	ASSERT_NE(nullptr, fld3);
	ASSERT_NE(nullptr, fld4);

	ASSERT_EQ(0xDABEEF, fld3->get());
	ASSERT_EQ(0xfee1ABBA, fld4->get());
	EQ_STRING_O(VFLD1, "test.this");
}

TEST(decode, set_fail)
{
	PROTO proto;
	med::decoder_context<> ctx;

	//missing mandatory field
	uint8_t const missing_mandatory[] = { 4
		, 0, 0x22, 9, 't', 'e', 's', 't', '.', 't', 'h', 'i', 's'
		, 0, 0x49, 3, 0xDA, 0xBE, 0xEF
		, 0, 0x21, 2, 0x35, 0xD9
		, 0, 0x89, 0xFE, 0xE1, 0xAB, 0xBA
	};
	ctx.reset(missing_mandatory, sizeof(missing_mandatory));
#ifdef MED_NO_EXCEPTION
	if (decode(make_octet_decoder(ctx), proto)) { FAIL() << toString(ctx.error_ctx()); }
	EXPECT_EQ(med::error::MISSING_IE, ctx.error_ctx().get_error()) << toString(ctx.error_ctx());
#else //!MED_NO_EXCEPTION
	try
	{
		decode(make_octet_decoder(ctx), proto);
		FAIL();
	}
	catch (med::exception const& ex)
	{
		EXPECT_EQ(med::error::MISSING_IE, ex.error());
	}
#endif //MED_NO_EXCEPTION

	//extra field in set
	uint8_t const extra_field[] = { 4
		, 0, 0x49, 3, 0xDA, 0xBE, 0xEF
		, 0, 0x49, 3, 0xDA, 0xBE, 0xEF
	};
	ctx.reset(extra_field, sizeof(extra_field));
#ifdef MED_NO_EXCEPTION
	if (decode(make_octet_decoder(ctx), proto)) { FAIL() << toString(ctx.error_ctx()); }
	EXPECT_EQ(med::error::EXTRA_IE, ctx.error_ctx().get_error()) << toString(ctx.error_ctx());
#else //!MED_NO_EXCEPTION
	try
	{
		decode(make_octet_decoder(ctx), proto);
		FAIL();
	}
	catch (med::exception const& ex)
	{
		EXPECT_EQ(med::error::EXTRA_IE, ex.error());
	}
#endif //MED_NO_EXCEPTION
}

TEST(decode, mset_ok)
{
	PROTO proto;

	//mandatory fields
	uint8_t const encoded1[] = { 0x14
		, 0, 0x0b, 0x11
		, 0, 0x21, 2, 0x35, 0xD9
		, 0, 0x0b, 0x12
		, 0, 0x21, 2, 0x35, 0xDA
		, 0, 0x0c, 0x13
	};
	med::decoder_context<> ctx{ encoded1 };
#ifdef MED_NO_EXCEPTION
	if (!decode(med::make_octet_decoder(ctx), proto)) { FAIL() << toString(ctx.error_ctx()); }
#else //!MED_NO_EXCEPTION
	decode(med::make_octet_decoder(ctx), proto);
#endif //MED_NO_EXCEPTION
	MSG_MSET const* msg = proto.select();
	ASSERT_NE(nullptr, msg);

	EXPECT_EQ(0x11, (msg->get<FLD_UC>(0)->get()));
	EXPECT_EQ(0x12, (msg->get<FLD_UC>(1)->get()));
	EXPECT_EQ(1, msg->count<FLD_U8>());
	EXPECT_EQ(0x13, (msg->get<FLD_U8>(0)->get()));
	EXPECT_EQ(0x35D9, (msg->get<FLD_U16>(0)->get()));
	EXPECT_EQ(0x35DA, (msg->get<FLD_U16>(1)->get()));
	FLD_U24 const* fld3 = msg->field(0);
	FLD_IP const* fld4 = msg->field(0);
	VFLD1 const* vfld1 = msg->field(0);
	EXPECT_EQ(nullptr, fld3);
	EXPECT_EQ(nullptr, fld4);
	EXPECT_EQ(nullptr, vfld1);

	//optional fields
	uint8_t const encoded2[] = { 0x14
		, 0, 0x22, 9, 't', 'e', 's', 't', '.', 't', 'h', 'i', 's'
		, 0, 0x0b, 0x11
		, 0, 0x21, 2, 0x35, 0xD9
		, 0, 0x21, 2, 0x35, 0xDA
		, 0, 0x0c, 0x13
		, 0, 0x0b, 0x12
	};
	ctx.reset(encoded2, sizeof(encoded2));
#ifdef MED_NO_EXCEPTION
	if (!decode(med::make_octet_decoder(ctx), proto)) { FAIL() << toString(ctx.error_ctx()); }
#else //!MED_NO_EXCEPTION
	decode(med::make_octet_decoder(ctx), proto);
#endif //MED_NO_EXCEPTION
	msg = proto.select();
	ASSERT_NE(nullptr, msg);

	fld3 = msg->field(0);
	fld4 = msg->field(0);
	EXPECT_EQ(nullptr, fld3);
	EXPECT_EQ(nullptr, fld4);

	EXPECT_EQ(1, msg->count<VFLD1>());
	EQ_STRING_O_(0, VFLD1, "test.this");

	//all fields out of order
	uint8_t const encoded3[] = { 0x14
		, 0, 0x22, 9, 't', 'e', 's', 't', '.', 't', 'h', 'i', 's'
		, 0, 0x22, 7, 't', 'e', 's', 't', '.', 'i', 't'
		, 0, 0x89, 0xFE, 0xE1, 0xAB, 0xBA
		, 0, 0x0b, 0x11
		, 0, 0x49, 3, 0xDA, 0xBE, 0xEF
		, 0, 0x49, 3, 0x22, 0xBE, 0xEF
		, 0, 0x21, 2, 0x35, 0xD9
		, 0, 0x21, 2, 0x35, 0xDA
		, 0, 0x89, 0xAB, 0xBA, 0xC0, 0x01
		, 0, 0x0b, 0x12
		, 0, 0x0c, 0x13
	};
	ctx.reset(encoded3, sizeof(encoded3));
#ifdef MED_NO_EXCEPTION
	if (!decode(med::make_octet_decoder(ctx), proto)) { FAIL() << toString(ctx.error_ctx()); }
#else //!MED_NO_EXCEPTION
	decode(med::make_octet_decoder(ctx), proto);
#endif //MED_NO_EXCEPTION
	msg = proto.select();
	ASSERT_NE(nullptr, msg);

	fld3 = msg->field(0);
	FLD_U24 const* fld3_2 = msg->field(1);
	fld4 = msg->field(0);
	FLD_IP const* fld4_2 = msg->field(1);
	ASSERT_NE(nullptr, fld3);
	ASSERT_NE(nullptr, fld3_2);
	ASSERT_NE(nullptr, fld4);
	ASSERT_NE(nullptr, fld4_2);

	EXPECT_EQ(0xDABEEF, fld3->get());
	EXPECT_EQ(0x22BEEF, fld3_2->get());
	EXPECT_EQ(0xfee1ABBA, fld4->get());
	EXPECT_EQ(0xABBAc001, fld4_2->get());
	EXPECT_EQ(2, msg->count<VFLD1>());
	EQ_STRING_O_(0, VFLD1, "test.this");
	EQ_STRING_O_(1, VFLD1, "test.it");
}

TEST(decode, mset_fail)
{
	PROTO proto;
	med::decoder_context<> ctx;
//	std::size_t alloc_buf[1024];
//	ctx.get_allocator().reset(alloc_buf);

	//arity underflow in mandatory field
	uint8_t const mandatory_underflow[] = { 0x14
		, 0, 0x0b, 0x11
		, 0, 0x0c, 0x13
		, 0, 0x21, 2, 0x35, 0xD9
	};
	ctx.reset(mandatory_underflow, sizeof(mandatory_underflow));
#ifdef MED_NO_EXCEPTION
	if (decode(make_octet_decoder(ctx), proto)) { FAIL() << toString(ctx.error_ctx()); }
	EXPECT_EQ(med::error::MISSING_IE, ctx.error_ctx().get_error()) << toString(ctx.error_ctx());
#else //!MED_NO_EXCEPTION
	try
	{
		decode(make_octet_decoder(ctx), proto);
		FAIL();
	}
	catch (med::exception const& ex)
	{
		EXPECT_EQ(med::error::MISSING_IE, ex.error());
	}
#endif //MED_NO_EXCEPTION

	//arity overflow in mandatory field
	uint8_t const mandatory_overflow[] = { 0x14
		, 0, 0x0b, 0x11
		, 0, 0x0b, 0x12
		, 0, 0x0c, 0x13
		, 0, 0x0c, 0x14
		, 0, 0x0c, 0x15
		, 0, 0x21, 2, 0x35, 0xD9
	};
	ctx.reset(mandatory_overflow, sizeof(mandatory_overflow));
#ifdef MED_NO_EXCEPTION
	if (decode(make_octet_decoder(ctx), proto)) { FAIL() << toString(ctx.error_ctx()); }
	EXPECT_EQ(med::error::EXTRA_IE, ctx.error_ctx().get_error()) << toString(ctx.error_ctx());
#else //!MED_NO_EXCEPTION
	try
	{
		decode(make_octet_decoder(ctx), proto);
		FAIL();
	}
	catch (med::exception const& ex)
	{
		EXPECT_EQ(med::error::EXTRA_IE, ex.error());
	}
#endif //MED_NO_EXCEPTION
}

TEST(decode, msg_func)
{
	PROTO proto;
	med::decoder_context<> ctx;

	//mandatory only
	uint8_t const encoded1[] = { 0xFF
		, 0x10
		, 37, 38
	};
	ctx.reset(encoded1, sizeof(encoded1));
#ifdef MED_NO_EXCEPTION
	if (!decode(med::make_octet_decoder(ctx), proto)) { FAIL() << toString(ctx.error_ctx()); }
#else //!MED_NO_EXCEPTION
	decode(med::make_octet_decoder(ctx), proto);
#endif //MED_NO_EXCEPTION

	MSG_FUNC const* msg = proto.select();
	ASSERT_NE(nullptr, msg);
	ASSERT_EQ(2, msg->count<FLD_UC>());
	EXPECT_EQ(37, (msg->get<FLD_UC>(0)->get()));
	EXPECT_EQ(38, (msg->get<FLD_UC>(1)->get()));
	ASSERT_EQ(0, msg->count<FLD_U8>());
	FLD_U16 const* pu16 = msg->field();
	EXPECT_EQ(nullptr, pu16);
	FLD_U24 const* pu24 = msg->field();
	EXPECT_EQ(nullptr, pu24);
	EXPECT_EQ(0, msg->count<FLD_IP>());

	//with 1 optional
	uint8_t const encoded2[] = { 0xFF
		, 0x21
		, 37, 38, 39
		, 0x35, 0xD9
	};
	ctx.reset(encoded2, sizeof(encoded2));
#ifdef MED_NO_EXCEPTION
	if (!decode(med::make_octet_decoder(ctx), proto)) { FAIL() << toString(ctx.error_ctx()); }
#else //!MED_NO_EXCEPTION
	decode(med::make_octet_decoder(ctx), proto);
#endif //MED_NO_EXCEPTION

	msg = proto.select();
	ASSERT_NE(nullptr, msg);
	ASSERT_EQ(3, msg->count<FLD_UC>());
	EXPECT_EQ(37, (msg->get<FLD_UC>(0)->get()));
	EXPECT_EQ(38, (msg->get<FLD_UC>(1)->get()));
	EXPECT_EQ(39, (msg->get<FLD_UC>(2)->get()));
	ASSERT_EQ(0, msg->count<FLD_U8>());
	pu16 = msg->field();
	ASSERT_NE(nullptr, pu16);
	EXPECT_EQ(0x35D9, pu16->get());
	pu24 = msg->field();
	EXPECT_EQ(nullptr, pu24);
	EXPECT_EQ(0, msg->count<FLD_IP>());

	//with all optionals
	uint8_t const encoded3[] = { 0xFF
		, 0xB7
		, 37, 38, 39, 40
		, 3
		, 'a', 'b'
		, 0x35, 0xD9
		, 0xDA, 0xBE, 0xEF
		, 0, 0, 0x14, 0xDF
		, 0, 0, 0x15, 0x68
		, 0, 0, 0x15, 0xF1
	};
	ctx.reset(encoded3, sizeof(encoded3));
#ifdef MED_NO_EXCEPTION
	if (!decode(med::make_octet_decoder(ctx), proto)) { FAIL() << toString(ctx.error_ctx()); }
#else //!MED_NO_EXCEPTION
	decode(med::make_octet_decoder(ctx), proto);
#endif //MED_NO_EXCEPTION

	msg = proto.select();
	ASSERT_NE(nullptr, msg);
	ASSERT_EQ(4, msg->count<FLD_UC>());
	EXPECT_EQ(37, (msg->get<FLD_UC>(0)->get()));
	EXPECT_EQ(38, (msg->get<FLD_UC>(1)->get()));
	EXPECT_EQ(39, (msg->get<FLD_UC>(2)->get()));
	EXPECT_EQ(40, (msg->get<FLD_UC>(3)->get()));
	ASSERT_EQ(2, msg->count<FLD_U8>());
	EXPECT_EQ('a', (msg->get<FLD_U8>(0)->get()));
	EXPECT_EQ('b', (msg->get<FLD_U8>(1)->get()));
	pu16 = msg->field();
	ASSERT_NE(nullptr, pu16);
	EXPECT_EQ(0x35D9, pu16->get());
	pu24 = msg->field();
	ASSERT_NE(nullptr, pu24);
	EXPECT_EQ(0xDABEEF, pu24->get());
	FLD_QTY const* pqty = msg->field();
	ASSERT_NE(nullptr, pqty);
	auto const u32_qty = msg->count<FLD_IP>();
	ASSERT_EQ(pqty->get(), u32_qty);
	for (uint8_t i = 0; i < u32_qty; ++i)
	{
		FLD_IP const* p = msg->get<FLD_IP>(i);
		ASSERT_NE(nullptr, p);
		EXPECT_EQ(make_hash(i), p->get());
	}

	//------------ fail cases
	uint8_t const mandatory_underflow[] = { 0xFF
		, 0x07
		, 0x35, 0xD9
		, 0xDA, 0xBE, 0xEF
		, 0xFE, 0xE1, 0xAB, 0xBA
	};
	ctx.reset(mandatory_underflow, sizeof(mandatory_underflow));
#ifdef MED_NO_EXCEPTION
	if (decode(make_octet_decoder(ctx), proto)) { FAIL() << toString(ctx.error_ctx()); }
	EXPECT_EQ(med::error::MISSING_IE, ctx.error_ctx().get_error()) << toString(ctx.error_ctx());
#else //!MED_NO_EXCEPTION
	try
	{
		decode(make_octet_decoder(ctx), proto);
		FAIL();
	}
	catch (med::exception const& ex)
	{
		EXPECT_EQ(med::error::MISSING_IE, ex.error());
	}
#endif //MED_NO_EXCEPTION

	uint8_t const conditional_overflow[] = { 0xFF
		, 0xD7
		, 37, 38
		, 'a', 'b', 'c'
		, 0x35, 0xD9
		, 0xDA, 0xBE, 0xEF
		, 0xFE, 0xE1, 0xAB, 0xBA
	};
	ctx.reset(conditional_overflow, sizeof(conditional_overflow));
#ifdef MED_NO_EXCEPTION
	if (decode(make_octet_decoder(ctx), proto)) { FAIL() << toString(ctx.error_ctx()); }
	EXPECT_EQ(med::error::EXTRA_IE, ctx.error_ctx().get_error()) << toString(ctx.error_ctx());
#else //!MED_NO_EXCEPTION
	try
	{
		decode(make_octet_decoder(ctx), proto);
		FAIL();
	}
	catch (med::exception const& ex)
	{
		EXPECT_EQ(med::error::EXTRA_IE, ex.error());
	}
#endif //MED_NO_EXCEPTION
}


//TODO: add more isolated UTs on fields
TEST(field, tagged_nibble)
{
	uint8_t buffer[4];
	med::encoder_context<> ctx{ buffer };

	FLD_TN field;
	field.set(5);
#ifdef MED_NO_EXCEPTION
	if (!encode(make_octet_encoder(ctx), field)) { FAIL() << toString(ctx.error_ctx()); }
#else //!MED_NO_EXCEPTION
	encode(make_octet_encoder(ctx), field);
#endif //MED_NO_EXCEPTION
	EXPECT_STRCASEEQ("E5 ", as_string(ctx.buffer()));

	decltype(field) dfield;
	med::decoder_context<> dctx;
	dctx.reset(ctx.buffer().get_start(), ctx.buffer().get_offset());
#ifdef MED_NO_EXCEPTION
	if (!decode(make_octet_decoder(dctx), dfield)) { FAIL() << toString(ctx.error_ctx()); }
#else //!MED_NO_EXCEPTION
	decode(make_octet_decoder(dctx), dfield);
#endif //MED_NO_EXCEPTION
	EXPECT_EQ(field.get(), dfield.get());
}

//placeholder::_length
struct PL_HDR : med::sequence<
	M< FLD_U16 >,
	med::placeholder::_length<6>, //don't count U16+U8 (2+1 bytes) and length encoded as U24 (3 bytes)
	M< FLD_U8 >
>
{
	static constexpr char const* name()         { return "PL-Header"; }
};

struct PL_SEQ : med::sequence<
	M< PL_HDR >,
	O< T<0x62>, FLD_DW, med::max<2> >
>
{
	using length_type = FLD_U24;
	static constexpr char const* name()         { return "PL-Sequence"; }
};

TEST(placeholder, length)
{
	uint8_t buffer[1024];
	med::encoder_context<> ctx{ buffer };
	med::decoder_context<> dctx;
	std::size_t alloc_buf[1024];
	dctx.get_allocator().reset(alloc_buf);

	PL_SEQ msg;

	{
		PL_HDR& hdr = msg.ref<PL_HDR>();
		hdr.ref<FLD_U16>().set(0x1661);
		hdr.ref<FLD_U8>().set(0x37);

#ifdef MED_NO_EXCEPTION
		if (!encode(make_octet_encoder(ctx), msg)) { FAIL() << toString(ctx.error_ctx()); }
#else
		encode(make_octet_encoder(ctx), msg);
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
#ifdef MED_NO_EXCEPTION
		if (!decode(med::make_octet_decoder(dctx), dmsg)) { FAIL() << toString(ctx.error_ctx()); }
#else //!MED_NO_EXCEPTION
		decode(med::make_octet_decoder(dctx), dmsg);
#endif //MED_NO_EXCEPTION
		PL_HDR& dhdr = dmsg.ref<PL_HDR>();
		EXPECT_EQ(hdr.get<FLD_U16>().get(), dhdr.get<FLD_U16>().get());
		EXPECT_EQ(hdr.get<FLD_U8>().get(), dhdr.get<FLD_U8>().get());
	}

	ctx.reset();
	{
		static_assert(msg.arity<FLD_DW>() == 2, "");
		msg.push_back<FLD_DW>(ctx)->set(0x01020304);
		msg.push_back<FLD_DW>(ctx)->set(0x05060708);

#ifdef MED_NO_EXCEPTION
		if (!encode(make_octet_encoder(ctx), msg)) { FAIL() << toString(ctx.error_ctx()); }
#else
		encode(make_octet_encoder(ctx), msg);
#endif
		uint8_t const encoded[] = {
			0x16, 0x61
			, 0, 0, 10    //length 3 bytes
			, 0x37
			,0x62, 1,2,3,4
			,0x62, 5,6,7,8
		};
		ASSERT_EQ(sizeof(encoded), ctx.buffer().get_offset());
		ASSERT_TRUE(Matches(encoded, buffer));

		dctx.reset(ctx.buffer().get_start(), ctx.buffer().get_offset());
		PL_SEQ dmsg;
#ifdef MED_NO_EXCEPTION
		if (!decode(med::make_octet_decoder(dctx), dmsg)) { FAIL() << toString(dctx.error_ctx()); }
#else //!MED_NO_EXCEPTION
		decode(med::make_octet_decoder(dctx), dmsg);
#endif //MED_NO_EXCEPTION
		ASSERT_EQ(msg.count<FLD_DW>(), dmsg.count<FLD_DW>());
		auto it = dmsg.get<FLD_DW>().begin();
		for (auto const& v : msg.get<FLD_DW>())
		{
			EXPECT_EQ(v.get(), it->get());
			it++;
		}
	}
}
#endif

#if 1
//support of 'unlimited' sequence-ofs
//NOTE: declare a message which can't be decoded
struct MSEQ_OPEN : med::sequence<
	M< T<0x21>, FLD_U16, med::min<2>, med::inf >,  //<TV>*[2,*)
	M< L, FLD_U24, med::inf >,                     //<LV>*[1,*)
	M< T<0x42>, L, FLD_IP, med::inf >,             //<TLV>(fixed)*[1,*)
	M< CNT, FLD_W, med::inf >,                     //C<V>*[1,*)
	O< T<0x62>, L, FLD_DW, med::inf >,             //[TLV]*[0,*)
	//O< T<0x80>, CNT, SEQOF_3<1>, med::inf >,     //T[CV]*[0,*)
	O< L, VFLD1, med::inf >                        //[LV(var)]*[0,*) till EoF
>
{
};

//encoding-only and only some fields since different IEs are checked above
TEST(encode, mseq_open)
{
	uint8_t buffer[1024];
	med::encoder_context<> ctx{ buffer };

	MSEQ_OPEN msg;
	static_assert(msg.arity<FLD_U16>() == med::inf(), "");
	static_assert(msg.arity<FLD_U24>() == med::inf(), "");
	static_assert(msg.arity<FLD_IP>() == med::inf(), "");
	static_assert(msg.arity<FLD_W>() == med::inf(), "");

	{
		auto* p = msg.push_back<FLD_U16>();
		ASSERT_NE(nullptr, p);
		p->set(0x35D9);
		p = msg.push_back<FLD_U16>(ctx);
		ASSERT_NE(nullptr, p);
		p->set(0x35DA);
	}
	msg.push_back<FLD_U24>(ctx)->set(0xDABEEF);
	msg.push_back<FLD_U24>(ctx)->set(0x22BEEF);
	msg.push_back<FLD_IP>(ctx)->set(0xFee1ABBA);
	msg.push_back<FLD_W>(ctx)->set(0x7654);
	msg.push_back<FLD_W>(ctx)->set(0x9876);
	msg.push_back<FLD_DW>(ctx)->set(0x01020304);
	msg.push_back<VFLD1>(ctx)->set("test.this");
	msg.push_back<VFLD1>(ctx)->set("test.it");

//	O< T<0x80>, CNT, SEQOF_3<1>, med::inf >,      //T[CV]*[0,*)

#ifdef MED_NO_EXCEPTION
	if (!encode(make_octet_encoder(ctx), msg)) { FAIL() << toString(ctx.error_ctx()); }
#else
	encode(make_octet_encoder(ctx), msg);
#endif
	uint8_t const encoded[] = {
		  0x21, 0x35, 0xD9                //<T=0x21, FLD_U16>
		, 0x21, 0x35, 0xDA
		, 3, 0xDA, 0xBE, 0xEF             //<L, FLD_U24>
		, 3, 0x22, 0xBE, 0xEF
		, 0x42, 4, 0xFE, 0xE1, 0xAB, 0xBA //<T=0x42, L, FLD_IP>
		, 0,2, 0x76, 0x54,  0x98, 0x76    //<C16, FLD_W>
		, 0x62, 4, 0x01, 0x02, 0x03, 0x04    //[T=0x51, L, FLD_DW]
		, 9, 't', 'e', 's', 't', '.', 't', 'h', 'i', 's'
		, 7, 't', 'e', 's', 't', '.', 'i', 't'
	};
	EXPECT_EQ(sizeof(encoded), ctx.buffer().get_offset());
	EXPECT_TRUE(Matches(encoded, buffer));
}

TEST(encode, alloc_fail)
{
	uint8_t buffer[1];
	med::encoder_context<> ctx{ buffer };

	MSEQ_OPEN msg;

	//check the inplace storage for unlimited field has only one slot
	auto* p = msg.push_back<FLD_U24>();
	ASSERT_NE(nullptr, p);
	p->set(1);

#ifdef MED_NO_EXCEPTION
	p = msg.push_back<FLD_U24>();
	ASSERT_EQ(nullptr, p);
	p = msg.push_back<FLD_U24>(ctx);
	ASSERT_EQ(nullptr, p);
#else
	ASSERT_THROW(msg.push_back<FLD_U24>(), med::exception);
	ASSERT_THROW(msg.push_back<FLD_U24>(ctx), med::exception);
#endif
}

TEST(decode, alloc_fail)
{
	uint8_t const encoded[] = {
		  0x21, 0x35, 0xD9                //<T=0x21, FLD_U16>
		, 0x21, 0x35, 0xDA
		, 3, 0xDA, 0xBE, 0xEF             //<L, FLD_U24>
		, 3, 0x22, 0xBE, 0xEF
		, 0x42, 4, 0xFE, 0xE1, 0xAB, 0xBA //<T=0x42, L, FLD_IP>
		, 0,2, 0x76, 0x54,  0x98, 0x76    //<C16, FLD_W>
		, 0x62, 4, 0x01, 0x02, 0x03, 0x04    //[T=0x51, L, FLD_DW]
		, 9, 't', 'e', 's', 't', '.', 't', 'h', 'i', 's'
		, 7, 't', 'e', 's', 't', '.', 'i', 't'
	};

	med::decoder_context<> ctx{ encoded };

	MSEQ_OPEN msg;

#ifdef MED_NO_EXCEPTION
	ASSERT_FALSE(decode(make_octet_decoder(ctx), msg));
	EXPECT_EQ(med::error::OUT_OF_MEMORY, ctx.error_ctx().get_error()) << toString(ctx.error_ctx());
#else //!MED_NO_EXCEPTION
	ASSERT_THROW(decode(make_octet_decoder(ctx), msg), med::exception);
#endif //MED_NO_EXCEPTION
}
#endif

//customized print of container
template <int I>
struct DEEP_SEQ : med::sequence< M<FLD_U8>, M<FLD_U16> >
{
	static constexpr char const* name() { return "DeepSeq"; }
	template <std::size_t N> void print(char (&sz)[N]) const
	{
		std::snprintf(sz, N, "%u#%04X", this->template get<FLD_U8>().get(), this->template get<FLD_U16>().get());
	}
};

template <int I>
struct DEEP_SEQ1 : med::sequence< M<DEEP_SEQ<I>> >
{
	static constexpr char const* name() { return "DeepSeq1"; }
};

template <int I>
struct DEEP_SEQ2 : med::sequence< M<DEEP_SEQ1<I>> >
{
	static constexpr char const* name() { return "DeepSeq2"; }
};

template <int I>
struct DEEP_SEQ3 : med::sequence< M<DEEP_SEQ2<I>> >
{
	static constexpr char const* name() { return "DeepSeq3"; }
};
template <int I>
struct DEEP_SEQ4 : med::sequence< M<DEEP_SEQ3<I>> >
{
	static constexpr char const* name() { return "DeepSeq4"; }
};

struct DEEP_MSG : med::sequence<
	M< DEEP_SEQ<0>  >,
	M< DEEP_SEQ1<0> >,
	M< DEEP_SEQ2<0> >,
	M< DEEP_SEQ3<0> >,
	M< DEEP_SEQ4<0> >,
	M< DEEP_SEQ<1>  >,
	M< DEEP_SEQ1<1> >,
	M< DEEP_SEQ2<1> >,
	M< DEEP_SEQ3<1> >,
	M< DEEP_SEQ4<1> >
>
{
	static constexpr char const* name() { return "DeepMsg"; }
};

TEST(print, container)
{
	uint8_t const encoded[] = {
		1,0,2,
		2,0,3,
		3,0,4,
		5,0,6,
		7,0,8,

		11,0,12,
		12,0,13,
		13,0,14,
		15,0,16,
		17,0,18,
	};

	DEEP_MSG msg;
	med::decoder_context<> ctx;

	ctx.reset(encoded, sizeof(encoded));
#ifdef MED_NO_EXCEPTION
	if (!decode(med::make_octet_decoder(ctx), msg)) { FAIL() << toString(ctx.error_ctx()); }
#else //!MED_NO_EXCEPTION
	decode(med::make_octet_decoder(ctx), msg);
#endif //MED_NO_EXCEPTION

	std::size_t const cont_num[6] = { 21,  1,  9, 15, 19, 21 };
	std::size_t const cust_num[6] = { 10,  0,  2,  4,  6,  8 };

	for (std::size_t level = 0; level < std::extent<decltype(cont_num)>::value; ++level)
	{
		dummy_sink d{0};

		med::print(d, msg, level);
		EXPECT_EQ(cont_num[level], d.num_on_container);
		EXPECT_EQ(cust_num[level], d.num_on_custom);
		EXPECT_EQ(0, d.num_on_value);
	}
}

TEST(print_all, container)
{
	uint8_t const encoded[] = {
		1,0,2,
		2,0,3,
		3,0,4,
		5,0,6,
		7,0,8,

		11,0,12,
		12,0,13,
		13,0,14,
		15,0,16,
		17,0,18,
	};

	DEEP_MSG msg;
	med::decoder_context<> ctx;

	ctx.reset(encoded, sizeof(encoded));
#ifdef MED_NO_EXCEPTION
	if (!decode(med::make_octet_decoder(ctx), msg)) { FAIL() << toString(ctx.error_ctx()); }
#else //!MED_NO_EXCEPTION
	decode(med::make_octet_decoder(ctx), msg);
#endif //MED_NO_EXCEPTION

	dummy_sink d{0};
	med::print_all(d, msg);
	EXPECT_EQ(31, d.num_on_container);
	EXPECT_EQ(20, d.num_on_value);
	ASSERT_EQ(0, d.num_on_custom);
}

struct EncData
{
	std::vector<uint8_t> encoded;
};

std::ostream& operator<<(std::ostream& os, EncData const& param)
{
	return os << param.encoded.size() << " bytes";
}

class PrintUt : public ::testing::TestWithParam<EncData>
{
public:

protected:
	PROTO m_proto;
	med::decoder_context<> m_ctx;

private:
	virtual void SetUp() override
	{
		EncData const& param = GetParam();
		m_ctx.reset(param.encoded.data(), param.encoded.size());
	}
};


TEST_P(PrintUt, levels)
{
#ifdef MED_NO_EXCEPTION
	if (!decode(make_octet_decoder(m_ctx), m_proto)) { FAIL() << toString(m_ctx.error_ctx()); }
#else //!MED_NO_EXCEPTION
	decode(make_octet_decoder(m_ctx), m_proto);
#endif //MED_NO_EXCEPTION

	dummy_sink d{0};
	med::print(d, m_proto, 1);

	std::size_t const num_prints[2][3] = {
#ifdef CODEC_TRACE_ENABLE
		{1, 0, 1},
		{2, 1, 2},
#else
		{1, 0, 0},
		{2, 1, 1},
#endif
	};

	EXPECT_EQ(num_prints[0][0], d.num_on_container);
	EXPECT_EQ(num_prints[0][1], d.num_on_custom);
	EXPECT_EQ(num_prints[0][2], d.num_on_value);

	med::print(d, m_proto, 2);
	EXPECT_EQ(num_prints[1][0], d.num_on_container);
	EXPECT_LE(num_prints[1][1], d.num_on_custom);
	EXPECT_LE(num_prints[1][2], d.num_on_value);
}

TEST_P(PrintUt, incomplete)
{
	EncData const& param = GetParam();
	m_ctx.reset(param.encoded.data(), 2);

#ifdef MED_NO_EXCEPTION
	if (decode(make_octet_decoder(m_ctx), m_proto)) { FAIL() << toString(m_ctx.error_ctx()); }
#else
	try
	{
		decode(make_octet_decoder(m_ctx), m_proto);
		FAIL();
	}
	catch (med::exception const& ex)
	{
		//EXPECT_EQ(med::error::INCORRECT_TAG, ex.error());
	}
#endif

	dummy_sink d{0};
	med::print(d, m_proto);

	std::size_t const num_prints[3] = {
#ifdef CODEC_TRACE_ENABLE
		1, 0, 2,
#else
		1, 0, 1,
#endif
	};

	EXPECT_EQ(num_prints[0], d.num_on_container);
	EXPECT_EQ(num_prints[1], d.num_on_custom);
	EXPECT_GE(num_prints[2], d.num_on_value);
}

EncData const test_prints[] = {
	{
		{ 1
			, 37
			, 0x21, 0x35, 0xD9
			, 3, 0xDA, 0xBE, 0xEF
			, 0x42, 4, 0xFE, 0xE1, 0xAB, 0xBA
			, 0x51, 0x01, 0x02, 0x03, 0x04
			, 0,1, 2, 0x21, 0,3
			, 0x12, 4, 't', 'e', 's', 't', '.', 't', 'h', 'i', 's', '!'
		}
	},
	{
		{ 4
			, 0, 0x22, 9, 't', 'e', 's', 't', '.', 't', 'h', 'i', 's'
			, 0, 0x89, 0xFE, 0xE1, 0xAB, 0xBA
			, 0, 0x49, 3, 0xDA, 0xBE, 0xEF
			, 0, 0x21, 2, 0x35, 0xD9
			, 0, 0x0b, 0x11
		}
	},
	{
		{ 0x14
			, 0, 0x22, 9, 't', 'e', 's', 't', '.', 't', 'h', 'i', 's'
			, 0, 0x22, 7, 't', 'e', 's', 't', '.', 'i', 't'
			, 0, 0x89, 0xFE, 0xE1, 0xAB, 0xBA
			, 0, 0x0b, 0x11
			, 0, 0x49, 3, 0xDA, 0xBE, 0xEF
			, 0, 0x49, 3, 0x22, 0xBE, 0xEF
			, 0, 0x21, 2, 0x35, 0xD9
			, 0, 0x21, 2, 0x35, 0xDA
			, 0, 0x89, 0xAB, 0xBA, 0xC0, 0x01
			, 0, 0x0b, 0x12
			, 0, 0x0c, 0x13
		}
	},
};

INSTANTIATE_TEST_CASE_P(print, PrintUt, ::testing::ValuesIn(test_prints));


int main(int argc, char **argv)
{
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
