#include "ut.hpp"


namespace pad {

struct u8 : med::value<uint8_t> {};
struct u16 : med::value<uint16_t> {};
struct u24 : med::value<med::bytes<3>> {};
struct u32 : med::value<uint32_t> {};

template <class T>
using CASE = M< med::value<med::fixed<T::id, uint8_t>>, T>;

struct flag : med::sequence<
	med::placeholder::_length<>,
	M< u8 >
>
{
	static constexpr uint8_t id = 1;
	using container_t::get;
	uint8_t get() const         { return get<u8>().get(); }
	void set(uint8_t v)         { ref<u8>().set(v); }
};

struct port : med::sequence<
	med::placeholder::_length<>,
	M< u16 >
>
{
	static constexpr uint8_t id = 2;
	using container_t::get;
	uint16_t get() const        { return get<u16>().get(); }
	void set(uint16_t v)        { ref<u16>().set(v); }
};

struct pdu : med::sequence<
	med::placeholder::_length<>,
	M< u24 >
>
{
	static constexpr uint8_t id = 3;
	using container_t::get;
	u24::value_type get() const  { return get<u24>().get(); }
	void set(u24::value_type v)  { ref<u24>().set(v); }
};

struct ip : med::sequence<
	med::placeholder::_length<>,
	M< u32 >
>
{
	static constexpr uint8_t id = 4;
	using container_t::get;
	u32::value_type get() const  { return get<u32>().get(); }
	void set(u32::value_type v)  { ref<u32>().set(v); }
};

struct exclusive : med::choice<
	CASE< flag >,
	CASE< port >,
	CASE< pdu >,
	CASE< ip >
>
{
	using length_type = med::value<uint8_t, med::padding<uint32_t>>;
};

using PL = med::length_t<med::value<uint8_t, med::padding<uint32_t>>>;

struct msg : med::sequence<
	M< PL, u16>,
	M< T<3>, PL, u24>
>
{
};

/* 3GPP TS 24.380
8.2.3.13 Track Info field
0                   1                   2                   3
0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|Track Info     |Track Info     |Queueing       |Participant    |
|field ID value |length value   |Capability     |Type Length    |
|               |               |value          |value          |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                         Participant Type value                |
:                                                               :
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                  Floor Participant Reference 1                |
:                               |                               :
|                  Floor Participant Reference n                |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

The <Track Info field ID> value is a binary value and is set according to table 8.2.3.1-2.
The <Track Info length> value is a binary value and has a value indicating the total length
in octets of the <Queueing Capability> value and one or more <Floor Participant Reference>
value items.

The <Queueing Capability> value is an 8 bit binary value where:
	'0'  the floor participant in the MCPTT client does not support queueing
	'1'  the floor participant in the MCPTT client supports queueing

The <Participant Type Length> value is 8 bit binary value set to the length
of the <Participant Type> value.
The <Participant Type> value is a string. If the length of the <Participant Type>
value is not a multiple of 4 bytes, the <Participant Type> value is padded to a
multiple of 4 bytes. The value of the padding bytes is set to zero.

The <Floor Participant Reference> value is a 32 bit binary value containing
a reference to the floor participant in the non-controlling MCPTT function
of an MCPTT group.
*/

struct que_capability : med::value<uint8_t>
{
	static constexpr char const* name() { return "Queueing Capability"; }
};
struct participant_type : med::ascii_string<med::max<255>>
{
	static constexpr char const* name() { return "Participant Type"; }
};
struct floor_part_ref : med::value<uint32_t>
{
	static constexpr char const* name() { return "Floor Participant Reference"; }
};

struct track_info : med::sequence<
	M<que_capability>,
	M<PL, participant_type>,
	M<floor_part_ref, med::max<63>> //(0xFF-1)/4 = 63
>
{
	static constexpr char const* name() { return "Track Info"; }
};

struct ti_len : med::value<uint8_t>
{
	using dependency_type = participant_type;
	static int dependency(participant_type const& ie)
	{
		//round up to 4 then add 1 byte for length itself
		return ((ie.size() + 3) & ~0x3) + 1;
	}
};

struct floor_prio : med::value<uint16_t>
{
	uint8_t get() const                 { return get_encoded() >> 8; }
	void set(uint8_t v)                 { set_encoded(uint16_t(v) << 8); }
	static constexpr char const* name() { return "Floor Priority"; }
};

struct user_id : med::ascii_string<med::max<255>>
{
	static constexpr char const* name() { return "Used ID"; }
};

using flen = med::value<uint8_t, med::padding<uint32_t, med::offset<2>>>;
struct FloorReq : med::sequence<
	M<T<0>, med::length_t<flen>, floor_prio>,
	O<T<6>, med::length_t<flen>, user_id>,
	M<T<11>, med::length_t<ti_len>, track_info>
>{};

} //end: namespace pad

