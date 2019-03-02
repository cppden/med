#include "ut.hpp"
#include "bit_string.hpp"


constexpr std::size_t MIXED = 0b1110'1110'0111'0111'0011'0011'0101'0101;

TEST(bits, variable)
{
	med::bits_variable bits;
	EXPECT_FALSE(bits.is_set());
	EXPECT_EQ(nullptr, bits.data());
	EXPECT_EQ(med::nbits{0}, bits.least_bits());

	{
		bits.uint(med::nbits{1}, MIXED);
		EXPECT_TRUE(bits.is_set());
		EXPECT_EQ(0b1, bits.uint());
		EXPECT_EQ(med::nbits{1}, bits.least_bits());
		EXPECT_EQ(1, bits.size());
		uint8_t const exp[] = {0b1000'0000};
		ASSERT_TRUE(Matches(exp, bits.data()));
	}
	{
		bits.uint(med::nbits{7}, MIXED);
		EXPECT_EQ(0b1010101, bits.uint());
		EXPECT_EQ(med::nbits{7}, bits.least_bits());
		EXPECT_EQ(1, bits.size());
		uint8_t const exp[] = {0b1010'1010};
		ASSERT_TRUE(Matches(exp, bits.data()));
	}
	{
		bits.uint(med::nbits{8}, MIXED);
		EXPECT_EQ(0b1010101, bits.uint());
		EXPECT_EQ(med::nbits{8}, bits.least_bits());
		EXPECT_EQ(1, bits.size());
		uint8_t const exp[] = {0b0101'0101};
		ASSERT_TRUE(Matches(exp, bits.data()));
	}
	{
		bits.uint(med::nbits{19}, MIXED);
		EXPECT_EQ(0b111'0011'0011'0101'0101, bits.uint());
		EXPECT_EQ(med::nbits{3}, bits.least_bits());
		EXPECT_EQ(3, bits.size());
		uint8_t const exp[] = {0b1110'0110, 0b0110'1010, 0b1010'0000};
		ASSERT_TRUE(Matches(exp, bits.data()));
	}

	bits.clear();
	EXPECT_FALSE(bits.is_set());
	EXPECT_EQ(nullptr, bits.data());
	EXPECT_EQ(med::nbits{0}, bits.least_bits());
}

TEST(bits, fixed_small)
{
	{
		med::bits_fixed<7> bits;
		EXPECT_FALSE(bits.is_set());
		EXPECT_EQ(nullptr, bits.data());
		EXPECT_EQ(med::nbits{7}, bits.least_bits());

		bits.uint(0b01010101);
		EXPECT_TRUE(bits.is_set());
		EXPECT_EQ(0b01010101, bits.uint());
		uint8_t const exp[] = {0b1010'1010};
		static_assert (std::size(exp) == bits.size());
		ASSERT_TRUE(Matches(exp, bits.data()));
	}
	{
		med::bits_fixed<17> bits;
		EXPECT_EQ(med::nbits{1}, bits.least_bits());

		bits.uint(0b1'0100'1100'0111);
		EXPECT_EQ(0b1'0100'1100'0111, bits.uint());
		//0'0001'0100'1100'0111
		uint8_t const exp[] = {0b0'0001'010, 0b0110'0011, 0b1000'0000};
		static_assert (std::size(exp) == bits.size());
		ASSERT_TRUE(Matches(exp, bits.data()));
	}
}
