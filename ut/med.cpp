
#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wgnu-zero-variadic-macro-arguments"
#endif
//#pragma clang diagnostic pop


#include "encode.hpp"
#include "decode.hpp"
#include "update.hpp"
#include "printer.hpp"
#include "encoder_context.hpp"
#include "decoder_context.hpp"
#include "octet_encoder.hpp"
#include "octet_decoder.hpp"

#include "ut.hpp"


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
struct VFLD1 : med::ascii_string<med::min<5>, med::max<10>>//, med::with_snapshot
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
	, med::tag<C<0x00>, FLD_U8>
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
	//M< FLD_TN >,
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
			return ies.template as<FLD_QTY>().get();
		}
	};

	struct setter
	{
		template <class T>
		void operator()(FLD_QTY& num_ips, T const& ies) const
		{
			if (std::size_t const qty = med::field_count(ies.template as<FLD_IP>()))
			{
				CODEC_TRACE("*********** qty = %zu", qty);
				num_ips.set(qty);
			}
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
		template <class T> bool operator()(T const& ies) const
		{
			return ies.template as<FLD_FLAGS>().get() & BITS;
		}
	};

	template <value_type QTY, int DELTA = 0>
	struct counter
	{
		template <class T>
		std::size_t operator()(T const& ies) const
		{
			return std::size_t(((ies.template as<FLD_FLAGS>().get() >> QTY) & QTY_MASK) + DELTA);
		}
	};

	struct setter
	{
		template <class T>
		void operator()(FLD_FLAGS& flags, T const& ies) const
		{
			//hdr.template as<version_flags>
			value_type bits =
				(ies.template as<FLD_U16>().is_set() ? U16 : 0 ) |
				(ies.template as<FLD_U24>().is_set() ? U24 : 0 );

			auto const uc_qty = med::field_count(ies.template as<FLD_UC>());
			auto const u8_qty = med::field_count(ies.template as<FLD_U8>());

			CODEC_TRACE("*********** bits=%02X uc=%zu u8=%zu", bits, uc_qty, u8_qty);

			if (ies.template as<FLD_IP>().is_set())
			{
				bits |= QTY;
			}

			bits |= (uc_qty-1) << UC_QTY;
			bits |= u8_qty << U8_QTY;
			CODEC_TRACE("*********** setting bits = %02X", bits);

			flags.set(bits);
			//ies.template as<FLD_FLAGS>().set(bits);
		}
	};
};

//optional fields with functors
struct MSG_FUNC : med::sequence<
	M< FLD_FLAGS, FLD_FLAGS::setter >,
	M< FLD_UC, med::min<2>, med::max<4>, FLD_FLAGS::counter<FLD_FLAGS::UC_QTY, 1> >,
	O< FLD_QTY, FLD_QTY::setter, FLD_FLAGS::has_bits<FLD_FLAGS::QTY> >,
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

#if (MED_EXCEPTIONS)
	encode(make_octet_encoder(ctx), proto);
#else
	if (!encode(make_octet_encoder(ctx), proto)) { FAIL() << toString(ctx.error_ctx()); }
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

#if (MED_EXCEPTIONS)
	encode(med::make_octet_encoder(ctx), proto);
#else
	if (!encode(med::make_octet_encoder(ctx), proto)) { FAIL() << toString(ctx.error_ctx()); }
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

#if (MED_EXCEPTIONS)
	encode(med::make_octet_encoder(ctx), proto);
#else
	if (!encode(med::make_octet_encoder(ctx), proto)) { FAIL() << toString(ctx.error_ctx()); }
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

#if (MED_EXCEPTIONS)
	try
	{
		encode(med::make_octet_encoder(ctx), proto);
		FAIL();
	}
	catch (med::exception const& ex)
	{
		EXPECT_EQ(med::error::MISSING_IE, ex.error());
	}
#else
	if (encode(med::make_octet_encoder(ctx), proto)) { FAIL() << toString(ctx.error_ctx()); }
	EXPECT_EQ(med::error::MISSING_IE, ctx.error_ctx().get_error()) << toString(ctx.error_ctx());
	EXPECT_NE(nullptr, toString(ctx.error_ctx()));
#endif //MED_EXCEPTIONS
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

#if (MED_EXCEPTIONS)
	encode(med::make_octet_encoder(ctx), proto);
#else
	if (!encode(med::make_octet_encoder(ctx), proto)) { FAIL() << toString(ctx.error_ctx()); }
#endif //MED_EXCEPTIONS

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

#if (MED_EXCEPTIONS)
	encode(med::make_octet_encoder(ctx), proto);
#else
	if (!encode(med::make_octet_encoder(ctx), proto)) { FAIL() << toString(ctx.error_ctx()); }
#endif //MED_EXCEPTIONS
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
		, 0x60, 2, 0, 33 //[T=0x60, L, FLD_CHO]
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
		auto& rdw = cmsg.get<FLD_DW>();
		auto it = rdw.begin();
		ASSERT_NE(rdw.end(), it);
		ASSERT_EQ(0x01020304, it->get());
		++it;
		ASSERT_NE(rdw.end(), it);
		ASSERT_EQ(0x12345678, it->get());
//		FLD_DW const* rf = msg.field(1);
//		ASSERT_EQ(0x12345678, rf->get());
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
#if (MED_EXCEPTIONS)
		try
		{
			encode(med::make_octet_encoder(ctx), proto);
			FAIL();
		}
		catch (med::exception const& ex)
		{
			EXPECT_EQ(med::error::MISSING_IE, ex.error());
		}
#else
		if (encode(med::make_octet_encoder(ctx), proto)) { FAIL() << toString(ctx.error_ctx()); }
		EXPECT_EQ(med::error::MISSING_IE, ctx.error_ctx().get_error()) << toString(ctx.error_ctx());
		EXPECT_NE(nullptr, toString(ctx.error_ctx()));
#endif //MED_EXCEPTIONS
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
#if (MED_EXCEPTIONS)
		try
		{
			encode(med::make_octet_encoder(ctx), proto);
			FAIL();
		}
		catch (med::exception const& ex)
		{
			EXPECT_EQ(med::error::MISSING_IE, ex.error());
		}
#else
		if (encode(med::make_octet_encoder(ctx), proto)) { FAIL() << toString(ctx.error_ctx()); }
		EXPECT_EQ(med::error::MISSING_IE, ctx.error_ctx().get_error()) << toString(ctx.error_ctx());
		EXPECT_NE(nullptr, toString(ctx.error_ctx()));
#endif //MED_EXCEPTIONS
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
#if (MED_EXCEPTIONS)
		try
		{
			encode(med::make_octet_encoder(ctx), proto);
			FAIL();
		}
		catch (med::exception const& ex)
		{
			EXPECT_EQ(med::error::MISSING_IE, ex.error());
		}
#else
		if (encode(med::make_octet_encoder(ctx), proto)) { FAIL() << toString(ctx.error_ctx()); }
		EXPECT_EQ(med::error::MISSING_IE, ctx.error_ctx().get_error()) << toString(ctx.error_ctx());
		EXPECT_NE(nullptr, toString(ctx.error_ctx()));
#endif //MED_EXCEPTIONS
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
#if (MED_EXCEPTIONS)
		try
		{
			encode(med::make_octet_encoder(ctx), proto);
			FAIL();
		}
		catch (med::exception const& ex)
		{
			EXPECT_EQ(med::error::MISSING_IE, ex.error());
		}
#else
		if (encode(med::make_octet_encoder(ctx), proto)) { FAIL() << toString(ctx.error_ctx()); }
		EXPECT_EQ(med::error::MISSING_IE, ctx.error_ctx().get_error()) << toString(ctx.error_ctx());
		EXPECT_NE(nullptr, toString(ctx.error_ctx()));
#endif //MED_EXCEPTIONS
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
#if (MED_EXCEPTIONS)
		try
		{
			encode(med::make_octet_encoder(ctx), proto);
			FAIL();
		}
		catch (med::exception const& ex)
		{
			EXPECT_EQ(med::error::OVERFLOW, ex.error());
		}
#else
		if (encode(med::make_octet_encoder(ctx), proto)) { FAIL() << toString(ctx.error_ctx()); }
		EXPECT_EQ(med::error::OVERFLOW, ctx.error_ctx().get_error()) << toString(ctx.error_ctx());
		EXPECT_NE(nullptr, toString(ctx.error_ctx()));
#endif //MED_EXCEPTIONS
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

#if (MED_EXCEPTIONS)
	encode(med::make_octet_encoder(ctx), proto);
#else
	if (!encode(med::make_octet_encoder(ctx), proto)) { FAIL() << toString(ctx.error_ctx()); }
#endif //MED_EXCEPTIONS

	uint8_t const encoded1[] = { 4
		, 0, 0x0b, 0x11
		, 0, 0x21, 2, 0x35, 0xD9
	};
	EXPECT_EQ(sizeof(encoded1), ctx.buffer().get_offset());
	EXPECT_TRUE(Matches(encoded1, buffer));

	//optional fields
	ctx.reset();
	msg.ref<VFLD1>().set("test.this");

#if (MED_EXCEPTIONS)
	encode(med::make_octet_encoder(ctx), proto);
#else
	if (!encode(med::make_octet_encoder(ctx), proto)) { FAIL() << toString(ctx.error_ctx()); }
#endif

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

#if (MED_EXCEPTIONS)
	encode(med::make_octet_encoder(ctx), proto);
#else
	if (!encode(med::make_octet_encoder(ctx), proto)) { FAIL() << toString(ctx.error_ctx()); }
#endif

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
#if (MED_EXCEPTIONS)
	try
	{
		encode(med::make_octet_encoder(ctx), proto);
		FAIL();
	}
	catch (med::exception const& ex)
	{
		EXPECT_EQ(med::error::MISSING_IE, ex.error());
	}
