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
	encode(med::octet_encoder{ctx}, hdr);

	uint8_t const encoded_1[] = { 1, 3, 0x12, 0 };
	EXPECT_EQ(sizeof(encoded_1), ctx.buffer().get_offset());
	EXPECT_TRUE(Matches(encoded_1, buffer));

	//-- 2 bytes
	ctx.reset();
	hdr.ref<pad::port>().set(0x1234);
	encode(med::octet_encoder{ctx}, hdr);

	uint8_t const encoded_2[] = { 2, 4, 0x12, 0x34 };
	EXPECT_EQ(sizeof(encoded_2), ctx.buffer().get_offset());
	EXPECT_TRUE(Matches(encoded_2, buffer));

	//-- 3 bytes
	ctx.reset();
	hdr.ref<pad::pdu>().set(0x123456);
	encode(med::octet_encoder{ctx}, hdr);

	uint8_t const encoded_3[] = { 3, 5, 0x12, 0x34, 0x56, 0, 0, 0 };
	EXPECT_EQ(sizeof(encoded_3), ctx.buffer().get_offset());
	EXPECT_TRUE(Matches(encoded_3, buffer));

	//-- 4 bytes
	ctx.reset();
	hdr.ref<pad::ip>().set(0x12345678);
	encode(med::octet_encoder{ctx}, hdr);

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
	encode(med::octet_encoder{ctx}, hdr);

	uint8_t const encoded_1[] = { 1, 4, 0x12, 0 };
	EXPECT_EQ(sizeof(encoded_1), ctx.buffer().get_offset());
	EXPECT_TRUE(Matches(encoded_1, buffer));

	//-- 2 bytes
	ctx.reset();
	hdr.ref<pad::port>().set(0x1234);
	encode(med::octet_encoder{ctx}, hdr);

	uint8_t const encoded_2[] = { 2, 4, 0x12, 0x34 };
	EXPECT_EQ(sizeof(encoded_2), ctx.buffer().get_offset());
	EXPECT_TRUE(Matches(encoded_2, buffer));

	//-- 3 bytes
	ctx.reset();
	hdr.ref<pad::pdu>().set(0x123456);
	encode(med::octet_encoder{ctx}, hdr);

	uint8_t const encoded_3[] = { 3, 8, 0x12, 0x34, 0x56, 0, 0, 0 };
	EXPECT_EQ(sizeof(encoded_3), ctx.buffer().get_offset());
	EXPECT_TRUE(Matches(encoded_3, buffer));

	//-- 4 bytes
	ctx.reset();
	hdr.ref<pad::ip>().set(0x12345678);
	encode(med::octet_encoder{ctx}, hdr);

	uint8_t const encoded_4[] = { 4, 8, 0x12, 0x34, 0x56, 0x78, 0, 0 };
	EXPECT_EQ(sizeof(encoded_4), ctx.buffer().get_offset());
	EXPECT_TRUE(Matches(encoded_4, buffer));
}



