
#include "ut.hpp"

#include "padding.hpp"
#include "value.hpp"

TEST(value, traits)
{
	#define VT_BITS(N, T) \
	{                                                    \
		using tt = med::value_traits<med::bits<N>>;      \
		static_assert(std::is_same_v<T, tt::value_type>);\
		static_assert(tt::bits == N);                    \
	}                                                    \
	/* VT_BITS */
	VT_BITS(0, void);
	VT_BITS(1, uint8_t);
	VT_BITS(8, uint8_t);
	VT_BITS(9, uint16_t);
	VT_BITS(16, uint16_t);
	VT_BITS(17, uint32_t);
	VT_BITS(32, uint32_t);
	VT_BITS(33, uint64_t);
	VT_BITS(64, uint64_t);
	#undef VT_BITS

	#define VT_BYTES(N, T) \
	{                                                    \
		using tt = med::value_traits<med::bytes<N>>;     \
		static_assert(std::is_same_v<T, tt::value_type>);\
		static_assert(tt::bits == N*8);                  \
	}                                                    \
	/* VT_BYTES */
	VT_BYTES(1, uint8_t);
	VT_BYTES(2, uint16_t);
	VT_BYTES(3, uint32_t);
	VT_BYTES(4, uint32_t);
	VT_BYTES(5, uint64_t);
	VT_BYTES(8, uint64_t);
	#undef VT_BYTES

	#define VT_TYPE(T) \
	{                                                    \
		using tt = med::value_traits<T>;                 \
		static_assert(std::is_same_v<T, tt::value_type>);\
		static_assert(tt::bits == sizeof(T)*8);          \
	}                                                    \
	/* VT_TYPE */
	VT_TYPE(uint8_t);
	VT_TYPE(uint16_t);
	VT_TYPE(uint32_t);
	VT_TYPE(uint64_t);
	VT_TYPE(int8_t);
	VT_TYPE(int16_t);
	VT_TYPE(int32_t);
	VT_TYPE(int64_t);
	#undef VT_TYPE
}

TEST(value, fixed)
{
	using tt = med::value<med::fixed<1, uint8_t>>;
	tt v;
	static_assert(std::is_same_v<uint8_t, tt::value_type>);
	EXPECT_TRUE(v.is_set());
	EXPECT_EQ(1, v.get());
	EXPECT_TRUE(v.set_encoded(1));
	EXPECT_FALSE(v.set_encoded(2));
	EXPECT_TRUE(v == tt{});
}

TEST(value, init)
{
	using tt = med::value<med::init<1, uint8_t>>;
	tt v;
	static_assert(std::is_same_v<uint8_t, tt::value_type>);
	EXPECT_TRUE(v.is_set());
	EXPECT_EQ(1, v.get_encoded());
	EXPECT_TRUE(v == tt{});
	v.set(2);
	EXPECT_TRUE(v.is_set());
	EXPECT_EQ(2, v.get_encoded());

	tt v2;
	EXPECT_FALSE(v == v2);
	v.clear();
	EXPECT_TRUE(v == v2);
}

TEST(value, integer)
{
	#define VT_TYPE(T) \
	{                             \
		med::value<T> v;          \
		EXPECT_FALSE(v.is_set()); \
		v.set(1);                 \
		EXPECT_TRUE(v.is_set());  \
		EXPECT_EQ(1, v.get());    \
		med::value<T> v2;         \
		EXPECT_FALSE(v == v2);    \
		v2.set(v.get());          \
		EXPECT_TRUE(v == v2);     \
		v2.set(v.get() + 1);      \
		EXPECT_FALSE(v == v2);    \
	}                             \
	/* VT_TYPE */
	VT_TYPE(uint8_t);
	VT_TYPE(uint16_t);
	VT_TYPE(uint32_t);
	VT_TYPE(uint64_t);
	VT_TYPE(int8_t);
	VT_TYPE(int16_t);
	VT_TYPE(int32_t);
	VT_TYPE(int64_t);
	#undef VT_TYPE

	med::value<uint8_t, med::padding<med::bits<4>>> pbyte;
	EXPECT_FALSE(pbyte.is_set());
}

TEST(value, bytes)
{
	med::value<med::bytes<3>> v;
	EXPECT_FALSE(v.is_set());
	v.set(3);
	EXPECT_TRUE(v.is_set());
	EXPECT_EQ(3, v.get());

	check_octet_encode(v, {0,0,3});
	check_octet_decode(v, {0,0,3});
}