#else
	if (encode(med::make_octet_encoder(ctx), proto)) { FAIL() << toString(ctx.error_ctx()); }
	EXPECT_EQ(med::error::MISSING_IE, ctx.error_ctx().get_error()) << toString(ctx.error_ctx());
	EXPECT_NE(nullptr, toString(ctx.error_ctx()));
#endif
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

#if (MED_EXCEPTIONS)
	encode(med::make_octet_encoder(ctx), proto);
#else
	if (!encode(med::make_octet_encoder(ctx), proto)) { FAIL() << toString(ctx.error_ctx()); }
#endif

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

#if (MED_EXCEPTIONS)
	encode(med::make_octet_encoder(ctx), proto);
#else
	if (!encode(med::make_octet_encoder(ctx), proto)) { FAIL() << toString(ctx.error_ctx()); }
#endif

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

#if (MED_EXCEPTIONS)
	encode(med::make_octet_encoder(ctx), proto);
#else
	if (!encode(med::make_octet_encoder(ctx), proto)) { FAIL() << toString(ctx.error_ctx()); }
#endif

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
#if (MED_EXCEPTIONS)
	encode(med::make_octet_encoder(ctx), proto);
#else
	if (!encode(med::make_octet_encoder(ctx), proto)) { FAIL() << toString(ctx.error_ctx()); }
#endif

	msg.push_back<FLD_U24>(ctx)->set(0);
	ctx.reset();
#if (MED_EXCEPTIONS)
	try
	{
		encode(med::make_octet_encoder(ctx), proto);
		FAIL();
	}
	catch (med::exception const& ex)
	{
		EXPECT_EQ(med::error::MISSING_IE, ex.error());
	}
#else
	if (encode(med::make_octet_encoder(ctx), proto)) { FAIL() << toString(ctx.error_ctx()); }
	EXPECT_EQ(med::error::MISSING_IE, ctx.error_ctx().get_error()) << toString(ctx.error_ctx());
	EXPECT_NE(nullptr, toString(ctx.error_ctx()));
#endif
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

#if (MED_EXCEPTIONS)
	encode(med::make_octet_encoder(ctx), proto);
#else
	if (!encode(med::make_octet_encoder(ctx), proto)) { FAIL() << toString(ctx.error_ctx()); }
#endif

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

#if (MED_EXCEPTIONS)
	encode(med::make_octet_encoder(ctx), proto);
#else
	if (!encode(med::make_octet_encoder(ctx), proto)) { FAIL() << toString(ctx.error_ctx()); }
#endif

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

#if (MED_EXCEPTIONS)
	encode(med::make_octet_encoder(ctx), proto);
#else
	if (!encode(med::make_octet_encoder(ctx), proto)) { FAIL() << toString(ctx.error_ctx()); }
#endif

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
#if (MED_EXCEPTIONS)
	decode(med::make_octet_decoder(ctx), proto);
#else
	if (!decode(med::make_octet_decoder(ctx), proto)) { FAIL() << toString(ctx.error_ctx()); }
#endif

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
#if (MED_EXCEPTIONS)
	decode(med::make_octet_decoder(ctx), proto);
#else
	if (!decode(med::make_octet_decoder(ctx), proto)) { FAIL() << toString(ctx.error_ctx()); }
#endif

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
#if (MED_EXCEPTIONS)
	decode(med::make_octet_decoder(ctx), proto);
#else
	if (!decode(med::make_octet_decoder(ctx), proto)) { FAIL() << toString(ctx.error_ctx()); }
#endif

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

	//check clear
	{
		auto& r = proto.ref<MSG_SEQ>();
		ASSERT_TRUE(r.is_set());
		r.clear();
		ASSERT_FALSE(r.is_set());
	}
}