TEST(padding, traits)
{
	#define PAD_TYPE(T) \
	{                                                              \
		using pt = med::padding<T>;                                \
		static_assert(pt::padding_traits::pad_bits == sizeof(T)*8);\
		static_assert(pt::padding_traits::filler == 0);            \
	}                                                              \
	/* PAD_TYPE */
	PAD_TYPE(uint8_t);
	PAD_TYPE(uint16_t);
	PAD_TYPE(uint32_t);
	PAD_TYPE(uint64_t);
	#undef PAD_TYPE

	#define PAD_BITS(N) \
	{                                                    \
		using pt = med::padding<med::bits<N>>;           \
		static_assert(pt::padding_traits::pad_bits == N);\
		static_assert(pt::padding_traits::filler == 0);   \
	}                                                    \
	/* PAD_BITS */
	PAD_BITS(1);
	PAD_BITS(64);
	#undef PAD_BITS

	#define PAD_BYTES(N) \
	{                                                      \
		using pt = med::padding<med::bytes<N>>;            \
		static_assert(pt::padding_traits::pad_bits == N*8);\
		static_assert(pt::padding_traits::filler == 0);     \
	}                                                      \
	/* PAD_BYTES */
	PAD_BYTES(1);
	PAD_BYTES(8);
	#undef PAD_BYTES
}

TEST(padding, value)
{
	uint8_t buffer[64];
	med::encoder_context<> ctx{ buffer };

	pad::msg msg;

	msg.ref<pad::u16>().set(0x1122);
	msg.ref<pad::u24>().set(0x334455);
	encode(med::octet_encoder{ctx}, msg);

	uint8_t const encoded_1[] = {
		2/*PL*/, 0x11,0x22/*u16*/, 0,0/*padding*/,
		3/*T*/, 3/*PL*/, 0x33,0x44,0x55/*u16*/, 0/*padding*/,
	};
	EXPECT_EQ(sizeof(encoded_1), ctx.buffer().get_offset());
	EXPECT_TRUE(Matches(encoded_1, buffer));

	med::decoder_context<> dctx{ctx.buffer().get_start(), ctx.buffer().get_offset()};
	pad::msg dmsg;
	decode(med::octet_decoder{dctx}, dmsg);
	ASSERT_TRUE(msg == dmsg);
}

