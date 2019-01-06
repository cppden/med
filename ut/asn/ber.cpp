#include "../ut.hpp"

#include "asn/ids.hpp"
#include "asn/asn.hpp"
#include "asn/ber/length.hpp"
#include "asn/ber/encoder.hpp"
#include "asn/ber/decoder.hpp"

using namespace std::literals;

TEST(asn_ber, length)
{
	static_assert(med::asn::ber::length::bits<char>(0b0000'0000) == 1);
	static_assert(med::asn::ber::length::bits<char>(0b0000'0001) == 2);
	static_assert(med::asn::ber::length::bits<char>(0b0000'0010) == 3);
	static_assert(med::asn::ber::length::bits<char>(0b0000'0100) == 4);
	static_assert(med::asn::ber::length::bits<char>(0b0000'1000) == 5);
	static_assert(med::asn::ber::length::bits<char>(0b0000'1000) == 5);
	static_assert(med::asn::ber::length::bits<char>(0b0001'0000) == 6);
	static_assert(med::asn::ber::length::bits<char>(0b0010'0000) == 7);
	static_assert(med::asn::ber::length::bits<char>(0b0100'0000) == 8);
	static_assert(med::asn::ber::length::bits<char>(-1) == 1);
	static_assert(med::asn::ber::length::bits<char>(-2) == 2);
	static_assert(med::asn::ber::length::bits<char>(-3) == 3);
	static_assert(med::asn::ber::length::bits<char>(-5) == 4);
	static_assert(med::asn::ber::length::bits<char>(-9) == 5);
	static_assert(med::asn::ber::length::bits<char>(-17) == 6);
	static_assert(med::asn::ber::length::bits<char>(-33) == 7);
	static_assert(med::asn::ber::length::bits<char>(-65) == 8);

	static_assert(med::asn::ber::length::bits<short>(0b0000'0000'0000'0000) == 1);
	static_assert(med::asn::ber::length::bits<short>(0b0000'0000'0000'0001) == 2);
	static_assert(med::asn::ber::length::bits<short>(0b0000'0000'0000'0010) == 3);
	static_assert(med::asn::ber::length::bits<short>(0b0000'0000'0000'0100) == 4);
	static_assert(med::asn::ber::length::bits<short>(0b0000'0000'0000'1000) == 5);
	static_assert(med::asn::ber::length::bits<short>(0b0000'0000'0000'1000) == 5);
	static_assert(med::asn::ber::length::bits<short>(0b0000'0000'0001'0000) == 6);
	static_assert(med::asn::ber::length::bits<short>(0b0000'0000'0010'0000) == 7);
	static_assert(med::asn::ber::length::bits<short>(0b0000'0000'0100'0000) == 8);
	static_assert(med::asn::ber::length::bits<short>(0b0000'0000'1000'0000) == 9);
	static_assert(med::asn::ber::length::bits<short>(0b0000'0001'0000'0000) == 10);
	static_assert(med::asn::ber::length::bits<short>(0b0000'0010'0000'0000) == 11);
	static_assert(med::asn::ber::length::bits<short>(0b0000'0100'0000'0000) == 12);
	static_assert(med::asn::ber::length::bits<short>(0b0000'1000'0000'0000) == 13);
	static_assert(med::asn::ber::length::bits<short>(0b0001'0000'0000'0000) == 14);
	static_assert(med::asn::ber::length::bits<short>(0b0010'0000'0000'0000) == 15);
	static_assert(med::asn::ber::length::bits<short>(0b0100'0000'0000'0000) == 16);
	static_assert(med::asn::ber::length::bits<short>(-1) == 1);
	static_assert(med::asn::ber::length::bits<short>(-2) == 2);
	static_assert(med::asn::ber::length::bits<short>(-3) == 3);
	static_assert(med::asn::ber::length::bits<short>(-5) == 4);
	static_assert(med::asn::ber::length::bits<short>(-9) == 5);
	static_assert(med::asn::ber::length::bits<short>(-17) == 6);
	static_assert(med::asn::ber::length::bits<short>(-33) == 7);
	static_assert(med::asn::ber::length::bits<short>(-65) == 8);
	static_assert(med::asn::ber::length::bits<short>(-129) == 9);
	static_assert(med::asn::ber::length::bits<short>(-257) == 10);
	static_assert(med::asn::ber::length::bits<short>(-513) == 11);
	static_assert(med::asn::ber::length::bits<short>(-1025) == 12);
	static_assert(med::asn::ber::length::bits<short>(-2049) == 13);
	static_assert(med::asn::ber::length::bits<short>(-4097) == 14);
	static_assert(med::asn::ber::length::bits<short>(-8193) == 15);
	static_assert(med::asn::ber::length::bits<short>(-16385) == 16);

	static_assert(med::asn::ber::length::bits<int>(0b0000'0000'0000'0000) == 1);
	static_assert(med::asn::ber::length::bits<int>(0b0000'0000'0000'0001) == 2);
	static_assert(med::asn::ber::length::bits<int>(0b0000'0000'0000'0010) == 3);
	static_assert(med::asn::ber::length::bits<int>(0b0000'0000'0000'0100) == 4);
	static_assert(med::asn::ber::length::bits<int>(0b0000'0000'0000'1000) == 5);
	static_assert(med::asn::ber::length::bits<int>(0b0000'0000'0000'1000) == 5);
	static_assert(med::asn::ber::length::bits<int>(0b0000'0000'0001'0000) == 6);
	static_assert(med::asn::ber::length::bits<int>(0b0000'0000'0010'0000) == 7);
	static_assert(med::asn::ber::length::bits<int>(0b0000'0000'0100'0000) == 8);
	static_assert(med::asn::ber::length::bits<int>(-1) == 1);
	static_assert(med::asn::ber::length::bits<int>(-2) == 2);
	static_assert(med::asn::ber::length::bits<int>(-3) == 3);
	static_assert(med::asn::ber::length::bits<int>(-5) == 4);
	static_assert(med::asn::ber::length::bits<int>(-9) == 5);
	static_assert(med::asn::ber::length::bits<int>(-17) == 6);
	static_assert(med::asn::ber::length::bits<int>(-33) == 7);
	static_assert(med::asn::ber::length::bits<int>(-65) == 8);

	static_assert(med::asn::ber::length::bits<long>(0b0000'0000'0000'0000) == 1);
	static_assert(med::asn::ber::length::bits<long>(0b0000'0000'0000'0001) == 2);
	static_assert(med::asn::ber::length::bits<long>(0b0000'0000'0000'0010) == 3);
	static_assert(med::asn::ber::length::bits<long>(0b0000'0000'0000'0100) == 4);
	static_assert(med::asn::ber::length::bits<long>(0b0000'0000'0000'1000) == 5);
	static_assert(med::asn::ber::length::bits<long>(0b0000'0000'0000'1000) == 5);
	static_assert(med::asn::ber::length::bits<long>(0b0000'0000'0001'0000) == 6);
	static_assert(med::asn::ber::length::bits<long>(0b0000'0000'0010'0000) == 7);
	static_assert(med::asn::ber::length::bits<long>(0b0000'0000'0100'0000) == 8);
	static_assert(med::asn::ber::length::bits<long>(-1) == 1);
	static_assert(med::asn::ber::length::bits<long>(-2) == 2);
	static_assert(med::asn::ber::length::bits<long>(-3) == 3);
	static_assert(med::asn::ber::length::bits<long>(-5) == 4);
	static_assert(med::asn::ber::length::bits<long>(-9) == 5);
	static_assert(med::asn::ber::length::bits<long>(-17) == 6);
	static_assert(med::asn::ber::length::bits<long>(-33) == 7);
	static_assert(med::asn::ber::length::bits<long>(-65) == 8);
}

TEST(asn_ber, identifier)
{
	using tv1 = med::asn::ber::tag_value<med::asn::traits<1>, false>;
	static_assert(tv1::value() == 0b00000001);
	using tv30 = med::asn::ber::tag_value<med::asn::traits<30, med::asn::tag_class::PRIVATE>, true>;
	static_assert(tv30::value() == 0b11111110);
	using tv5bit = med::asn::ber::tag_value<med::asn::traits<0b11111>, false>;
	static_assert(tv5bit::value() == 0b00011111'00011111);
	using tv7bit = med::asn::ber::tag_value<med::asn::traits<0b1111111>, false>;
	static_assert(tv7bit::value() == 0b00011111'01111111);
	using tv8bit = med::asn::ber::tag_value<med::asn::traits<0b11111111>, false>;
	static_assert(tv8bit::value() == 0b00011111'10000001'01111111);
	using tv14bit = med::asn::ber::tag_value<med::asn::traits<0b111111'11111111>, false>;
	static_assert(tv14bit::value() == 0b00011111'11111111'01111111);
	using tv16bit = med::asn::ber::tag_value<med::asn::traits<0b11111111'11111111>, false>;
	static_assert(tv16bit::value() == 0b00011111'10000011'11111111'01111111);
	using tv21bit = med::asn::ber::tag_value<med::asn::traits<0b11111'11111111'11111111>, false>;
	static_assert(tv21bit::value() == 0b00011111'11111111'11111111'01111111);
	using tv24bit = med::asn::ber::tag_value<med::asn::traits<0b11111111'11111111'11111111>, false>;
	static_assert(tv24bit::value() == 0b00011111'10000111'11111111'11111111'01111111);
	using tv28bit = med::asn::ber::tag_value<med::asn::traits<0b1111'11111111'11111111'11111111>, false>;
	static_assert(tv28bit::value() == 0b00011111'11111111'11111111'11111111'01111111);
	using tv32bit = med::asn::ber::tag_value<med::asn::traits<0b11111111'11111111'11111111'11111111>, false>;
	static_assert(tv32bit::value() == 0b00011111'10001111'11111111'11111111'11111111'01111111);
}

template <class IE>
std::string encoded(typename IE::value_type const& val)
{
	uint8_t enc_buf[128] = {};
	med::encoder_context<> ectx{ enc_buf };

	IE enc;
	enc.set(val);
	encode(med::asn::ber::encoder{ectx}, enc);

	IE dec;
	med::decoder_context<> dctx;
	dctx.reset(ectx.buffer().get_start(), ectx.buffer().get_offset());
	decode(med::asn::ber::decoder{dctx}, dec);
	EXPECT_EQ(enc.get(), dec.get());

	return as_string(ectx.buffer());
}

template <class IE>
std::string encoded()
{
	uint8_t enc_buf[128] = {};
	med::encoder_context<> ectx{ enc_buf };

	IE enc;
	encode(med::asn::ber::encoder{ectx}, enc);

	IE dec;
	med::decoder_context<> dctx;
	dctx.reset(ectx.buffer().get_start(), ectx.buffer().get_offset());
	decode(med::asn::ber::decoder{dctx}, dec);

	return as_string(ectx.buffer());
}


#if 1
TEST(asn_ber, boolean)
{	
	EXPECT_EQ("01 01 FF "s, encoded<med::asn::boolean>(true));
	EXPECT_EQ("01 01 00 "s, encoded<med::asn::boolean>(false));
}

TEST(asn_ber, prefixed_boolean)
{
	using boolean = med::value<bool, med::asn::traits<1024, med::asn::tag_class::CONTEXT_SPECIFIC>>;

	EXPECT_EQ("9F 88 00 01 FF "s, encoded<boolean>(true));
	EXPECT_EQ("9F 88 00 01 00 "s, encoded<boolean>(false));
}
#endif

TEST(asn_ber, integer)
{
	EXPECT_EQ("02 01 00 "s, encoded<med::asn::integer>(0));
	EXPECT_EQ("02 01 7F "s, encoded<med::asn::integer>(127));
	EXPECT_EQ("02 02 00 80 "s, encoded<med::asn::integer>(128));
	EXPECT_EQ("02 01 80 "s, encoded<med::asn::integer>(-128));
	EXPECT_EQ("02 02 FF 7F "s, encoded<med::asn::integer>(-129));
}

TEST(asn_ber, null)
{
	EXPECT_EQ("05 00 "s, encoded<med::asn::null>());
}