TEST(decode, seq_fail)
{
	PROTO proto;

	//wrong choice tag
	uint8_t const wrong_choice[] = { 100, 37, 0x49 };
	med::decoder_context<> ctx{ wrong_choice };
#if (MED_EXCEPTIONS)
	try
	{
		decode(make_octet_decoder(ctx), proto);
		FAIL();
	}
	catch (med::exception const& ex)
	{
		EXPECT_EQ(med::error::INCORRECT_TAG, ex.error());
	}
#else
	if (decode(make_octet_decoder(ctx), proto)) { FAIL() << toString(ctx.error_ctx()); }
	EXPECT_EQ(med::error::INCORRECT_TAG, ctx.error_ctx().get_error()) << toString(ctx.error_ctx());
	EXPECT_NE(nullptr, toString(ctx.error_ctx()));
#endif

	//wrong tag of required field 2
	uint8_t const wrong_tag[] = { 2, 37, 0x49, 0xDA, 0xBE, 0xEF };
	ctx.reset(wrong_tag, sizeof(wrong_tag));
#if (MED_EXCEPTIONS)
	try
	{
		decode(make_octet_decoder(ctx), proto);
		FAIL();
	}
	catch (med::exception const& ex)
	{
		EXPECT_EQ(med::error::INCORRECT_TAG, ex.error());
	}
#else
	if (decode(make_octet_decoder(ctx), proto)) { FAIL() << toString(ctx.error_ctx()); }
	EXPECT_EQ(med::error::INCORRECT_TAG, ctx.error_ctx().get_error()) << toString(ctx.error_ctx());
	EXPECT_NE(nullptr, toString(ctx.error_ctx()));
#endif

	//missing required field
	uint8_t const missing[] = { 1, 37 };
	ctx.reset(missing, sizeof(missing));
#if (MED_EXCEPTIONS)
	try
	{
		decode(make_octet_decoder(ctx), proto);
		FAIL();
	}
	catch (med::exception const& ex)
	{
		EXPECT_EQ(med::error::OVERFLOW, ex.error());
	}
#else
	if (decode(make_octet_decoder(ctx), proto)) { FAIL() << toString(ctx.error_ctx()); }
	EXPECT_EQ(med::error::OVERFLOW, ctx.error_ctx().get_error()) << toString(ctx.error_ctx());
	EXPECT_NE(nullptr, toString(ctx.error_ctx()));
#endif

	//incomplete field
	uint8_t const incomplete[] = { 1, 37, 0x21, 0x35 };
	ctx.reset(incomplete, sizeof(incomplete));
#if (MED_EXCEPTIONS)
	try
	{
		decode(make_octet_decoder(ctx), proto);
		FAIL();
	}
	catch (med::exception const& ex)
	{
		EXPECT_EQ(med::error::OVERFLOW, ex.error());
	}
#else
	if (decode(make_octet_decoder(ctx), proto)) { FAIL() << toString(ctx.error_ctx()); }
	EXPECT_EQ(med::error::OVERFLOW, ctx.error_ctx().get_error()) << toString(ctx.error_ctx());
	EXPECT_NE(nullptr, toString(ctx.error_ctx()));
#endif

	//invalid length of fixed field
	uint8_t const bad_length[] = { 1
		, 37
		, 0x21, 0x35, 0xD9
		, 2, 0xDA, 0xBE, 0xEF
		, 0x42, 4, 0xFE, 0xE1, 0xAB, 0xBA
	};
	ctx.reset(bad_length, sizeof(bad_length));
#if (MED_EXCEPTIONS)
	try
	{
		decode(make_octet_decoder(ctx), proto);
		FAIL();
	}
	catch (med::exception const& ex)
	{
		EXPECT_EQ(med::error::OVERFLOW, ex.error());
	}
#else
	if (decode(make_octet_decoder(ctx), proto)) { FAIL() << toString(ctx.error_ctx()); }
	EXPECT_EQ(med::error::OVERFLOW, ctx.error_ctx().get_error()) << toString(ctx.error_ctx());
	EXPECT_NE(nullptr, toString(ctx.error_ctx()));
#endif

	//invalid length of variable field (shorter)
	uint8_t const bad_var_len_lo[] = { 1
		, 37
		, 0x21, 0x35, 0xD9
		, 3, 0xDA, 0xBE, 0xEF
		, 0x42, 4, 0xFE, 0xE1, 0xAB, 0xBA
		, 0x12, 3, 't', 'e', 's'
	};
	ctx.reset(bad_var_len_lo, sizeof(bad_var_len_lo));
#if (MED_EXCEPTIONS)
	try
	{
		decode(make_octet_decoder(ctx), proto);
		FAIL();
	}
	catch (med::exception const& ex)
	{
		EXPECT_EQ(med::error::INCORRECT_VALUE, ex.error());
	}
#else
	if (decode(make_octet_decoder(ctx), proto)) { FAIL() << toString(ctx.error_ctx()); }
	EXPECT_EQ(med::error::INCORRECT_VALUE, ctx.error_ctx().get_error()) << toString(ctx.error_ctx());
	EXPECT_NE(nullptr, toString(ctx.error_ctx()));
#endif

	//invalid length of variable field (longer)
	uint8_t const bad_var_len_hi[] = { 1
		, 37
		, 0x21, 0x35, 0xD9
		, 3, 0xDA, 0xBE, 0xEF
		, 0x42, 4, 0xFE, 0xE1, 0xAB, 0xBA
		, 0x12, 5, 't', 'e', 's', 't', 'e', 's', 't', 'e', 's', 't', 'e'
	};
	ctx.reset(bad_var_len_hi, sizeof(bad_var_len_hi));
#if (MED_EXCEPTIONS)
	try
	{
		decode(make_octet_decoder(ctx), proto);
		FAIL();
	}
	catch (med::exception const& ex)
	{
		EXPECT_EQ(med::error::INCORRECT_VALUE, ex.error());
	}
#else
	if (decode(make_octet_decoder(ctx), proto)) { FAIL() << toString(ctx.error_ctx()); }
	EXPECT_EQ(med::error::INCORRECT_VALUE, ctx.error_ctx().get_error()) << toString(ctx.error_ctx());
	EXPECT_NE(nullptr, toString(ctx.error_ctx()));
#endif
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
#if (MED_EXCEPTIONS)
	decode(med::make_octet_decoder(ctx), proto);
#else
	if (!decode(med::make_octet_decoder(ctx), proto)) { FAIL() << toString(ctx.error_ctx()); }
#endif

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
	EXPECT_TRUE(msg->get<VFLD1>().empty());

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
		, 0x60, 2, 0, 33 //O< T<0x60>, L, FLD_CHO >
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
#if (MED_EXCEPTIONS)
	decode(med::make_octet_decoder(ctx), proto);
#else
	if (!decode(med::make_octet_decoder(ctx), proto)) { FAIL() << toString(ctx.error_ctx()); }
#endif

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
	auto& rso2 = msg->get<SEQOF_2>();
	auto so2_it = rso2.begin();
	ASSERT_NE(rso2.end(), so2_it);
	EXPECT_EQ(0x1122, so2_it->get<FLD_W>().get());

	pcho = so2_it->field();
	ASSERT_NE(nullptr, pcho);
	FLD_U16 const* pu16 = pcho->select();
	ASSERT_NE(nullptr, pu16);
	EXPECT_EQ(0x3344, pu16->get());

	so2_it++;
	ASSERT_NE(rso2.end(), so2_it);
	EXPECT_EQ(0x3344, so2_it->get<FLD_W>().get());

	pcho = so2_it->field();
	EXPECT_EQ(nullptr, pcho);

	so2_it++;
	ASSERT_EQ(rso2.end(), so2_it);

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
#if (MED_EXCEPTIONS)
	try
	{
		decode(make_octet_decoder(ctx), proto);
		FAIL();
	}
	catch (med::exception const& ex)
	{
		EXPECT_EQ(med::error::MISSING_IE, ex.error());
	}
#else
	if (decode(make_octet_decoder(ctx), proto)) { FAIL() << toString(ctx.error_ctx()); }
	EXPECT_EQ(med::error::MISSING_IE, ctx.error_ctx().get_error()) << toString(ctx.error_ctx());
	EXPECT_NE(nullptr, toString(ctx.error_ctx()));
#endif

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
#if (MED_EXCEPTIONS)
	try
	{
		decode(make_octet_decoder(ctx), proto);
		FAIL();
	}
	catch (med::exception const& ex)
	{
		EXPECT_EQ(med::error::MISSING_IE, ex.error());
	}
#else
	if (decode(make_octet_decoder(ctx), proto)) { FAIL() << toString(ctx.error_ctx()); }
	EXPECT_EQ(med::error::MISSING_IE, ctx.error_ctx().get_error()) << toString(ctx.error_ctx());
	EXPECT_NE(nullptr, toString(ctx.error_ctx()));
#endif

	//incomplete field
	uint8_t const incomplete[] = { 0x11, 37, 38, 0x21, 0x35 };
	ctx.reset(incomplete, sizeof(incomplete));
#if (MED_EXCEPTIONS)
	try
	{
		decode(make_octet_decoder(ctx), proto);
		FAIL();
	}
	catch (med::exception const& ex)
	{
		EXPECT_EQ(med::error::OVERFLOW, ex.error());
	}
#else
	if (decode(make_octet_decoder(ctx), proto)) { FAIL() << toString(ctx.error_ctx()); }
	EXPECT_EQ(med::error::OVERFLOW, ctx.error_ctx().get_error()) << toString(ctx.error_ctx());
	EXPECT_NE(nullptr, toString(ctx.error_ctx()));
#endif

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
#if (MED_EXCEPTIONS)
	try
	{
		decode(make_octet_decoder(ctx), proto);
		FAIL();
	}
	catch (med::exception const& ex)
	{
		EXPECT_EQ(med::error::OVERFLOW, ex.error());
	}
#else
	if (decode(make_octet_decoder(ctx), proto)) { FAIL() << toString(ctx.error_ctx()); }
	EXPECT_EQ(med::error::OVERFLOW, ctx.error_ctx().get_error()) << toString(ctx.error_ctx());
	EXPECT_NE(nullptr, toString(ctx.error_ctx()));
#endif

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
#if (MED_EXCEPTIONS)
	try
	{
		decode(make_octet_decoder(ctx), proto);
		FAIL();
	}
	catch (med::exception const& ex)
	{
		EXPECT_EQ(med::error::INCORRECT_VALUE, ex.error());
	}
#else
	if (decode(make_octet_decoder(ctx), proto)) { FAIL() << toString(ctx.error_ctx()); }
	EXPECT_EQ(med::error::INCORRECT_VALUE, ctx.error_ctx().get_error()) << toString(ctx.error_ctx());
	EXPECT_NE(nullptr, toString(ctx.error_ctx()));
#endif

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
#if (MED_EXCEPTIONS)
	try
	{
		decode(make_octet_decoder(ctx), proto);
		FAIL();
	}
	catch (med::exception const& ex)
	{
		EXPECT_EQ(med::error::OVERFLOW, ex.error());
	}
#else
	if (decode(make_octet_decoder(ctx), proto)) { FAIL() << toString(ctx.error_ctx()); }
	EXPECT_EQ(med::error::OVERFLOW, ctx.error_ctx().get_error()) << toString(ctx.error_ctx());
	EXPECT_NE(nullptr, toString(ctx.error_ctx()));
#endif
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
#if (MED_EXCEPTIONS)
	decode(med::make_octet_decoder(ctx), proto);
#else
	if (!decode(med::make_octet_decoder(ctx), proto)) { FAIL() << toString(ctx.error_ctx()); }
#endif
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
#if (MED_EXCEPTIONS)
	decode(med::make_octet_decoder(ctx), proto);
#else
	if (!decode(med::make_octet_decoder(ctx), proto)) { FAIL() << toString(ctx.error_ctx()); }
#endif
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
#if (MED_EXCEPTIONS)
	decode(med::make_octet_decoder(ctx), proto);
#else
	if (!decode(med::make_octet_decoder(ctx), proto)) { FAIL() << toString(ctx.error_ctx()); }
#endif
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
#if (MED_EXCEPTIONS)
	try
	{
		decode(make_octet_decoder(ctx), proto);
		FAIL();
	}
	catch (med::exception const& ex)
	{
		EXPECT_EQ(med::error::MISSING_IE, ex.error());
	}
#else
	if (decode(make_octet_decoder(ctx), proto)) { FAIL() << toString(ctx.error_ctx()); }
	EXPECT_EQ(med::error::MISSING_IE, ctx.error_ctx().get_error()) << toString(ctx.error_ctx());
	EXPECT_NE(nullptr, toString(ctx.error_ctx()));
#endif

	//extra field in set
	uint8_t const extra_field[] = { 4
		, 0, 0x49, 3, 0xDA, 0xBE, 0xEF
		, 0, 0x49, 3, 0xDA, 0xBE, 0xEF
	};
	ctx.reset(extra_field, sizeof(extra_field));
#if (MED_EXCEPTIONS)
	try
	{
		decode(make_octet_decoder(ctx), proto);
		FAIL();
	}
	catch (med::exception const& ex)
	{
		EXPECT_EQ(med::error::EXTRA_IE, ex.error());
	}
#else
	if (decode(make_octet_decoder(ctx), proto)) { FAIL() << toString(ctx.error_ctx()); }
	EXPECT_EQ(med::error::EXTRA_IE, ctx.error_ctx().get_error()) << toString(ctx.error_ctx());
	EXPECT_NE(nullptr, toString(ctx.error_ctx()));
#endif
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
#if (MED_EXCEPTIONS)
	decode(med::make_octet_decoder(ctx), proto);
#else
	if (!decode(med::make_octet_decoder(ctx), proto)) { FAIL() << toString(ctx.error_ctx()); }
#endif
	MSG_MSET const* msg = proto.select();
	ASSERT_NE(nullptr, msg);

	{
		ASSERT_EQ(2, msg->count<FLD_UC>());
		auto it = msg->get<FLD_UC>().begin();
		EXPECT_EQ(0x11, it->get());
		++it;
		EXPECT_EQ(0x12, it->get());
	}
	{
		ASSERT_EQ(1, msg->count<FLD_U8>());
		EXPECT_EQ(0x13, (msg->get<FLD_U8>().begin()->get()));
	}
	{
		ASSERT_EQ(2, msg->count<FLD_U16>());
		auto it = msg->get<FLD_U16>().begin();
		EXPECT_EQ(0x35D9, it->get());
		++it;
		EXPECT_EQ(0x35DA, it->get());
	}
	EXPECT_TRUE(msg->get<FLD_U24>().empty());
	EXPECT_TRUE(msg->get<FLD_IP>().empty());
	EXPECT_TRUE(msg->get<VFLD1>().empty());

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
#if (MED_EXCEPTIONS)
	decode(med::make_octet_decoder(ctx), proto);
#else
	if (!decode(med::make_octet_decoder(ctx), proto)) { FAIL() << toString(ctx.error_ctx()); }
#endif
	msg = proto.select();
	ASSERT_NE(nullptr, msg);

	EXPECT_TRUE(msg->get<FLD_U24>().empty());
	EXPECT_TRUE(msg->get<FLD_IP>().empty());

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
#if (MED_EXCEPTIONS)
	decode(med::make_octet_decoder(ctx), proto);
#else
	if (!decode(med::make_octet_decoder(ctx), proto)) { FAIL() << toString(ctx.error_ctx()); }
#endif
	msg = proto.select();
	ASSERT_NE(nullptr, msg);

	{
		uint32_t const exp[] = {0xDABEEF, 0x22BEEF};
		auto const* p = exp;
		ASSERT_EQ(std::size(exp), msg->count<FLD_U24>());
		for (auto& f : msg->get<FLD_U24>())
		{
			EXPECT_EQ(*p++, f.get());
		}
	}
	{
		uint32_t const exp[] = {0xfee1ABBA, 0xABBAc001};
		auto const* p = exp;
		ASSERT_EQ(std::size(exp), msg->count<FLD_IP>());
		for (auto& f : msg->get<FLD_IP>())
		{
			EXPECT_EQ(*p++, f.get());
		}
	}
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
#if (MED_EXCEPTIONS)
	try
	{
		decode(make_octet_decoder(ctx), proto);
		FAIL();
	}
	catch (med::exception const& ex)
	{
		EXPECT_EQ(med::error::MISSING_IE, ex.error());
	}
#else
	if (decode(make_octet_decoder(ctx), proto)) { FAIL() << toString(ctx.error_ctx()); }
	EXPECT_EQ(med::error::MISSING_IE, ctx.error_ctx().get_error()) << toString(ctx.error_ctx());
	EXPECT_NE(nullptr, toString(ctx.error_ctx()));
#endif

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
#if (MED_EXCEPTIONS)
	try
	{
		decode(make_octet_decoder(ctx), proto);
		FAIL();
	}
	catch (med::exception const& ex)
	{
		EXPECT_EQ(med::error::EXTRA_IE, ex.error());
	}
#else
	if (decode(make_octet_decoder(ctx), proto)) { FAIL() << toString(ctx.error_ctx()); }
	EXPECT_EQ(med::error::EXTRA_IE, ctx.error_ctx().get_error()) << toString(ctx.error_ctx());
	EXPECT_NE(nullptr, toString(ctx.error_ctx()));
#endif
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
#if (MED_EXCEPTIONS)
	decode(med::make_octet_decoder(ctx), proto);
#else
	if (!decode(med::make_octet_decoder(ctx), proto)) { FAIL() << toString(ctx.error_ctx()); }
#endif

	MSG_FUNC const* msg = proto.select();
	ASSERT_NE(nullptr, msg);
	ASSERT_EQ(2, msg->count<FLD_UC>());
	{
		auto it = msg->get<FLD_UC>().begin();
		EXPECT_EQ(37, it->get());
		EXPECT_EQ(38, std::next(it)->get());
		ASSERT_EQ(0, msg->count<FLD_U8>());
	}
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
#if (MED_EXCEPTIONS)
	decode(med::make_octet_decoder(ctx), proto);
#else
	if (!decode(med::make_octet_decoder(ctx), proto)) { FAIL() << toString(ctx.error_ctx()); }
#endif

	msg = proto.select();
	ASSERT_NE(nullptr, msg);
	ASSERT_EQ(3, msg->count<FLD_UC>());
	{
		auto it = msg->get<FLD_UC>().begin();
		EXPECT_EQ(37, it->get()); ++it;
		EXPECT_EQ(38, it->get()); ++it;
		EXPECT_EQ(39, it->get());
	}
	ASSERT_EQ(0, msg->count<FLD_U8>());
	EXPECT_EQ(msg->get<FLD_U8>().end(), msg->get<FLD_U8>().begin());
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
#if (MED_EXCEPTIONS)
	decode(med::make_octet_decoder(ctx), proto);
#else
	if (!decode(med::make_octet_decoder(ctx), proto)) { FAIL() << toString(ctx.error_ctx()); }
#endif

	msg = proto.select();
	ASSERT_NE(nullptr, msg);
	{
		uint8_t const exp[] = {37,38,39,40};
		ASSERT_EQ(std::size(exp), msg->count<FLD_UC>());
		auto* p = exp;
		for (auto& f : msg->get<FLD_UC>()) { EXPECT_EQ(*p++, f.get()); }
	}
	{
		uint8_t const exp[] = {'a','b'};
		ASSERT_EQ(std::size(exp), msg->count<FLD_U8>());
		auto* p = exp;
		for (auto& f : msg->get<FLD_U8>()) { EXPECT_EQ(*p++, f.get()); }
	}
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
	std::size_t i = 0;
	for (auto& f : msg->get<FLD_IP>())
	{
		EXPECT_EQ(make_hash(i), f.get()); i++;
	}

	//------------ fail cases
	uint8_t const mandatory_underflow[] = { 0xFF
		, 0x07
		, 0x35, 0xD9
		, 0xDA, 0xBE, 0xEF
		, 0xFE, 0xE1, 0xAB, 0xBA
	};
	ctx.reset(mandatory_underflow, sizeof(mandatory_underflow));
#if (MED_EXCEPTIONS)
	try
	{
		decode(make_octet_decoder(ctx), proto);
		FAIL();
	}
	catch (med::exception const& ex)
	{
		EXPECT_EQ(med::error::MISSING_IE, ex.error());
	}
#else
	if (decode(make_octet_decoder(ctx), proto)) { FAIL() << toString(ctx.error_ctx()); }
	EXPECT_EQ(med::error::MISSING_IE, ctx.error_ctx().get_error()) << toString(ctx.error_ctx());
	EXPECT_NE(nullptr, toString(ctx.error_ctx()));
#endif

	uint8_t const conditional_overflow[] = { 0xFF
		, 0xD7
		, 37, 38
		, 'a', 'b', 'c'
		, 0x35, 0xD9
		, 0xDA, 0xBE, 0xEF
		, 0xFE, 0xE1, 0xAB, 0xBA
	};
	ctx.reset(conditional_overflow, sizeof(conditional_overflow));
#if (MED_EXCEPTIONS)
	try
	{
		decode(make_octet_decoder(ctx), proto);
		FAIL();
	}
	catch (med::exception const& ex)
	{
		EXPECT_EQ(med::error::EXTRA_IE, ex.error());
	}
#else
	if (decode(make_octet_decoder(ctx), proto)) { FAIL() << toString(ctx.error_ctx()); }
	EXPECT_EQ(med::error::EXTRA_IE, ctx.error_ctx().get_error()) << toString(ctx.error_ctx());
	EXPECT_NE(nullptr, toString(ctx.error_ctx()));
#endif
}


