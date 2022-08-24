#include "ut.hpp"

namespace cp {

struct var_intern : med::octet_string<med::octets_var_intern<8>, med::min<0>> {};
struct var_extern : med::octet_string<med::octets_var_extern, med::min<0>, med::max<8>> {};
struct fix_intern : med::octet_string<med::octets_fix_intern<8>> {};
struct fix_extern : med::octet_string<med::octets_fix_extern<8>> {};

struct octs : med::sequence<
	O< T<1>, L, var_intern >,
	O< T<2>, L, var_extern >,
	O< T<3>, L, fix_intern >,
	O< T<4>, L, fix_extern >
>{};

struct byte : med::value<uint8_t> {};
struct word : med::value<uint16_t> {};
struct dword : med::value<uint32_t> {};

struct hdr : med::sequence<
	M<byte>,
	M<word>
>
{
	auto get_tag() const    { return get<byte>().get(); }
	void set_tag(uint8_t v) { return ref<byte>().set(v); }
};

struct cho : med::choice<
	M<C<1>, byte>,
	M<C<2>, word>,
	M<C<4>, dword>
>{};
struct hdr_cho : med::choice< hdr
	, M<T<1>, L, var_intern>
	, M<T<2>, L, fix_extern>
>
{
};


struct seq : med::sequence<
	M< T<0xF1>, byte, med::min<2>, med::inf >, //<TV>*[2,*)
	M< T<0xF2>, L, word, med::inf >,           //<TLV>(fixed))
	M< med::counter_t<byte>, dword, med::inf > //C<V>*[1,*)
>{};

struct seq_r : med::sequence<
	M< T<0xE1>, dword, med::min<2>, med::inf >,
	M< T<0xE2>, L, byte, med::min<2>, med::inf >,
	M< med::counter_t<byte>, word, med::min<2>, med::inf >
>{};

} //end: namespace cp

TEST(copy, seq_same)
{
	uint8_t const encoded[] = {
		0xF1, 0x13, //TV
		0xF1, 0x37, //TV
		0xF2, 2, 0xFE, 0xE1, //TLV
		0xF2, 2, 0xAB, 0xBA, //TLV
		1, 0xDE, 0xAD, 0xBE, 0xEF, //CV
	};

	cp::seq src_msg;
	cp::seq dst_msg;


	uint8_t dec_buf[128];
	{
		med::allocator alloc{dec_buf};
		med::decoder_context<med::allocator> ctx{ encoded, &alloc };
		decode(med::octet_decoder{ctx}, src_msg);
		//fails when need allocator but not provided one
		ASSERT_THROW(dst_msg.copy(src_msg), med::exception);
		//succeed with allocator
		dst_msg.copy(src_msg, ctx);
	}

	{
		uint8_t buffer[128];
		med::encoder_context<> ctx{ buffer };

		encode(med::octet_encoder{ctx}, dst_msg);
		EXPECT_EQ(sizeof(encoded), ctx.buffer().get_offset());
		EXPECT_TRUE(Matches(encoded, buffer));
	}
}

TEST(copy, seq_diff)
{
	cp::seq src_msg;
	cp::seq_r dst_msg;
	uint8_t dec_buf[256];

	{
		uint8_t const encoded[] = {
			0xF1, 0x13, //TV(byte)
			0xF1, 0x37, //TV(byte)
			0xF2, 2, 0xFE, 0xE1, //TLV(word)
			0xF2, 2, 0xAB, 0xBA, //TLV(word)
			2, 0xDE, 0xAD, 0xBE, 0xEF, //CV(dword)
			   0xC0, 0x01, 0xCA, 0xFE, //CV(dword)
		};

		med::allocator alloc{dec_buf};
		med::decoder_context<med::allocator> ctx{ encoded, &alloc };
		decode(med::octet_decoder{ctx}, src_msg);
		dst_msg.copy(src_msg, ctx);
	}

	{
//		M< T<0xE1>, dword, med::min<2>, med::inf >,
//		M< T<0xE2>, L, byte, med::min<2>, med::inf >,
//		M< med::counter_t<byte>, word, med::min<2>, med::inf >
		uint8_t const encoded[] = {
			0xE1, 0xDE, 0xAD, 0xBE, 0xEF,
			0xE1, 0xC0, 0x01, 0xCA, 0xFE,
			0xE2, 1, 0x13,
			0xE2, 1, 0x37,
			2, 0xFE, 0xE1,
			   0xAB, 0xBA,
		};

		uint8_t buffer[128];
		med::encoder_context<> ctx{ buffer };

		encode(med::octet_encoder{ctx}, dst_msg);
		EXPECT_EQ(sizeof(encoded), ctx.buffer().get_offset());
		EXPECT_TRUE(Matches(encoded, buffer));
	}
}

TEST(copy, choice)
{
	cp::cho src;
	src.ref<cp::word>().set(0xABBA);

	cp::cho dst;
	dst.ref<cp::byte>().set(1);
	cp::byte const* pb = dst.cselect();
	cp::word const* pw = dst.cselect();
	ASSERT_NE(nullptr, pb);
	ASSERT_EQ(nullptr, pw);
	dst.copy(src);

	pb = dst.cselect();
	pw = dst.cselect();
	ASSERT_EQ(nullptr, pb);
	ASSERT_NE(nullptr, pw);
	EXPECT_EQ(0xABBA, pw->get());

	src.clear();
	EXPECT_FALSE(src.is_set());
	EXPECT_TRUE(dst.is_set());
	src.copy_to(dst);
	EXPECT_TRUE(dst.is_set());
}

TEST(copy, choice_hdr)
{
	cp::hdr_cho src;
	src.header().ref<cp::word>().set(0xDADA);
	uint8_t const data[] = {1,2,3,4};
	src.ref<cp::var_intern>().set(data);


	cp::hdr_cho dst;
	dst.header().ref<cp::word>().set(0xBABA);
	dst.ref<cp::fix_extern>().set(data);
	src.copy_to(dst);

	cp::var_intern const* pv = dst.cselect();
	EXPECT_EQ(0xDADA, dst.header().get<cp::word>().get());
	ASSERT_NE(nullptr, pv);
	//EXPECT_EQ(0xABBA, pw->get());
}