TEST(value, one_byte)
{
	{
		constexpr uint8_t VAL = 1;
		med::value<med::bits<1>> v;
		v.set(VAL);
		check_octet_encode(v, {0b1000'0000}, true);
		check_octet_decode(v, {0b1000'0000});
	}
	{
		constexpr uint8_t VAL = 0b11;
		med::value<med::bits<2>> v;
		v.set(VAL);
		check_octet_encode(v, {0b1100'0000}, true);
		check_octet_decode(v, {0b1100'0000});
	}
	{
		constexpr uint8_t VAL = 0b10101;
		med::value<med::bits<5>> v;
		v.set(VAL);
		check_octet_encode(v, {0b1010'1000}, true);
		check_octet_decode(v, {0b1010'1000});
	}
	{
		constexpr uint8_t VAL = 0b1010101;
		med::value<med::bits<7>> v;
		v.set(VAL);
		check_octet_encode(v, {0b1010'1010}, true);
		check_octet_decode(v, {0b1010'1010});
	}
}
TEST(value, one_byte_offset)
{
	//-- 1 bit
	{
		constexpr uint8_t VAL = 1;
		med::value<med::bits<1, 1>> v;
		v.set(VAL);
		check_octet_encode(v, {0b0100'0000}, true);
		check_octet_decode(v, {0b0100'0000});
	}
	{
		constexpr uint8_t VAL = 1;
		med::value<med::bits<1, 4>> v;
		v.set(VAL);
		check_octet_encode(v, {0b0000'1000}, true);
		check_octet_decode(v, {0b0000'1000});
	}
	{
		constexpr uint8_t VAL = 1;
		med::value<med::bits<1, 7>> v;
		v.set(VAL);
		check_octet_encode(v, {0b0000'0001});
		check_octet_decode(v, {0b0000'0001});
	}
	//-- 2 bits
	{
		constexpr uint8_t VAL = 0b11;
		using v_t = med::value<med::bits<2, 3>>;
		v_t v;
		v.set(VAL);
		check_octet_encode(v, {0b0001'1000}, true);
		check_octet_decode(v, {0b0001'1000});
	}
	{
		constexpr uint8_t VAL = 0b11;
		using v_t = med::value<med::bits<2, 6>>;
		v_t v;
		v.set(VAL);
		check_octet_encode(v, {0b0000'0011});
		check_octet_decode(v, {0b0000'0011});
	}
	//-- 5 bits
	{
		constexpr uint8_t VAL = 0b10101;
		using v_t = med::value<med::bits<5, 2>>;
		v_t v;
		v.set(VAL);
		check_octet_encode(v, {0b0010'1010}, true);
		check_octet_decode(v, {0b0010'1010});
	}
	{
		constexpr uint8_t VAL = 0b10101;
		using v_t = med::value<med::bits<5, 3>>;
		v_t v;
		v.set(VAL);
		check_octet_encode(v, {0b0001'0101});
		check_octet_decode(v, {0b0001'0101});
	}
	//-- 7 bits
	{
		constexpr uint8_t VAL = 0b1010101;
		using v_t = med::value<med::bits<7, 1>>;
		v_t v;
		v.set(VAL);
		check_octet_encode(v, {0b0101'0101});
		check_octet_decode(v, {0b0101'0101});
	}
}

TEST(value, two_bytes_cross)
{
	{
		constexpr uint16_t VAL = 0b1'0101'0101;
		med::value<med::bits<9>> v;
		v.set(VAL);
		check_octet_encode(v, {0b1010'1010, 0b1000'0000}, true);
		check_octet_decode(v, {0b1010'1010, 0b1000'0000});
	}
	{
		constexpr uint16_t VAL = 0b11'0101'0101;
		med::value<med::bits<10>> v;
		v.set(VAL);
		check_octet_encode(v, {0b1101'0101, 0b0100'0000}, true);
		check_octet_decode(v, {0b1101'0101, 0b0100'0000});
	}
	{
		constexpr uint16_t VAL = 0b101'0101'0101'0101;
		med::value<med::bits<15>> v;
		v.set(VAL);
		check_octet_encode(v, {0b1010'1010, 0b1010'1010}, true);
		check_octet_decode(v, {0b1010'1010, 0b1010'1010});
	}
}

TEST(value, two_bytes_cross_offset)
{
	{
		constexpr uint8_t VAL = 0b11;
		med::value<med::bits<2, 7>> v;
		v.set(VAL);
		check_octet_encode(v, {0b0000'0001, 0b1000'0000}, true);
		check_octet_decode(v, {0b0000'0001, 0b1000'0000});
	}
	{
		constexpr uint8_t VAL = 0b101;
		med::value<med::bits<3, 6>> v;
		v.set(VAL);
		check_octet_encode(v, {0b0000'0010, 0b1000'0000}, true);
		check_octet_decode(v, {0b0000'0010, 0b1000'0000});
	}
	{
		constexpr uint8_t VAL = 0b1101;
		med::value<med::bits<4, 5>> v;
		v.set(VAL);
		check_octet_encode(v, {0b0000'0110, 0b1000'0000}, true);
		check_octet_decode(v, {0b0000'0110, 0b1000'0000});
	}
	{
		constexpr uint8_t VAL = 0b1'0101;
		med::value<med::bits<5, 4>> v;
		v.set(VAL);
		check_octet_encode(v, {0b0000'1010, 0b1000'0000}, true);
		check_octet_decode(v, {0b0000'1010, 0b1000'0000});
	}
	{
		constexpr uint8_t VAL = 0b11'0101;
		med::value<med::bits<6, 3>> v;
		v.set(VAL);
		check_octet_encode(v, {0b0001'1010, 0b1000'0000}, true);
		check_octet_decode(v, {0b0001'1010, 0b1000'0000});
	}
	{
		constexpr uint8_t VAL = 0b101'0101;
		med::value<med::bits<7, 2>> v;
		v.set(VAL);
		check_octet_encode(v, {0b0010'1010, 0b1000'0000}, true);
		check_octet_decode(v, {0b0010'1010, 0b1000'0000});
	}
	{
		constexpr uint16_t VAL = 0b101'0101'0101'0101;
		med::value<med::bits<15, 1>> v;
		v.set(VAL);
		check_octet_encode(v, {0b0101'0101, 0b0101'0101});
		check_octet_decode(v, {0b0101'0101, 0b0101'0101});
	}
}

TEST(value, three_bytes_cross)
{
	{
		constexpr uint32_t VAL = 0b1'0111'0101'1101'0111;
		med::value<med::bits<17>> v;
		v.set(VAL);
		check_octet_encode(v, {0b1011'1010, 0b1110'1011, 0b1000'0000}, true);
		check_octet_decode(v, {0b1011'1010, 0b1110'1011, 0b1000'0000});
	}
	{
		constexpr uint32_t VAL = 0b111'0101'0111'0101'0101'0111;
		med::value<med::bits<23>> v;
		v.set(VAL);
		check_octet_encode(v, {0b1110'1010, 0b1110'1010, 0b1010'1110}, true);
		check_octet_decode(v, {0b1110'1010, 0b1110'1010, 0b1010'1110});
	}
}
TEST(value, three_bytes_cross_offset)
{
	{
		constexpr uint32_t VAL = 0b1'0111'0111'0101'0111;
		med::value<med::bits<17, 1>> v;
		v.set(VAL);
		check_octet_encode(v, {0b0101'1101, 0b1101'0101, 0b1100'0000}, true);
		check_octet_decode(v, {0b0101'1101, 0b1101'0101, 0b1100'0000});
	}
	{
		constexpr uint32_t VAL = 0b111'0101'0111'0101'0101'0111;
		med::value<med::bits<23, 1>> v;
		v.set(VAL);
		check_octet_encode(v, {0b0111'0101, 0b0111'0101, 0b0101'0111});
		check_octet_decode(v, {0b0111'0101, 0b0111'0101, 0b0101'0111});
	}
}

TEST(value, four_bytes_cross)
{
	{
		constexpr uint32_t VAL = 0b1'1010'1010'0111'0101'1101'0111;
		med::value<med::bits<25>> v;
		v.set(VAL);
		check_octet_encode(v, {0b1101'0101, 0b0011'1010, 0b1110'1011, 0b1000'0000}, true);
		check_octet_decode(v, {0b1101'0101, 0b0011'1010, 0b1110'1011, 0b1000'0000});
	}
	{
		constexpr uint32_t VAL = 0b101'1011'1010'1110'0111'0101'1101'0111;
		med::value<med::bits<31>> v;
		v.set(VAL);
		check_octet_encode(v, {0b1011'0111, 0b0101'1100, 0b1110'1011, 0b1010'1110}, true);
		check_octet_decode(v, {0b1011'0111, 0b0101'1100, 0b1110'1011, 0b1010'1110});
	}
}
TEST(value, four_bytes_cross_offset)
{
	{
		constexpr uint32_t VAL = 0b111'0101'1101'0101'1101'0101'0111'0011;
		med::value<med::bits<31, 1>> v;
		v.set(VAL);
		check_octet_encode(v, {0b0111'0101, 0b1101'0101, 0b1101'0101, 0b0111'0011});
		check_octet_decode(v, {0b0111'0101, 0b1101'0101, 0b1101'0101, 0b0111'0011});
	}
}

#if 1
TEST(value, float)
{
	{
		med::value<float> v;
		EXPECT_FALSE(v.is_set());
		v.set(5.);
		EXPECT_TRUE(v.is_set());
		EXPECT_FLOAT_EQ(5., v.get());
	}

	{
		med::value<double> v;
		EXPECT_FALSE(v.is_set());
		v.set(5.);
		EXPECT_TRUE(v.is_set());
		EXPECT_DOUBLE_EQ(5., v.get());
	}
}
#endif