//TODO: add more isolated UTs on fields
TEST(field, tagged_nibble)
{
	uint8_t buffer[4];
	med::encoder_context<> ctx{ buffer };

	FLD_TN field;
	field.set(5);
#if (MED_EXCEPTIONS)
	encode(make_octet_encoder(ctx), field);
#else
	if (!encode(make_octet_encoder(ctx), field)) { FAIL() << toString(ctx.error_ctx()); }
#endif
	EXPECT_STRCASEEQ("E5 ", as_string(ctx.buffer()));

	decltype(field) dfield;
	med::decoder_context<> dctx;
	dctx.reset(ctx.buffer().get_start(), ctx.buffer().get_offset());
#if (MED_EXCEPTIONS)
	decode(make_octet_decoder(dctx), dfield);
#else
	if (!decode(make_octet_decoder(dctx), dfield)) { FAIL() << toString(ctx.error_ctx()); }
#endif
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

#if (MED_EXCEPTIONS)
		encode(make_octet_encoder(ctx), msg);
#else
		if (!encode(make_octet_encoder(ctx), msg)) { FAIL() << toString(ctx.error_ctx()); }
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
		decode(med::make_octet_decoder(dctx), dmsg);
#else
		if (!decode(med::make_octet_decoder(dctx), dmsg)) { FAIL() << toString(ctx.error_ctx()); }
#endif
		PL_HDR& dhdr = dmsg.ref<PL_HDR>();
		EXPECT_EQ(hdr.get<FLD_U16>().get(), dhdr.get<FLD_U16>().get());
		EXPECT_EQ(hdr.get<FLD_U8>().get(), dhdr.get<FLD_U8>().get());
	}

	ctx.reset();
	{
		static_assert(msg.arity<FLD_DW>() == 2, "");
		msg.push_back<FLD_DW>(ctx)->set(0x01020304);
		msg.push_back<FLD_DW>(ctx)->set(0x05060708);

#if (MED_EXCEPTIONS)
		encode(make_octet_encoder(ctx), msg);
#else
		if (!encode(make_octet_encoder(ctx), msg)) { FAIL() << toString(ctx.error_ctx()); }
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
#if (MED_EXCEPTIONS)
		decode(med::make_octet_decoder(dctx), dmsg);
#else
		if (!decode(med::make_octet_decoder(dctx), dmsg)) { FAIL() << toString(dctx.error_ctx()); }
#endif
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

#if 1 //GTPC-like header for testing

namespace gtpc {

//Bits: 7   6   5   4   3   2   1   0
//     [   Msg Prio  ] [   Spare     ]
// The relative priority of the GTP-C message may take any value between 0 and 15,
// where 0 corresponds to the highest priority and 15 the lowest priority.
struct message_priority : med::value<med::defaults<0, uint8_t>>
{
	static constexpr char const* name()         { return "Message-Priority"; }

	enum : value_type
	{
		MASK = 0b11110000
	};

	uint8_t get() const                         { return (get_encoded() & MASK) >> 4; }
	void set(uint8_t v)                         { set_encoded((v << 4) & MASK); }
};

//Bits: 7   6   5   4   3   2   1   0
//     [ Version ] [P] [T] [MP][Spare]
struct version_flags : med::value<uint8_t>
{
	enum : value_type
	{
		MP = 1u << 2, //Message Priority flag
		T  = 1u << 3, //TEID flag
		P  = 1u << 4, //Piggybacking flag
		MASK_VERSION = 0xE0u, //1110 0000
	};

	uint8_t version() const             { return (get() & MASK_VERSION) >> 5; }

	bool piggyback() const              { return get() & P; }
	void piggyback(bool v)              { set(v ? (get() | P) : (get() & ~P)); }

	static constexpr char const* name() { return "Version-Flags"; }

	struct setter
	{
		template <class HDR>
		void operator()(version_flags& flags, HDR const& hdr) const
		{
			auto bits = flags.get();
			if (hdr.template as<message_priority>().is_set()) { bits |= MP; } else { bits &= ~MP; }
			flags.set(bits);
		}
	};

	struct has_message_priority
	{
		template <class HDR>
		bool operator()(HDR const& hdr) const
		{
			return hdr.template as<version_flags>().get() & version_flags::MP;
		}
	};
};

struct sequence_number : med::value<med::bits<24>>
{
	static constexpr char const* name()         { return "Sequence-Number"; }
};

/*Oct\Bit  8   7   6   5   4   3   2   1
-----------------------------------------
  1       [ Version ] [P] [T] [MP][Spare]
  2       [ Sequence Number (1st octet) ]
  3       [ Sequence Number (2nd octet) ]
  4       [ Sequence Number (3rd octet) ]
  5       [ Msg Priority] [  Spare      ]
----------------------------------------- */
struct header : med::sequence<
	M< version_flags, version_flags::setter >,
	M< sequence_number >,
	O< message_priority, version_flags::has_message_priority >
>
{
	version_flags const& flags() const          { return get<version_flags>(); }
	version_flags& flags()                      { return ref<version_flags>(); }

	sequence_number::value_type sn() const      { return get<sequence_number>().get(); }
	void sn(sequence_number::value_type v)      { ref<sequence_number>().set(v); }

	static constexpr char const* name()         { return "Header"; }

	explicit header(uint8_t ver = 2)
	{
		flags().set(ver << 5);
	}
};

} //end: namespace gtpc

TEST(opt_defs, unset)
{
	uint8_t const encoded[] = {
		0x40,
		1,2,3,
		0
	};

	{
		gtpc::header header;
		header.sn(0x010203);

		uint8_t buffer[1024];
		med::encoder_context<> ctx{ buffer };
#if (MED_EXCEPTIONS)
		encode(med::make_octet_encoder(ctx), header);
#else
		if (!encode(med::make_octet_encoder(ctx), header)) { FAIL() << toString(ctx.error_ctx()); }
#endif
		ASSERT_EQ(sizeof(encoded), ctx.buffer().get_offset());
		ASSERT_TRUE(Matches(encoded, buffer));
	}
	{
		med::decoder_context<> ctx;
		//std::size_t alloc_buf[1024];
		//ctx.get_allocator().reset(alloc_buf);

		gtpc::header header;
		ctx.reset(encoded, sizeof(encoded));
#if (MED_EXCEPTIONS)
		decode(med::make_octet_decoder(ctx), header);
#else
		if (!decode(med::make_octet_decoder(ctx), header)) { FAIL() << toString(ctx.error_ctx()); }
#endif
		gtpc::message_priority const* prio = header.field();
		EXPECT_EQ(nullptr, prio);
		ASSERT_EQ(0x010203, header.sn());
	}
}

TEST(opt_defs, set)
{
	uint8_t const encoded[] = {
		0x44,
		1,2,3,
		0x70
	};

	{
		gtpc::header header;
		header.sn(0x010203);
		header.ref<gtpc::message_priority>().set(7);

		uint8_t buffer[1024];
		med::encoder_context<> ctx{ buffer };
#if (MED_EXCEPTIONS)
		encode(med::make_octet_encoder(ctx), header);
#else
		if (!encode(med::make_octet_encoder(ctx), header)) { FAIL() << toString(ctx.error_ctx()); }
#endif
		ASSERT_EQ(sizeof(encoded), ctx.buffer().get_offset());
		ASSERT_TRUE(Matches(encoded, buffer));
	}
	{
		med::decoder_context<> ctx;
		//std::size_t alloc_buf[1024];
		//ctx.get_allocator().reset(alloc_buf);

		gtpc::header header;
		ctx.reset(encoded, sizeof(encoded));
#if (MED_EXCEPTIONS)
		decode(med::make_octet_decoder(ctx), header);
#else
		if (!decode(med::make_octet_decoder(ctx), header)) { FAIL() << toString(ctx.error_ctx()); }
#endif
		gtpc::message_priority const* prio = header.cfield();
		ASSERT_NE(nullptr, prio);
		EXPECT_EQ(7, prio->get());
		ASSERT_EQ(0x010203, header.sn());
	}
}

#endif


#if 1 // Diameter

namespace diameter {

//to concat message code and request/answer bit
enum msg_code_e : std::size_t
{
	REQUEST = 0x80000000,
	ANSWER  = 0x00000000,
};

struct version : med::value<med::fixed<1, uint8_t>> {};
struct length : med::value<med::bytes<3>> {};

struct cmd_flags : med::value<uint8_t>
{
	enum : value_type
	{
		R = 0x80,
		P = 0x40,
		E = 0x20,
		T = 0x10,
	};

	bool request() const                { return get() & R; }
	void request(bool v)                { set(v ? (get() | R) : (get() & ~R)); }

	bool proxiable() const              { return get() & P; }
	void proxiable(bool v)              { set(v ? (get() | P) : (get() & ~P)); }

	bool error() const                  { return get() & E; }
	void error(bool v)                  { set(v ? (get() | E) : (get() & ~E)); }

	bool retx() const                   { return get() & T; }
	void retx(bool v)                   { set(v ? (get() | T) : (get() & ~T)); }

	static constexpr char const* name() { return "Cmd-Flags"; }
};

struct cmd_code : med::value<med::bytes<3>>
{
	static constexpr char const* name() { return "Cmd-Code"; }
};

struct app_id : med::value<uint32_t>
{
	static constexpr char const* name() { return "App-Id"; }
};

struct hop_by_hop_id : med::value<uint32_t>
{
	static constexpr char const* name() { return "Hop-by-Hop-Id"; }
};

struct end_to_end_id : med::value<uint32_t>
{
	static constexpr char const* name() { return "End-to-End-Id"; }
};

/*0                   1                   2                   3
  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
 +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 |    Version    |                 Message Length                |
 +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 | command flags |                  Command-Code                 |
 +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 |                         Application-ID                        |
 +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 |                      Hop-by-Hop Identifier                    |
 +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 |                      End-to-End Identifier                    |
 +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 |  AVPs ...
 +-+-+-+-+-+-+-+-+-+-+-+-+- */

struct header : med::sequence<
	med::mandatory< version >,
	med::placeholder::_length<0>,
	med::mandatory< cmd_flags >,
	med::mandatory< cmd_code >,
	med::mandatory< app_id >,
	med::mandatory< hop_by_hop_id >,
	med::mandatory< end_to_end_id >
>
{
	std::size_t get_tag() const                 { return get<cmd_code>().get() + (flags().request() ? REQUEST : ANSWER); }
	void set_tag(std::size_t tag)               { ref<cmd_code>().set(tag & 0xFFFFFF); flags().request(tag & REQUEST); }

	cmd_flags const& flags() const              { return get<cmd_flags>(); }
	cmd_flags& flags()                          { return ref<cmd_flags>(); }

	app_id::value_type ap_id() const            { return get<app_id>().get(); }
	void ap_id(app_id::value_type id)           { ref<app_id>().set(id); }

	hop_by_hop_id::value_type hop_id() const    { return get<hop_by_hop_id>().get(); }
	void hop_id(hop_by_hop_id::value_type id)   { ref<hop_by_hop_id>().set(id); }

	end_to_end_id::value_type end_id() const    { return get<end_to_end_id>().get(); }
	void end_id(end_to_end_id::value_type id)   { ref<end_to_end_id>().set(id); }

	static constexpr char const* name()         { return "Header"; }
};

template <class MSG>
using request = med::tag<med::value<med::fixed<REQUEST + MSG::code>>, MSG>;

template <class MSG>
using answer = med::tag<med::value<med::fixed<ANSWER + MSG::code>>, MSG>;

struct avp_code : med::value<uint32_t>
{
	static constexpr char const* name() { return "AVP-Code"; }
};

template <avp_code::value_type CODE>
struct avp_code_fixed : med::value<med::fixed<CODE, uint32_t>>
{
};

struct avp_flags : med::value<uint8_t>
{
	enum : value_type
	{
		V = 0x80, //vendor specific/vendor-id is present
		M = 0x40, //AVP is mandatory
		P = 0x20, //protected
	};

	bool mandatory() const              { return get() & M; }
	void mandatory(bool v)              { set(v ? (get() | M) : (get() & ~M)); }

	bool protect() const                { return get() & P; }
	void protect(bool v)                { set(v ? (get() | P) : (get() & ~P)); }

	static constexpr char const* name() { return "AVP-Flags"; }
};

struct vendor : med::value<uint32_t>
{
	static constexpr char const* name()   { return "Vendor"; }
};

struct has_vendor
{
	template <class HDR>
	bool operator()(HDR const& hdr) const
	{
		return hdr.template as<avp_flags>().get() & avp_flags::V;
	}
};

template <class BODY, class BASE, typename Enable = void>
struct with_length : med::length_t<med::value<med::bytes<3>>> {};

template <class BODY, class BASE>
struct with_length<BODY, BASE, std::enable_if_t<med::is_empty_v<BODY>> >
{
	//to optionally skip unknown AVPs
	auto get_length() const               { return static_cast<BASE const*>(this)->template get<length>().get(); }
	std::size_t get_tag() const           { return static_cast<BASE const*>(this)->template get<avp_code>().get(); }
	void set_tag(std::size_t val)         { static_cast<BASE const*>(this)->template ref<avp_code>().set(val); }
};

/*0                   1                   2                   3
  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
 +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 |                           AVP Code                            |
 +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 |V M P r r r r r|                  AVP Length                   |
 +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 |                        Vendor-ID (opt)                        |
 +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 |    Data ...
 +-+-+-+-+-+-+-+-+                                                 */
template <class BODY = med::empty, avp_code::value_type CODE = 0>
struct avp_header : med::sequence<
	M< std::conditional_t<(0 != CODE), avp_code_fixed<CODE>, avp_code> >,
	M< avp_flags >,
	std::conditional_t<med::is_empty_v<BODY>, M< length >, med::placeholder::_length<0> >,
	O< vendor, has_vendor >,
	M< BODY >
	>
	, with_length<BODY, avp_header<BODY,CODE>>
{
	auto const& flags() const             { return this->template get<avp_flags>(); }
	auto& flags()                         { return this->template ref<avp_flags>(); }
	vendor const* get_vendor() const      { return this->template get<vendor>(); }

	static constexpr char const* name()   { return "AVP-Header"; }
};

template <class T, typename Enable = void>
struct with_padding
{
};

template <class T>
struct with_padding<T, std::enable_if_t<std::is_same<typename T::ie_type, med::IE_OCTET_STRING>::value>>
{
	using padding = med::padding<uint32_t>;
};

template <class AVP, avp_code::value_type CODE, uint8_t FLAGS = 0, vendor::value_type VENDOR = 0>
struct avp : avp_header<AVP,CODE>, med::tag<med::value<med::fixed<CODE>>>, with_padding<AVP>
{
	enum : avp_code::value_type { id = CODE };

	AVP const& body() const                     { return this->template get<AVP>(); }
	AVP& body()                                 { return this->template ref<AVP>(); }
	bool is_set() const                         { return body().is_set(); }

	using avp_header<AVP,CODE>::get;
	decltype(auto) get() const                  { return body().get(); }
	template <typename... ARGS>
	auto set(ARGS&&... args)                    { return body().set(std::forward<ARGS>(args)...); }

	avp()
	{
		if (VENDOR)
		{
			this->template ref<vendor>().set(VENDOR);
			this->template ref<avp_flags>().set(FLAGS | avp_flags::V);
		}
		else
		{
			this->template ref<avp_flags>().set(FLAGS & ~avp_flags::V);
		}
	}
};

struct any_avp : avp_header<med::octet_string<>>
{
	using AVP = med::octet_string<>;
	AVP const& body() const                     { return this->template get<AVP>(); }
	AVP& body()                                 { return this->template ref<AVP>(); }
	bool is_set() const                         { return body().is_set(); }
};

struct unsigned32 : med::value<uint32_t>
{
	static constexpr char const* name() { return "Unsigned32"; }
};

struct result_code : avp<unsigned32, 268, avp_flags::M>
{
	static constexpr char const* name() { return "Result-Code"; }
};

struct origin_host : avp<med::ascii_string<>, 264, avp_flags::M>
{
	static constexpr char const* name() { return "Origin-Host"; }
};

struct origin_realm : avp<med::ascii_string<>, 296, avp_flags::M>
{
	static constexpr char const* name() { return "Origin-Realm"; }
};

struct disconnect_cause : avp<unsigned32, 273, avp_flags::M>
{
	static constexpr char const* name() { return "Disconnect-Cause"; }
};

struct error_message : avp<med::ascii_string<>, 281>
{
	static constexpr char const* name() { return "Error-Message"; }
};

struct failed_avp : avp<any_avp, 279, avp_flags::M>
{
	using length_type = length;
	static constexpr char const* name() { return "Failed-AVP"; }
};

/*
<DPR>  ::= < Diameter Header: 282, REQ >
		 { Origin-Host }
		 { Origin-Realm }
		 { Disconnect-Cause }
*/
struct DPR : med::set< avp_header<>
	, med::mandatory< origin_host >
	, med::mandatory< origin_realm >
	, med::mandatory< disconnect_cause >
>
{
	enum : std::size_t { code = 282 };
	static constexpr char const* name() { return "Disconnect-Peer-Request"; }
};

/*
<DPA>  ::= < Diameter Header: 282 >
		 { Result-Code }
		 { Origin-Host }
		 { Origin-Realm }
		 [ Error-Message ]
	   * [ Failed-AVP ]
*/
struct DPA : med::set< avp_header<>
	, med::mandatory< result_code >
	, med::mandatory< origin_host >
	, med::mandatory< origin_realm >
	, med::optional< error_message >
	, med::optional< failed_avp, med::inf >
>
{
	enum : std::size_t { code = 282 };
	static constexpr char const* name() { return "Disconnect-Peer-Answer"; }
};

//couple of messages from base protocol for testing
struct BASE : med::choice< header
	, request<DPR>
	, answer<DPA>
>
{
	using length_type = length;
};

uint8_t const dpr[] = {
	0x01, 0x00, 0x00, 0x4C, //VER(1), LEN(3)
	0x80, 0x00, 0x01, 0x1A, //R.P.E.T(1), CMD(3) = 282
	0x00, 0x00, 0x00, 0x00, //APP-ID
	0x22, 0x22, 0x22, 0x22, //H2H-ID
	0x55, 0x55, 0x55, 0x55, //E2E-ID

	0x00, 0x00, 0x01, 0x08, //AVP-CODE = 264 OrigHost
	0x40, 0x00, 0x00, 0x11, //V.M.P(1), LEN(3) = 17 + padding
	'O', 'r', 'i', 'g',
	'.', 'H', 'o', 's',
	't',   0,   0,   0,

	0x00, 0x00, 0x01, 0x28, //AVP-CODE = 296 OrigRealm
	0x40, 0x00, 0x00, 0x16, //V.M.P(1), LEN(3) = 22 + padding
	'o', 'r', 'i', 'g',
	'.', 'r', 'e', 'a',
	'l', 'm', '.', 'n',
	'e', 't',   0,   0,

	0x00, 0x00, 0x01, 0x11, //AVP = 273 Disconnect-Cause AVP
	0x40, 0x00, 0x00, 0x0C, //V.M.P(1), LEN(3) = 12
	0x00, 0x00, 0x00, 0x02, //cause = 2
};

uint8_t const dpa[] = {
	0x01, 0x00, 0x00, 76+12+20, //VER(1), LEN(3)
	0x00, 0x00, 0x01, 0x1A, //R.P.E.T(1), CMD(3) = 282
	0x00, 0x00, 0x00, 0x00, //APP-ID
	0x22, 0x22, 0x22, 0x22, //H2H-ID
	0x55, 0x55, 0x55, 0x55, //E2E-ID

	0x00, 0x00, 0x01, 0x0C, //AVP = 268 Result Code
	0x40, 0x00, 0x00, 0x0C, //V.M.P(1), LEN(3) = 12
	0x00, 0x00, 0x0B, 0xBC, //result = 3004

	0x00, 0x00, 0x01, 0x08, //AVP-CODE = 264 OrigHost
	0x40, 0x00, 0x00, 0x11, //V.M.P(1), LEN(3) = 17 + padding
	'O', 'r', 'i', 'g',
	'.', 'H', 'o', 's',
	't',   0,   0,   0,

	//NOTE: this AVP is not expected in DWA
	0x00, 0x00, 0x01, 0x02, //AVP-CODE = 258 Auth-App-Id AVP
	0x40, 0x00, 0x00, 0x0C, //V.M.P(1), LEN(3) = 12
	0x01, 0x00, 0x00, 0x16, //id = Gx

	0x00, 0x00, 0x01, 0x28, //AVP-CODE = 296 OrigRealm
	0x40, 0x00, 0x00, 0x16, //V.M.P(1), LEN(3) = 22 + padding
	'o', 'r', 'i', 'g',
	'.', 'r', 'e', 'a',
	'l', 'm', '.', 'n',
	'e', 't',   0,   0,

	0x00, 0x00, 0x01, 0x17, //AVP-CODE = 279 Failed-AVP (grouped)
	0x40, 0x00, 0x00, 20, //V.M.P(1), LEN(3)
	0x00, 0x00, 0x01, 0x02, //AVP-CODE = 258 Auth-App-Id AVP
	0x40, 0x00, 0x00, 0x0C, //V.M.P(1), LEN(3) = 12
	0x01, 0x00, 0x00, 0x23, //id = S6a

};

struct unexp_handler
{
	//unexpected message
	template <class FUNC, class IE>
	auto operator()(FUNC& func, IE const&, diameter::header const& header) const
	{
		return func(med::error::INCORRECT_TAG, med::name<IE>(), med::get_tag(header));
	}

	//unexpected AVP
	template <class FUNC, class IE>
	auto operator()(FUNC& func, IE const&, diameter::avp_header<> const& header) const
	{
		int const skip = 4 * ((header.get_length() + 3)/4);
		std::printf("UNEXPECTED AVP code = %zu, skipping %d bytes\n", med::get_tag(header), skip);
		return func(med::ADVANCE_STATE{8 * skip});
	}
};

} //end: namespace diameter

TEST(encode, dpr_padding)
{
	diameter::BASE base;

	diameter::DPR& msg = base.select();

	base.header().ap_id(0);
	base.header().hop_id(0x22222222);
	base.header().end_id(0x55555555);

	uint8_t buffer[1024] = {};
	med::encoder_context<> ctx{ buffer };

	msg.ref<diameter::origin_host>().set("Orig.Host");
	msg.ref<diameter::origin_realm>().set("orig.realm.net");
	msg.ref<diameter::disconnect_cause>().set(2);

#if (MED_EXCEPTIONS)
	encode(med::make_octet_encoder(ctx), base);
#else
	if (!encode(med::make_octet_encoder(ctx), base)) { FAIL() << toString(ctx.error_ctx()); }
#endif

	ASSERT_EQ(sizeof(diameter::dpr), ctx.buffer().get_offset());
	ASSERT_TRUE(Matches(diameter::dpr, buffer));
}

TEST(decode, dpr_padding)
{
	diameter::BASE base;

	med::decoder_context<> ctx{ diameter::dpr };
#if (MED_EXCEPTIONS)
	decode(med::make_octet_decoder(ctx), base);
#else
	if (!decode(med::make_octet_decoder(ctx), base)) { FAIL() << toString(ctx.error_ctx()); }
#endif

	ASSERT_EQ(0, base.header().ap_id());
	ASSERT_EQ(0x22222222, base.header().hop_id());
	ASSERT_EQ(0x55555555, base.header().end_id());

	diameter::DPR const* msg = base.cselect();
	ASSERT_NE(nullptr, msg);

	EQ_STRING_M(diameter::origin_host, "Orig.Host");
	EQ_STRING_M(diameter::origin_realm, "orig.realm.net");
	ASSERT_EQ(2, msg->get<diameter::disconnect_cause>().get());
}

TEST(decode, bad_padding)
{
	diameter::BASE base;

	uint8_t const dpr[] = {
			0x01, 0x00, 0x00, 76-3, //VER(1), LEN(3)
			0x80, 0x00, 0x01, 0x1A, //R.P.E.T(1), CMD(3) = 282
			0x00, 0x00, 0x00, 0x00, //APP-ID
			0x22, 0x22, 0x22, 0x22, //H2H-ID
			0x55, 0x55, 0x55, 0x55, //E2E-ID

			0x00, 0x00, 0x01, 0x11, //AVP = 273 Disconnect-Cause AVP
			0x40, 0x00, 0x00, 0x0C, //V.M.P(1), LEN(3) = 12
			0x00, 0x00, 0x00, 0x02, //cause = 2

			0x00, 0x00, 0x01, 0x28, //AVP-CODE = 296 OrigRealm
			0x40, 0x00, 0x00, 22, //V.M.P(1), LEN(3) = 22 + padding
			'o', 'r', 'i', 'g',
			'.', 'r', 'e', 'a',
			'l', 'm', '.', 'n',
			'e', 't',   0,   0,

			0x00, 0x00, 0x01, 0x08, //AVP-CODE = 264 OrigHost
			0x40, 0x00, 0x00, 17, //V.M.P(1), LEN(3) = 17 + padding
			'O', 'r', 'i', 'g',
			'.', 'H', 'o', 's',
			't',   //0,   0,   0,
	};

	med::decoder_context<> ctx{ dpr };

#if (MED_EXCEPTIONS)
	EXPECT_THROW(decode(med::make_octet_decoder(ctx), base), med::exception);
#else
	EXPECT_FALSE(decode(med::make_octet_decoder(ctx), base));
	EXPECT_EQ(med::error::OVERFLOW, ctx.error_ctx().get_error());
#endif
}
TEST(decode, dpa_unexp)
{
	diameter::BASE base;

	med::decoder_context<> ctx{ diameter::dpa };
	diameter::unexp_handler uh{};
#if (MED_EXCEPTIONS)
	decode(med::make_octet_decoder(ctx), base, uh);
#else
	if (!decode(med::make_octet_decoder(ctx), base, uh)) { FAIL() << toString(ctx.error_ctx()); }
#endif

	ASSERT_EQ(0, base.header().ap_id());
	ASSERT_EQ(0x22222222, base.header().hop_id());
	ASSERT_EQ(0x55555555, base.header().end_id());

	diameter::DPA const* msg = base.cselect();
	ASSERT_NE(nullptr, msg);

	ASSERT_EQ(3004, msg->get<diameter::result_code>().get());
	EQ_STRING_M(diameter::origin_host, "Orig.Host");
	EQ_STRING_M(diameter::origin_realm, "orig.realm.net");

	EXPECT_EQ(1, msg->count<diameter::failed_avp>());
}

#endif

#if 1
namespace pad {

template <class T>
using CASE = med::tag<med::value<med::fixed<T::id>>, T>;

struct flag : med::sequence<
	med::placeholder::_length<>,
	M< FLD_U8 >
>
{
	static constexpr uint8_t id = 1;
	using container_t::get;
	uint8_t get() const         { return get<FLD_U8>().get(); }
	void set(uint8_t v)         { ref<FLD_U8>().set(v); }
};

struct port : med::sequence<
	med::placeholder::_length<>,
	M< FLD_U16 >
>
{
	static constexpr uint8_t id = 2;
	using container_t::get;
	uint16_t get() const        { return get<FLD_U16>().get(); }
	void set(uint16_t v)        { ref<FLD_U16>().set(v); }
};

struct pdu : med::sequence<
	med::placeholder::_length<>,
	M< FLD_U24 >
>
{
	static constexpr uint8_t id = 3;
	using container_t::get;
	FLD_U24::value_type get() const  { return get<FLD_U24>().get(); }
	void set(FLD_U24::value_type v)  { ref<FLD_U24>().set(v); }
};

struct ip : med::sequence<
	med::placeholder::_length<>,
	M< FLD_IP >
>
{
	static constexpr uint8_t id = 4;
	using container_t::get;
	FLD_IP::value_type get() const  { return get<FLD_IP>().get(); }
	void set(FLD_IP::value_type v)  { ref<FLD_IP>().set(v); }
};

struct exclusive : med::choice< med::value<uint8_t>
	, CASE< flag >
	, CASE< port >
	, CASE< pdu >
	, CASE< ip >
>
{
	using length_type = med::value<uint8_t>;
	using padding = med::padding<uint32_t>;
	static constexpr char const* name() { return "Header"; }
};

struct inclusive : med::choice< med::value<uint8_t>
	, CASE< flag >
	, CASE< port >
	, CASE< pdu >
	, CASE< ip >
>
{
	using length_type = med::value<uint8_t>;
	using padding = med::padding<uint32_t, true>;
	static constexpr char const* name() { return "Header"; }
};

} //end: namespace pad


TEST(padding, exclusive)
{
	uint8_t buffer[16];
	med::encoder_context<> ctx{ buffer };

	pad::exclusive hdr;

	//-- 1 byte
	ctx.reset();
	hdr.ref<pad::flag>().set(0x12);
#if (MED_EXCEPTIONS)
	encode(make_octet_encoder(ctx), hdr);
#else
	if (!encode(make_octet_encoder(ctx), hdr)) { FAIL() << toString(ctx.error_ctx()); }
#endif
	uint8_t const encoded_1[] = { 1, 3, 0x12, 0 };
	EXPECT_EQ(sizeof(encoded_1), ctx.buffer().get_offset());
	EXPECT_TRUE(Matches(encoded_1, buffer));

	//-- 2 bytes
	ctx.reset();
	hdr.ref<pad::port>().set(0x1234);
#if (MED_EXCEPTIONS)
	encode(make_octet_encoder(ctx), hdr);
#else
	if (!encode(make_octet_encoder(ctx), hdr)) { FAIL() << toString(ctx.error_ctx()); }
#endif
	uint8_t const encoded_2[] = { 2, 4, 0x12, 0x34 };
	EXPECT_EQ(sizeof(encoded_2), ctx.buffer().get_offset());
	EXPECT_TRUE(Matches(encoded_2, buffer));

	//-- 3 bytes
	ctx.reset();
	hdr.ref<pad::pdu>().set(0x123456);
#if (MED_EXCEPTIONS)
	encode(make_octet_encoder(ctx), hdr);
#else
	if (!encode(make_octet_encoder(ctx), hdr)) { FAIL() << toString(ctx.error_ctx()); }
#endif
	uint8_t const encoded_3[] = { 3, 5, 0x12, 0x34, 0x56, 0, 0, 0 };
	EXPECT_EQ(sizeof(encoded_3), ctx.buffer().get_offset());
	EXPECT_TRUE(Matches(encoded_3, buffer));

	//-- 4 bytes
	ctx.reset();
	hdr.ref<pad::ip>().set(0x12345678);
#if (MED_EXCEPTIONS)
	encode(make_octet_encoder(ctx), hdr);
#else
	if (!encode(make_octet_encoder(ctx), hdr)) { FAIL() << toString(ctx.error_ctx()); }
#endif
	uint8_t const encoded_4[] = { 4, 6, 0x12, 0x34, 0x56, 0x78, 0, 0 };
	EXPECT_EQ(sizeof(encoded_4), ctx.buffer().get_offset());
	EXPECT_TRUE(Matches(encoded_4, buffer));
}

TEST(padding, inclusive)
{
	uint8_t buffer[16];
	med::encoder_context<> ctx{ buffer };

	pad::inclusive hdr;

	//-- 1 byte
	ctx.reset();
	hdr.ref<pad::flag>().set(0x12);
#if (MED_EXCEPTIONS)
	encode(make_octet_encoder(ctx), hdr);
#else
	if (!encode(make_octet_encoder(ctx), hdr)) { FAIL() << toString(ctx.error_ctx()); }
#endif
	uint8_t const encoded_1[] = { 1, 4, 0x12, 0 };
	EXPECT_EQ(sizeof(encoded_1), ctx.buffer().get_offset());
	EXPECT_TRUE(Matches(encoded_1, buffer));

	//-- 2 bytes
	ctx.reset();
	hdr.ref<pad::port>().set(0x1234);
#if (MED_EXCEPTIONS)
	encode(make_octet_encoder(ctx), hdr);
#else
	if (!encode(make_octet_encoder(ctx), hdr)) { FAIL() << toString(ctx.error_ctx()); }
#endif
	uint8_t const encoded_2[] = { 2, 4, 0x12, 0x34 };
	EXPECT_EQ(sizeof(encoded_2), ctx.buffer().get_offset());
	EXPECT_TRUE(Matches(encoded_2, buffer));

	//-- 3 bytes
	ctx.reset();
	hdr.ref<pad::pdu>().set(0x123456);
#if (MED_EXCEPTIONS)
	encode(make_octet_encoder(ctx), hdr);
#else
	if (!encode(make_octet_encoder(ctx), hdr)) { FAIL() << toString(ctx.error_ctx()); }
#endif
	uint8_t const encoded_3[] = { 3, 8, 0x12, 0x34, 0x56, 0, 0, 0 };
	EXPECT_EQ(sizeof(encoded_3), ctx.buffer().get_offset());
	EXPECT_TRUE(Matches(encoded_3, buffer));

	//-- 4 bytes
	ctx.reset();
	hdr.ref<pad::ip>().set(0x12345678);
#if (MED_EXCEPTIONS)
	encode(make_octet_encoder(ctx), hdr);
#else
	if (!encode(make_octet_encoder(ctx), hdr)) { FAIL() << toString(ctx.error_ctx()); }
#endif
	uint8_t const encoded_4[] = { 4, 8, 0x12, 0x34, 0x56, 0x78, 0, 0 };
	EXPECT_EQ(sizeof(encoded_4), ctx.buffer().get_offset());
	EXPECT_TRUE(Matches(encoded_4, buffer));
}
#endif

#if 1
//support of 'unlimited' sequence-ofs
//NOTE: declare a message which can't be decoded
struct MSEQ_OPEN : med::sequence<
	M< T<0x21>, FLD_U16, med::min<2>, med::inf >,  //<TV>*[2,*)
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
	msg.push_back<FLD_IP>(ctx)->set(0xFee1ABBA);
	msg.push_back<FLD_W>(ctx)->set(0x7654);
	msg.push_back<FLD_W>(ctx)->set(0x9876);
	msg.push_back<FLD_DW>(ctx)->set(0x01020304);
	msg.push_back<VFLD1>(ctx)->set("test.this");
	msg.push_back<VFLD1>(ctx)->set("test.it");

//	O< T<0x80>, CNT, SEQOF_3<1>, med::inf >,      //T[CV]*[0,*)

#if (MED_EXCEPTIONS)
	encode(make_octet_encoder(ctx), msg);
#else
	if (!encode(make_octet_encoder(ctx), msg)) { FAIL() << toString(ctx.error_ctx()); }
#endif
	uint8_t const encoded[] = {
		  0x21, 0x35, 0xD9                //<T=0x21, FLD_U16>
		, 0x21, 0x35, 0xDA
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
	auto* p = msg.push_back<FLD_IP>();
	ASSERT_NE(nullptr, p);
	p->set(1);

#if (MED_EXCEPTIONS)
	ASSERT_THROW(msg.push_back<FLD_IP>(), med::exception);
	ASSERT_THROW(msg.push_back<FLD_IP>(ctx), med::exception);
#else
	p = msg.push_back<FLD_IP>();
	ASSERT_EQ(nullptr, p);
	p = msg.push_back<FLD_IP>(ctx);
	ASSERT_EQ(nullptr, p);
#endif
}

TEST(decode, alloc_fail)
{
	uint8_t const encoded[] = {
		  0x21, 0x35, 0xD9                //<T=0x21, FLD_U16>
		, 0x21, 0x35, 0xDA
		, 0x42, 4, 0xFE, 0xE1, 0xAB, 0xBA //<T=0x42, L, FLD_IP>
		, 0,2, 0x76, 0x54,  0x98, 0x76    //<C16, FLD_W>
		, 0x62, 4, 0x01, 0x02, 0x03, 0x04 //[T=0x51, L, FLD_DW]
		, 9, 't', 'e', 's', 't', '.', 't', 'h', 'i', 's'
		, 7, 't', 'e', 's', 't', '.', 'i', 't'
	};

	med::decoder_context<> ctx{ encoded };

	MSEQ_OPEN msg;

#if (MED_EXCEPTIONS)
	ASSERT_THROW(decode(make_octet_decoder(ctx), msg), med::exception);
#else
	ASSERT_FALSE(decode(make_octet_decoder(ctx), msg));
	EXPECT_EQ(med::error::OUT_OF_MEMORY, ctx.error_ctx().get_error()) <<
		[&]()
		{
//			dummy_sink d{2};
//			med::print(d, msg);
			if (auto* err = toString(ctx.error_ctx()))
			{
				return err;
			}
			else
			{
				return "NO ERRORS";
			}
		}();
	EXPECT_NE(nullptr, toString(ctx.error_ctx()));
#endif
}

TEST(copy, mseq_open)
{
	uint8_t const encoded[] = {
		  0x21, 0x35, 0xD9                //<T=0x21, FLD_U16>
		, 0x21, 0x35, 0xDA
		, 0x42, 4, 0xFE, 0xE1, 0xAB, 0xBA //<T=0x42, L, FLD_IP>
		, 0,2, 0x76, 0x54,  0x98, 0x76    //<C16, FLD_W>
		, 0x62, 4, 0x01, 0x02, 0x03, 0x04 //[T=0x51, L, FLD_DW]
		, 9, 't', 'e', 's', 't', '.', 't', 'h', 'i', 's'
		, 7, 't', 'e', 's', 't', '.', 'i', 't'
	};

	MSEQ_OPEN src_msg;
	MSEQ_OPEN dst_msg;

	uint8_t dec_buf[1024];
	{
		med::decoder_context<> ctx{ encoded, dec_buf };
#if (MED_EXCEPTIONS)
		decode(make_octet_decoder(ctx), src_msg);
		ASSERT_THROW(dst_msg.copy(src_msg), med::exception);
		dst_msg.copy(src_msg, ctx);
#else
		if (!decode(make_octet_decoder(ctx), src_msg)) { FAIL() << toString(ctx.error_ctx()); }
		ASSERT_FALSE(dst_msg.copy(src_msg));
		ASSERT_TRUE(dst_msg.copy(src_msg, ctx));
#endif
	}

	{
		uint8_t buffer[1024];
		med::encoder_context<> ctx{ buffer };

#if (MED_EXCEPTIONS)
		encode(make_octet_encoder(ctx), dst_msg);
#else
		if (!encode(make_octet_encoder(ctx), dst_msg)) { FAIL() << toString(ctx.error_ctx()); }
#endif
		EXPECT_EQ(sizeof(encoded), ctx.buffer().get_offset());
		EXPECT_TRUE(Matches(encoded, buffer));
	}
}

#endif

#if 1
//setter with length calc
struct SHDR : med::value<uint8_t>
{
	template <class FLD>
	struct setter
	{
		template <class T>
		void operator()(SHDR& shdr, T const& ies) const
		{
			auto const qty = med::field_count(ies.template as<FLD>());
			shdr.set(qty);
		}
	};

};
struct SFLD : med::sequence<
	M<SHDR, SHDR::setter<FLD_U16>>,
	M<FLD_U16>
>{};
struct SMFLD : med::sequence<
	M<SHDR, SHDR::setter<FLD_U8>>,
	M<FLD_U8, med::max<7>>
>{};
struct SLEN : med::sequence<
	M<L, SFLD>,
	M<L, SMFLD>
>
{
};

TEST(encode, setter_with_length)
{
	SLEN msg;
	msg.ref<SFLD>().ref<FLD_U16>().set(0x55AA);
	for (std::size_t i = 0; i < 7; ++i)
	{
		msg.ref<SMFLD>().push_back<FLD_U8>()->set(i);
	}

	uint8_t buffer[1024];
	med::encoder_context<> ctx{ buffer };

#if (MED_EXCEPTIONS)
	encode(make_octet_encoder(ctx), msg);
#else
	if (!encode(make_octet_encoder(ctx), msg)) { FAIL() << toString(ctx.error_ctx()); }
#endif
	uint8_t const encoded[] = {
		3, 1, 0x55, 0xAA,
		8, 7, 0,1,2,3,4,5,6
	};
	EXPECT_EQ(sizeof(encoded), ctx.buffer().get_offset());
	EXPECT_TRUE(Matches(encoded, buffer));
}
#endif

#if 1
//update
struct UFLD : med::value<uint32_t>, med::with_snapshot
{
	static constexpr char const* name() { return "Updatalbe-Field"; }
};
struct UMSG : med::sequence<
	M<T<7>, L, UFLD>
>
{
};

TEST(update, fixed)
{
	UMSG msg;
	auto& ufld = msg.ref<UFLD>();
	ufld.set(0x12345678);

	uint8_t buffer[1024];
	med::encoder_context<> ctx{ buffer };
	auto encoder = make_octet_encoder(ctx);

#if (MED_EXCEPTIONS)
	encode(encoder, msg);
#else
	if (!encode(encoder, msg)) { FAIL() << toString(ctx.error_ctx()); }
#endif
	uint8_t const encoded[] = {7, 4, 0x12,0x34,0x56,0x78};
	EXPECT_EQ(sizeof(encoded), ctx.buffer().get_offset());
	EXPECT_TRUE(Matches(encoded, buffer));

	ufld.set(0x3456789A);
#if (MED_EXCEPTIONS)
	med::update(encoder, ufld);
#else
	if (!med::update(encoder, ufld)) { FAIL() << toString(ctx.error_ctx()); }
#endif
	uint8_t const updated[] = {7, 4, 0x34,0x56,0x78,0x9A};
	EXPECT_TRUE(Matches(updated, buffer));
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
#if (MED_EXCEPTIONS)
	decode(med::make_octet_decoder(ctx), msg);
#else
	if (!decode(med::make_octet_decoder(ctx), msg)) { FAIL() << toString(ctx.error_ctx()); }
#endif

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
#if (MED_EXCEPTIONS)
	decode(med::make_octet_decoder(ctx), msg);
#else
	if (!decode(med::make_octet_decoder(ctx), msg)) { FAIL() << toString(ctx.error_ctx()); }
#endif

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
#if (MED_EXCEPTIONS)
	decode(make_octet_decoder(m_ctx), m_proto);
#else
	if (!decode(make_octet_decoder(m_ctx), m_proto)) { FAIL() << toString(m_ctx.error_ctx()); }
#endif

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

#if (MED_EXCEPTIONS)
	try
	{
		decode(make_octet_decoder(m_ctx), m_proto);
		FAIL();
	}
	catch (med::exception const& ex)
	{
		//EXPECT_EQ(med::error::INCORRECT_TAG, ex.error());
	}
#else
	if (decode(make_octet_decoder(m_ctx), m_proto)) { FAIL() << toString(m_ctx.error_ctx()); }
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
