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
template <std::size_t TAG> using T = med::value<med::fixed<TAG, uint8_t>>;

struct msg : med::sequence<
	M< PL, u16>,
	M< T<3>, PL, u24>
>
{
};

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
		ASSERT_TRUE(msg != dmsg);
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

