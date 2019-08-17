#include <cmath>

#if 0

#include "../ut.hpp"

#include "asn/ids.hpp"
#include "asn/asn.hpp"
#include "asn/ber/length.hpp"
#include "asn/ber/encoder.hpp"
#include "asn/ber/decoder.hpp"

using namespace std::literals;

TEST(asn_ber, len_size)
{
	static_assert(med::asn::ber::length::bits<char>(0b0000'0000) == 1);
	static_assert(med::asn::ber::length::bits<char>(0b0000'0001) == 2);
	static_assert(med::asn::ber::length::bits<char>(0b0000'0010) == 3);
	static_assert(med::asn::ber::length::bits<char>(0b0000'0100) == 4);
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

	//unsigned
	static_assert(med::asn::ber::length::bits<uint8_t>(0b0000'0000) == 1);
	static_assert(med::asn::ber::length::bits<uint8_t>(0b0000'0001) == 1);
	static_assert(med::asn::ber::length::bits<uint8_t>(0b0000'0010) == 2);
	static_assert(med::asn::ber::length::bits<uint8_t>(0b0000'0100) == 3);
	static_assert(med::asn::ber::length::bits<uint8_t>(0b0000'1000) == 4);
	static_assert(med::asn::ber::length::bits<uint8_t>(0b0001'0000) == 5);
	static_assert(med::asn::ber::length::bits<uint8_t>(0b0010'0000) == 6);
	static_assert(med::asn::ber::length::bits<uint8_t>(0b0100'0000) == 7);
	static_assert(med::asn::ber::length::bits<uint8_t>(0b1000'0000) == 8);

	static_assert(med::asn::ber::length::bits<uint16_t>(0b0000'0000'0000'0000) == 1);
	static_assert(med::asn::ber::length::bits<uint16_t>(0b0000'0000'0000'0001) == 1);
	static_assert(med::asn::ber::length::bits<uint16_t>(0b0000'0000'0000'0010) == 2);
	static_assert(med::asn::ber::length::bits<uint16_t>(0b0000'0000'0000'0100) == 3);
	static_assert(med::asn::ber::length::bits<uint16_t>(0b0000'0000'0000'1000) == 4);
	static_assert(med::asn::ber::length::bits<uint16_t>(0b0000'0000'0001'0000) == 5);
	static_assert(med::asn::ber::length::bits<uint16_t>(0b0000'0000'0010'0000) == 6);
	static_assert(med::asn::ber::length::bits<uint16_t>(0b0000'0000'0100'0000) == 7);
	static_assert(med::asn::ber::length::bits<uint16_t>(0b0000'0000'1000'0000) == 8);
	static_assert(med::asn::ber::length::bits<uint16_t>(0b0000'0001'0000'0000) == 9);
	static_assert(med::asn::ber::length::bits<uint16_t>(0b0000'0010'0000'0000) == 10);
	static_assert(med::asn::ber::length::bits<uint16_t>(0b0000'0100'0000'0000) == 11);
	static_assert(med::asn::ber::length::bits<uint16_t>(0b0000'1000'0000'0000) == 12);
	static_assert(med::asn::ber::length::bits<uint16_t>(0b0001'0000'0000'0000) == 13);
	static_assert(med::asn::ber::length::bits<uint16_t>(0b0010'0000'0000'0000) == 14);
	static_assert(med::asn::ber::length::bits<uint16_t>(0b0100'0000'0000'0000) == 15);
	static_assert(med::asn::ber::length::bits<uint16_t>(0b1000'0000'0000'0000) == 16);
	static_assert(med::asn::ber::length::bits<uint16_t>(0b1111'1111'1111'1111) == 16);

	static_assert(med::asn::ber::length::bits<uint32_t>(0b0000'0000'0000'0000'0000'0000'0000'0000) == 1);
	static_assert(med::asn::ber::length::bits<uint32_t>(0b0000'0000'0000'0000'0000'0000'0000'0001) == 1);
	static_assert(med::asn::ber::length::bits<uint32_t>(0b0000'0000'0000'0000'0000'0000'0000'0010) == 2);
	static_assert(med::asn::ber::length::bits<uint32_t>(0b0000'0000'0000'0000'0000'0000'0000'0100) == 3);
	static_assert(med::asn::ber::length::bits<uint32_t>(0b0000'0000'0000'0000'0000'0000'0000'1000) == 4);
	static_assert(med::asn::ber::length::bits<uint32_t>(0b0000'0000'0000'0000'0000'0000'0001'0000) == 5);
	static_assert(med::asn::ber::length::bits<uint32_t>(0b0000'0000'0000'0000'0000'0000'0010'0000) == 6);
	static_assert(med::asn::ber::length::bits<uint32_t>(0b0000'0000'0000'0000'0000'0000'0100'0000) == 7);
	static_assert(med::asn::ber::length::bits<uint32_t>(0b0000'0000'0000'0000'0000'0000'1000'0000) == 8);
	static_assert(med::asn::ber::length::bits<uint32_t>(0b0000'0000'0000'0000'0000'0001'0000'0000) == 9);
	static_assert(med::asn::ber::length::bits<uint32_t>(0b0000'0000'0000'0000'0000'0010'0000'0000) == 10);
	static_assert(med::asn::ber::length::bits<uint32_t>(0b0000'0000'0000'0000'0000'0100'0000'0000) == 11);
	static_assert(med::asn::ber::length::bits<uint32_t>(0b0000'0000'0000'0000'0000'1000'0000'0000) == 12);
	static_assert(med::asn::ber::length::bits<uint32_t>(0b0000'0000'0000'0000'0001'0000'0000'0000) == 13);
	static_assert(med::asn::ber::length::bits<uint32_t>(0b0000'0000'0000'0000'0010'0000'0000'0000) == 14);
	static_assert(med::asn::ber::length::bits<uint32_t>(0b0000'0000'0000'0000'0100'0000'0000'0000) == 15);
	static_assert(med::asn::ber::length::bits<uint32_t>(0b0000'0000'0000'0000'1000'0000'0000'0000) == 16);
	static_assert(med::asn::ber::length::bits<uint32_t>(0b0000'0000'0000'0001'1000'0000'0000'0000) == 17);
	static_assert(med::asn::ber::length::bits<uint32_t>(0b0000'0000'0000'0010'1000'0000'0000'0000) == 18);
	static_assert(med::asn::ber::length::bits<uint32_t>(0b0000'0000'0000'0100'1000'0000'0000'0000) == 19);
	static_assert(med::asn::ber::length::bits<uint32_t>(0b0000'0000'0000'1000'1000'0000'0000'0000) == 20);
	static_assert(med::asn::ber::length::bits<uint32_t>(0b0000'0000'0001'0000'1000'0000'0000'0000) == 21);
	static_assert(med::asn::ber::length::bits<uint32_t>(0b0000'0000'0010'0000'1000'0000'0000'0000) == 22);
	static_assert(med::asn::ber::length::bits<uint32_t>(0b0000'0000'0100'0000'1000'0000'0000'0000) == 23);
	static_assert(med::asn::ber::length::bits<uint32_t>(0b0000'0000'1000'0000'1000'0000'0000'0000) == 24);
	static_assert(med::asn::ber::length::bits<uint32_t>(0b0000'0001'0000'0000'1000'0000'0000'0000) == 25);
	static_assert(med::asn::ber::length::bits<uint32_t>(0b0000'0010'0000'0000'1000'0000'0000'0000) == 26);
	static_assert(med::asn::ber::length::bits<uint32_t>(0b0000'0100'0000'0000'1000'0000'0000'0000) == 27);
	static_assert(med::asn::ber::length::bits<uint32_t>(0b0000'1000'0000'0000'1000'0000'0000'0000) == 28);
	static_assert(med::asn::ber::length::bits<uint32_t>(0b0001'0000'0000'0000'1000'0000'0000'0000) == 29);
	static_assert(med::asn::ber::length::bits<uint32_t>(0b0010'0000'0000'0000'1000'0000'0000'0000) == 30);
	static_assert(med::asn::ber::length::bits<uint32_t>(0b0100'0000'0000'0000'1000'0000'0000'0000) == 31);
	static_assert(med::asn::ber::length::bits<uint32_t>(0b1000'0000'0000'0000'1000'0000'0000'0000) == 32);
	static_assert(med::asn::ber::length::bits<uint32_t>(0b1111'1111'1111'1111'1111'1111'1111'1111) == 32);

	static_assert(med::asn::ber::length::bits<uint64_t>(0b0000'0000'0000'0000'0000'0000'0000'0000) == 1);
	static_assert(med::asn::ber::length::bits<uint64_t>(0b0000'0000'0000'0000'0000'0000'0000'0001) == 1);
	static_assert(med::asn::ber::length::bits<uint64_t>(0b0000'0000'0000'0000'0000'0000'0000'0010) == 2);
	static_assert(med::asn::ber::length::bits<uint64_t>(0b0000'0000'0000'0000'0000'0000'0000'0100) == 3);
	static_assert(med::asn::ber::length::bits<uint64_t>(0b0000'0000'0000'0000'0000'0000'0000'1000) == 4);
	static_assert(med::asn::ber::length::bits<uint64_t>(0b0000'0000'0000'0000'0000'0000'0001'0000) == 5);
	static_assert(med::asn::ber::length::bits<uint64_t>(0b0000'0000'0000'0000'0000'0000'0010'0000) == 6);
	static_assert(med::asn::ber::length::bits<uint64_t>(0b0000'0000'0000'0000'0000'0000'0100'0000) == 7);
	static_assert(med::asn::ber::length::bits<uint64_t>(0b0000'0000'0000'0000'0000'0000'1000'0000) == 8);
	static_assert(med::asn::ber::length::bits<uint64_t>(0b0000'0000'0000'0000'0000'0001'0000'0000) == 9);
	static_assert(med::asn::ber::length::bits<uint64_t>(0b0000'0000'0000'0000'0000'0010'0000'0000) == 10);
	static_assert(med::asn::ber::length::bits<uint64_t>(0b0000'0000'0000'0000'0000'0100'0000'0000) == 11);
	static_assert(med::asn::ber::length::bits<uint64_t>(0b0000'0000'0000'0000'0000'1000'0000'0000) == 12);
	static_assert(med::asn::ber::length::bits<uint64_t>(0b0000'0000'0000'0000'0001'0000'0000'0000) == 13);
	static_assert(med::asn::ber::length::bits<uint64_t>(0b0000'0000'0000'0000'0010'0000'0000'0000) == 14);
	static_assert(med::asn::ber::length::bits<uint64_t>(0b0000'0000'0000'0000'0100'0000'0000'0000) == 15);
	static_assert(med::asn::ber::length::bits<uint64_t>(0b0000'0000'0000'0000'1000'0000'0000'0000) == 16);
	static_assert(med::asn::ber::length::bits<uint64_t>(0b0000'0000'0000'0001'1000'0000'0000'0000) == 17);
	static_assert(med::asn::ber::length::bits<uint64_t>(0b0000'0000'0000'0010'1000'0000'0000'0000) == 18);
	static_assert(med::asn::ber::length::bits<uint64_t>(0b0000'0000'0000'0100'1000'0000'0000'0000) == 19);
	static_assert(med::asn::ber::length::bits<uint64_t>(0b0000'0000'0000'1000'1000'0000'0000'0000) == 20);
	static_assert(med::asn::ber::length::bits<uint64_t>(0b0000'0000'0001'0000'1000'0000'0000'0000) == 21);
	static_assert(med::asn::ber::length::bits<uint64_t>(0b0000'0000'0010'0000'1000'0000'0000'0000) == 22);
	static_assert(med::asn::ber::length::bits<uint64_t>(0b0000'0000'0100'0000'1000'0000'0000'0000) == 23);
	static_assert(med::asn::ber::length::bits<uint64_t>(0b0000'0000'1000'0000'1000'0000'0000'0000) == 24);
	static_assert(med::asn::ber::length::bits<uint64_t>(0b0000'0001'0000'0000'1000'0000'0000'0000) == 25);
	static_assert(med::asn::ber::length::bits<uint64_t>(0b0000'0010'0000'0000'1000'0000'0000'0000) == 26);
	static_assert(med::asn::ber::length::bits<uint64_t>(0b0000'0100'0000'0000'1000'0000'0000'0000) == 27);
	static_assert(med::asn::ber::length::bits<uint64_t>(0b0000'1000'0000'0000'1000'0000'0000'0000) == 28);
	static_assert(med::asn::ber::length::bits<uint64_t>(0b0001'0000'0000'0000'1000'0000'0000'0000) == 29);
	static_assert(med::asn::ber::length::bits<uint64_t>(0b0010'0000'0000'0000'1000'0000'0000'0000) == 30);
	static_assert(med::asn::ber::length::bits<uint64_t>(0b0100'0000'0000'0000'1000'0000'0000'0000) == 31);
	static_assert(med::asn::ber::length::bits<uint64_t>(0b1000'0000'0000'0000'1000'0000'0000'0000) == 32);
	static_assert(med::asn::ber::length::bits<uint64_t>(0b1111'1111'1111'1111'1111'1111'1111'1111) == 32);
}

TEST(asn_ber, len_encode)
{
	auto enc_len = [](std::size_t len)
	{
		uint8_t enc_buf[32] = {};
		med::encoder_context<> ctx{ enc_buf };
		med::asn::ber::encoder enc{ctx};

		enc.put_length<med::asn::null>(len);
		return as_string(ctx.buffer());
	};

	EXPECT_EQ("00 "s, enc_len(0));
	EXPECT_EQ("7F "s, enc_len(127));
	EXPECT_EQ("81 80 "s, enc_len(128));
	EXPECT_EQ("82 01 4D "s, enc_len(333));
}

TEST(asn_ber, len_decode)
{
	auto dec_len = [](std::initializer_list<uint8_t> bytes)
	{
		med::decoder_context<> ctx{ bytes.begin(), bytes.size() };
		med::asn::ber::decoder dec{ctx};

		return dec.ber_length<med::asn::null>();
	};

	EXPECT_EQ(0, dec_len({0}));
	EXPECT_EQ(127, dec_len({0x7F}));
	EXPECT_EQ(128, dec_len({0x81, 0x80}));
	EXPECT_EQ(333, dec_len({0x82, 0x01, 0x4D}));
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
auto encoded(typename IE::value_type const& val)
	-> std::enable_if_t<std::is_arithmetic_v<typename IE::value_type>, std::string>
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

template <class IE, typename T>
std::string encoded(T const* pval, std::size_t size)
//	-> std::enable_if_t<
//		IE::traits::asn_tag_type == med::asn::OCTET_STRING ||
//		IE::traits::asn_tag_type == med::asn::BIT_STRING
//	, std::string>
{
	uint8_t enc_buf[128*1024] = {};
	med::encoder_context<> ectx{ enc_buf };

	IE enc;
	enc.set(size, pval);
	encode(med::asn::ber::encoder{ectx}, enc);

	IE dec;
	med::decoder_context<> dctx;
	dctx.reset(ectx.buffer().get_start(), ectx.buffer().get_offset());
	decode(med::asn::ber::decoder{dctx}, dec);
	EXPECT_EQ(enc.get().size(), dec.get().size());
	//EXPECT_EQ(enc.get().data(), dec.get().data());

	return as_string(ectx.buffer());
}

template <class IE>
std::string encoded(IE const& enc)
{
	uint8_t enc_buf[128*1024] = {};
	med::encoder_context<> ectx{ enc_buf };

	encode(med::asn::ber::encoder{ectx}, enc);

	IE dec;
	med::decoder_context<> dctx;
	dctx.reset(ectx.buffer().get_start(), ectx.buffer().get_offset());
	decode(med::asn::ber::decoder{dctx}, dec);
	//EXPECT_EQ(enc.get().size(), dec.get().size());
	//EXPECT_EQ(enc.get().data(), dec.get().data());

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


//8.2 Encoding of a boolean value
TEST(asn_ber, boolean)
{	
	EXPECT_EQ("01 01 FF "s, encoded<med::asn::boolean>(true));
	EXPECT_EQ("01 01 00 "s, encoded<med::asn::boolean>(false));
}
TEST(asn_ber, prefixed_boolean)
{
	using boolean = med::asn::boolean_t<1024, med::asn::tag_class::CONTEXT_SPECIFIC>;

	EXPECT_EQ("9F 88 00 01 FF "s, encoded<boolean>(true));
	EXPECT_EQ("9F 88 00 01 00 "s, encoded<boolean>(false));
}

//8.3 Encoding of an integer value
TEST(asn_ber, integer)
{
	EXPECT_EQ("02 01 00 "s, encoded<med::asn::integer>(0));
	EXPECT_EQ("02 01 7F "s, encoded<med::asn::integer>(127));
	EXPECT_EQ("02 02 00 80 "s, encoded<med::asn::integer>(128));
	EXPECT_EQ("02 01 80 "s, encoded<med::asn::integer>(-128));
	EXPECT_EQ("02 02 FF 7F "s, encoded<med::asn::integer>(-129));
}
TEST(asn_ber, prefixed_integer)
{
	using integer = med::asn::value_t<int, 1024, med::asn::tag_class::CONTEXT_SPECIFIC>;

	EXPECT_EQ("9F 88 00 01 00 "s, encoded<integer>(0));
	EXPECT_EQ("9F 88 00 02 00 80 "s, encoded<integer>(128));
}

//8.4 Encoding of an enumerated value
TEST(asn_ber, enumerated)
{
	/*
	World-Schema DEFINITIONS AUTOMATIC TAGS ::=
	BEGIN
		Enum ::= ENUMERATED {one, two, three}
	END

	value Enum ::= one
	*/
	enum Enum { one, two, three };
	EXPECT_EQ("0A 01 00 "s, encoded<med::asn::enumerated>(one));
	EXPECT_EQ("0A 01 02 "s, encoded<med::asn::enumerated>(three));
}
TEST(asn_ber, prefixed_enumerated)
{
	/*
	World-Schema DEFINITIONS AUTOMATIC TAGS ::=
	BEGIN
		Enum ::= [1961] ENUMERATED {one, two, three}
	END

	value Enum ::= two
	*/
	enum Enum { one, two, three };
	using enumerated = med::asn::enumerated_t<1961, med::asn::tag_class::CONTEXT_SPECIFIC>;

	EXPECT_EQ("9F 8F 29 01 01 "s, encoded<enumerated>(two));
}

//8.5 Encoding of a real value
TEST(DISABLED_asn_ber, real)
{
	/*
	World-Schema DEFINITIONS AUTOMATIC TAGS ::=
	BEGIN
		Real ::= REAL
	END
	*/
	//value Real ::= 0
	EXPECT_EQ("09 00 "s, encoded<med::asn::real>(0));
	//value Real ::= 1
	EXPECT_EQ("09 03 01 20 31 "s, encoded<med::asn::real>(1));
	//value Real ::= 123,456
	EXPECT_EQ("09 09 02 20 31 32 33 2C 34 35 36 "s, encoded<med::asn::real>(123.456));
	//value Real ::= -0
	EXPECT_EQ("09 04 01 20 2D 30 "s, encoded<med::asn::real>(-0));
	//-0,0 --> 09 06 02 20 2D 30 2C 30
	//value Real ::= PLUS-INFINITY
	EXPECT_EQ("09 01 40 "s, encoded<med::asn::real>(std::numeric_limits<double>::infinity()));
	//value Real ::= MINUS-INFINITY
//	EXPECT_EQ("09 01 41 "s, encoded<med::asn::real>(med::asn::minus_inf));
	//value Real ::= NOT-A-NUMBER
	EXPECT_EQ("09 01 42 "s, encoded<med::asn::real>(std::nan("")));
	//value Real ::= -0
	EXPECT_EQ("09 01 43 "s, encoded<med::asn::real>(-0));
}

//8.6 Encoding of a bitstring value
TEST(asn_ber, bit_string)
{
	/*
	World-Schema DEFINITIONS AUTOMATIC TAGS ::= BEGIN
		Str ::= BIT STRING
	END
	*/

	//value Str ::= ''H
	uint8_t const none[] = {0};
	EXPECT_EQ("03 01 00 "s, encoded<med::asn::bit_string>(none, 0));
	//value Str ::= '0A3B5F291CD'H
	uint8_t const small[] = {0x0A,0x3B,0x5F,0x29,0x1C,0xD0};
	EXPECT_EQ("03 07 04 0A 3B 5F 29 1C D0 "s, encoded<med::asn::bit_string>(small, 11*4));
}

//8.7 Encoding of an octetstring value
TEST(asn_ber, octet_string)
{
	/*
	World-Schema DEFINITIONS AUTOMATIC TAGS ::= BEGIN
		Str ::= OCTET STRING
	END
	*/

	//value Str ::= '010203'H
	uint8_t const small[] = {1, 2, 3};
	EXPECT_EQ("04 03 01 02 03 "s, encoded<med::asn::octet_string>(small, std::size(small)));

	//value Str ::= '000102030405060708090A0B0C0D0E0F101112131415161718191A1B1C1D1E1F202122232425262728292A2B2C2D2E2F303132333435363738393A3B3C3D3E3F404142434445464748494A4B4C4D4E4F505152535455565758595A5B5C5D5E5F606162636465666768696A6B6C6D6E6F707172737475767778797A7B7C7D7E7F808182838485868788898A8B8C8D8E8F909192939495969798999A9B9C9D9E9FA0A1A2A3A4A5A6A7A8A9AAABACADAEAFB0B1B2B3B4B5B6B7B8B9BABBBCBDBEBFC0C1C2C3C4C5C6C7C8C9CACBCCCDCECFD0D1D2D3D4D5D6D7D8D9DADBDCDDDEDFE0E1E2E3E4E5E6E7E8E9EAEBECEDEEEFF0F1F2F3F4F5F6F7F8F9FAFBFCFDFEFF000102030405060708090A0B0C0D0E0F101112131415161718191A1B1C1D1E1F202122232425262728292A2B2C2D2E2F303132333435363738393A3B3C3D3E3F404142434445464748494A4B4C'H
	std::vector<uint8_t> big;
	for (std::size_t i = 0; i < 333; ++i) { big.push_back(uint8_t(i)); }
	EXPECT_EQ("04 82 01 4D 00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F 10 11 12 13 14 15 16 17 18 19 1A 1B 1C 1D 1E 1F 20 21 22 23 24 25 26 27 28 29 2A 2B 2C 2D 2E 2F 30 31 32 33 34 35 36 37 38 39 3A 3B 3C 3D 3E 3F 40 41 42 43 44 45 46 47 48 49 4A 4B 4C 4D 4E 4F 50 51 52 53 54 55 56 57 58 59 5A 5B 5C 5D 5E 5F 60 61 62 63 64 65 66 67 68 69 6A 6B 6C 6D 6E 6F 70 71 72 73 74 75 76 77 78 79 7A 7B 7C 7D 7E 7F 80 81 82 83 84 85 86 87 88 89 8A 8B 8C 8D 8E 8F 90 91 92 93 94 95 96 97 98 99 9A 9B 9C 9D 9E 9F A0 A1 A2 A3 A4 A5 A6 A7 A8 A9 AA AB AC AD AE AF B0 B1 B2 B3 B4 B5 B6 B7 B8 B9 BA BB BC BD BE BF C0 C1 C2 C3 C4 C5 C6 C7 C8 C9 CA CB CC CD CE CF D0 D1 D2 D3 D4 D5 D6 D7 D8 D9 DA DB DC DD DE DF E0 E1 E2 E3 E4 E5 E6 E7 E8 E9 EA EB EC ED EE EF F0 F1 F2 F3 F4 F5 F6 F7 F8 F9 FA FB FC FD FE FF 00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F 10 11 12 13 14 15 16 17 18 19 1A 1B 1C 1D 1E 1F 20 21 22 23 24 25 26 27 28 29 2A 2B 2C 2D 2E 2F 30 31 32 33 34 35 36 37 38 39 3A 3B 3C 3D 3E 3F 40 41 42 43 44 45 46 47 48 49 4A 4B 4C "s
		, encoded<med::asn::octet_string>(big.data(), big.size()));
}
TEST(asn_ber, octet_string_prefixed)
{
/*
World-Schema DEFINITIONS AUTOMATIC TAGS ::= BEGIN
	Str ::= [APPLICATION 12321] OCTET STRING
END
*/
	using octet_str = med::asn::octet_string_t<12321, med::asn::tag_class::APPLICATION>;

	//value Str ::= '010203'H
	uint8_t const small[] = {1, 2, 3};
	EXPECT_EQ("5F E0 21 03 01 02 03 "s, encoded<octet_str>(small, std::size(small)));

	//value Str ::= '000102030405060708090A0B0C0D0E0F101112131415161718191A1B1C1D1E1F202122232425262728292A2B2C2D2E2F303132333435363738393A3B3C3D3E3F404142434445464748494A4B4C4D4E4F505152535455565758595A5B5C5D5E5F606162636465666768696A6B6C6D6E6F707172737475767778797A7B7C7D7E7F808182838485868788898A8B8C8D8E8F909192939495969798999A9B9C9D9E9FA0A1A2A3A4A5A6A7A8A9AAABACADAEAFB0B1B2B3B4B5B6B7B8B9BABBBCBDBEBFC0C1C2C3C4C5C6C7C8C9CACBCCCDCECFD0D1D2D3D4D5D6D7D8D9DADBDCDDDEDFE0E1E2E3E4E5E6E7E8E9EAEBECEDEEEFF0F1F2F3F4F5F6F7F8F9FAFBFCFDFEFF000102030405060708090A0B0C0D0E0F101112131415161718191A1B1C1D1E1F202122232425262728292A2B2C2D2E2F303132333435363738393A3B3C3D3E3F404142434445464748494A4B4C'H
	std::vector<uint8_t> big;
	for (std::size_t i = 0; i < 333; ++i) { big.push_back(uint8_t(i)); }
	EXPECT_EQ("5F E0 21 82 01 4D 00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F 10 11 12 13 14 15 16 17 18 19 1A 1B 1C 1D 1E 1F 20 21 22 23 24 25 26 27 28 29 2A 2B 2C 2D 2E 2F 30 31 32 33 34 35 36 37 38 39 3A 3B 3C 3D 3E 3F 40 41 42 43 44 45 46 47 48 49 4A 4B 4C 4D 4E 4F 50 51 52 53 54 55 56 57 58 59 5A 5B 5C 5D 5E 5F 60 61 62 63 64 65 66 67 68 69 6A 6B 6C 6D 6E 6F 70 71 72 73 74 75 76 77 78 79 7A 7B 7C 7D 7E 7F 80 81 82 83 84 85 86 87 88 89 8A 8B 8C 8D 8E 8F 90 91 92 93 94 95 96 97 98 99 9A 9B 9C 9D 9E 9F A0 A1 A2 A3 A4 A5 A6 A7 A8 A9 AA AB AC AD AE AF B0 B1 B2 B3 B4 B5 B6 B7 B8 B9 BA BB BC BD BE BF C0 C1 C2 C3 C4 C5 C6 C7 C8 C9 CA CB CC CD CE CF D0 D1 D2 D3 D4 D5 D6 D7 D8 D9 DA DB DC DD DE DF E0 E1 E2 E3 E4 E5 E6 E7 E8 E9 EA EB EC ED EE EF F0 F1 F2 F3 F4 F5 F6 F7 F8 F9 FA FB FC FD FE FF 00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F 10 11 12 13 14 15 16 17 18 19 1A 1B 1C 1D 1E 1F 20 21 22 23 24 25 26 27 28 29 2A 2B 2C 2D 2E 2F 30 31 32 33 34 35 36 37 38 39 3A 3B 3C 3D 3E 3F 40 41 42 43 44 45 46 47 48 49 4A 4B 4C "s
		, encoded<octet_str>(big.data(), big.size()));
}

//8.8 Encoding of a null value
TEST(asn_ber, null)
{
	/*
	World-Schema DEFINITIONS AUTOMATIC TAGS ::= BEGIN
		Nothing ::= NULL
	END

	value Nothing ::= NULL
	*/
	EXPECT_EQ("05 00 "s, encoded<med::asn::null>());
}
TEST(asn_ber, null_prefixed)
{
	/*
	World-Schema DEFINITIONS AUTOMATIC TAGS ::= BEGIN
		Nothing ::= [137] NULL
	END
	*/
	using null = med::asn::null_t<137, med::asn::tag_class::CONTEXT_SPECIFIC>;
	EXPECT_EQ("9F 81 09 00 "s, encoded<null>());
}

namespace ab {
template <typename ...T>
using M = med::mandatory<T...>;
template <typename ...T>
using O = med::optional<T...>;

/*
World-Schema DEFINITIONS AUTOMATIC TAGS ::=
BEGIN
	Seq ::= SEQUENCE
	{
		moct	OCTET STRING,
		ooct	OCTET STRING OPTIONAL,
		mint	INTEGER,
		oint	INTEGER OPTIONAL
	}
END
*/
struct moct : med::asn::octet_string_t<0, med::asn::tag_class::CONTEXT_SPECIFIC> {};
struct ooct : med::asn::octet_string_t<1, med::asn::tag_class::CONTEXT_SPECIFIC> {};
struct mint : med::asn::value_t<int, 2, med::asn::tag_class::CONTEXT_SPECIFIC> {};
struct oint : med::asn::value_t<int, 3, med::asn::tag_class::CONTEXT_SPECIFIC> {};

//eq SEQUENCE: tag = [UNIVERSAL 16] constructed; length = 18
//  moct OCTET STRING: tag = [0] primitive; length = 2
//    0x1234
//  ooct OCTET STRING: tag = [1] primitive; length = 3
//    0x456789
//  mint INTEGER: tag = [2] primitive; length = 1
//    7
//  oint INTEGER: tag = [3] primitive; length = 4

struct Seq : med::asn::sequence<
	M<moct>,
	O<ooct>,
	M<mint>,
	O<oint>
>
{};

}

//8.9 Encoding of a sequence value
TEST(asn_ber, sequence)
{
	ab::Seq s;
	{
		/*
		value Seq ::= {
			moct '1234'H,
			ooct '456789'H,
			mint 7,
			oint 987654321
		}
		*/
		uint8_t const moct_val[] = {0x12, 0x34};
		uint8_t const ooct_val[] = {0x45, 0x67, 0x89};
		s.ref<ab::moct>().set(sizeof(moct_val), moct_val);
		s.ref<ab::ooct>().set(sizeof(ooct_val), ooct_val);
		s.ref<ab::mint>().set(7);
		s.ref<ab::oint>().set(987654321);
		EXPECT_EQ("30 12 80 02 12 34 81 03 45 67 89 82 01 07 83 04 3A DE 68 B1 "s, encoded(s));
	}
#if 0
	s.clear();
	{
		/*
		value Seq ::= {
			moct '1234'H,
			mint 7
		}
		*/
		uint8_t const moct_val[] = {0x12, 0x34};
		s.ref<ab::moct>().set(sizeof(moct_val), moct_val);
		s.ref<ab::mint>().set(7);
		EXPECT_EQ("30 07 80 02 12 34 82 01 07 "s, encoded(s));
	}
#endif
}

//8.10 Encoding of a sequence-of value
//8.11 Encoding of a set value
//8.12 Encoding of a set-of value
//8.13 Encoding of a choice value
/*
	cho CHOICE
	{
		one INTEGER,
		two INTEGER
	}
*/
//8.14 Encoding of a value of a prefixed type
//8.15 Encoding of an open type
//8.16 Encoding of an instance-of value
//8.17 Encoding of a value of the embedded-pdv type
//8.18 Encoding of a value of the external type
//8.19 Encoding of an object identifier value
//8.20 Encoding of a relative object identifier value
//8.21 Encoding of an OID internationalized resource identifier value
//8.22 Encoding of a relative OID internationalized resource identifier value
//8.23 Encoding for values of the restricted character string types
//8.24 Encoding for values of the unrestricted character string type
//8.25 Encoding for values of the useful types
//8.26 Encoding for values of the TIME type and the useful time types

#endif