TEST(padding, container)
{
	uint8_t buffer[64];
	med::encoder_context<> ctx{ buffer };

	pad::exclusive msg;

	auto check_decode = [&msg](auto const& buf)
	{
		med::decoder_context<> dctx{buf.get_start(), buf.get_offset()};
		pad::exclusive dmsg;
		ASSERT_FALSE(msg == dmsg);
		decode(med::octet_decoder{dctx}, dmsg);
		ASSERT_TRUE(msg == dmsg);
	};

	//-- 1 byte
	ctx.reset();
	msg.ref<pad::flag>().set(0x12);
	encode(med::octet_encoder{ctx}, msg);

	uint8_t const encoded_1[] = { 1, 3, 0x12, 0 };
	EXPECT_EQ(sizeof(encoded_1), ctx.buffer().get_offset());
	EXPECT_TRUE(Matches(encoded_1, buffer));
	check_decode(ctx.buffer());

	//-- 2 bytes
	ctx.reset();
	msg.ref<pad::port>().set(0x1234);
	encode(med::octet_encoder{ctx}, msg);

	uint8_t const encoded_2[] = { 2, 4, 0x12, 0x34 };
	EXPECT_EQ(sizeof(encoded_2), ctx.buffer().get_offset());
	EXPECT_TRUE(Matches(encoded_2, buffer));
	check_decode(ctx.buffer());

	//-- 3 bytes
	ctx.reset();
	msg.ref<pad::pdu>().set(0x123456);
	encode(med::octet_encoder{ctx}, msg);

	uint8_t const encoded_3[] = { 3, 5, 0x12, 0x34, 0x56, 0, 0, 0 };
	EXPECT_EQ(sizeof(encoded_3), ctx.buffer().get_offset());
	EXPECT_TRUE(Matches(encoded_3, buffer));

	//-- 4 bytes
	ctx.reset();
	msg.ref<pad::ip>().set(0x12345678);
	encode(med::octet_encoder{ctx}, msg);

	uint8_t const encoded_4[] = { 4, 6, 0x12, 0x34, 0x56, 0x78, 0, 0 };
	EXPECT_EQ(sizeof(encoded_4), ctx.buffer().get_offset());
	EXPECT_TRUE(Matches(encoded_4, buffer));
	check_decode(ctx.buffer());
}

TEST(padding, dependent)
{
	pad::FloorReq fc;

	fc.ref<pad::floor_prio>().set(0xFF);
	fc.ref<pad::user_id>().set("some");

	auto& ti = fc.ref<pad::track_info>();
	ti.ref<pad::que_capability>().set(3);
	ti.ref<pad::participant_type>().set("some1");
	ti.ref<pad::floor_part_ref>().push_back()->set(0x10203040);
	ti.ref<pad::floor_part_ref>().push_back()->set(0x20304050);
	ti.ref<pad::floor_part_ref>().push_back()->set(0x30405060);


	uint8_t const encoded[] = {
		0, 2, 0xFF, 0, //floor prio
		6, 4, 's','o', //used id + padding
		'm','e', 0, 0,
		11, 13/*TI-len*/, 3/*QC*/, 5/*PT-len*/,
		's','o','m','e','1',0,0,0, /*PT*/
		0x10,0x20,0x30,0x40, /*FPR1*/
		0x20,0x30,0x40,0x50, /*FPR2*/
		0x30,0x40,0x50,0x60, /*FPR3*/
	};

	uint8_t buffer[64];
	med::encoder_context<> ctx{ buffer };
	encode(med::octet_encoder{ctx}, fc);
	EXPECT_EQ(sizeof(encoded), ctx.buffer().get_offset());
	EXPECT_TRUE(Matches(encoded, buffer));

	med::decoder_context<> dctx{ctx.buffer().get_start(), ctx.buffer().get_offset()};
	pad::FloorReq fc2;
	decode(med::octet_decoder{dctx}, fc2);
	ASSERT_TRUE(fc == fc2);
}

TEST(padding, tlv)
{
	struct fld : med::ascii_string<med::max<255>>
	{
		static constexpr char const* name() { return "ASCII"; }
	};

	using len = med::value<uint8_t, med::padding<uint32_t, med::offset<2>>>;
	struct msg : med::sequence<
		M<T<0>, med::length_t<len>, fld>
	>{};

	msg m1;
	m1.ref<fld>().set("ABCD");

	uint8_t const encoded[] = {
		0, 4, 'A','B','C','D', 0, 0,
	};

	uint8_t buffer[64];
	med::encoder_context<> ctx{ buffer };
	encode(med::octet_encoder{ctx}, m1);
	EXPECT_EQ(sizeof(encoded), ctx.buffer().get_offset());
	EXPECT_TRUE(Matches(encoded, buffer));

	med::decoder_context<> dctx{ctx.buffer().get_start(), ctx.buffer().get_offset()};
	msg m2;
	decode(med::octet_decoder{dctx}, m2);
	ASSERT_TRUE(m1 == m2);
}
