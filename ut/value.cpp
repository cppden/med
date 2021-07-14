
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
	static_assert(tt::is_const);
	static_assert(!tt::writable::is_const);
	EXPECT_TRUE(v.is_set());
	EXPECT_EQ(1, v.get());
	EXPECT_TRUE(v.set_encoded(1));
	EXPECT_FALSE(v.set_encoded(2));
}

TEST(value, init)
{
	using tt = med::value<med::init<1, uint8_t>>;
	tt v;
	static_assert(std::is_same_v<uint8_t, tt::value_type>);
	static_assert(tt::is_const);
	EXPECT_TRUE(v.is_set());
	EXPECT_EQ(1, v.get_encoded());
}

TEST(value, default)
{
	using tt = med::value<med::defaults<1, uint8_t>>;
	tt v;
	static_assert(std::is_same_v<uint8_t, tt::value_type>);
	EXPECT_FALSE(v.is_set());
	EXPECT_EQ(1, v.get_encoded());
	v.set(2);
	EXPECT_TRUE(v.is_set());
	EXPECT_EQ(2, v.get_encoded());
}

TEST(value, integer)
{
	#define VT_TYPE(T) \
	{                                                    \
		med::value<T> v;                                 \
		EXPECT_FALSE(v.is_set());                        \
		v.set(1);                                      \
		EXPECT_TRUE(v.is_set());                         \
		EXPECT_EQ(1, v.get());                         \
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
}

TEST(value, bits)
{
	med::value<med::bits<15>> v;
	EXPECT_FALSE(v.is_set());
	v.set(3);
	EXPECT_TRUE(v.is_set());
	EXPECT_EQ(3, v.get());
}

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
