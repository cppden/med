
#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wgnu-zero-variadic-macro-arguments"
#endif
//#pragma clang diagnostic pop

#include "encode.hpp"
#include "decode.hpp"
#include "printer.hpp"
#include "encoder_context.hpp"
#include "decoder_context.hpp"
#include "octet_encoder.hpp"
#include "octet_decoder.hpp"

#include "ut.hpp"
#include "ut_proto.hpp"

namespace prn {

struct U8 : med::value<uint8_t>
{
	static constexpr char const* name() { return "U8"; }
};

struct U16 : med::value<uint16_t>
{
	static constexpr char const* name() { return "U16"; }
};


//customized print of container
template <int I>
struct DEEP_SEQ : med::sequence< M<U8>, M<U16> >
{
	static constexpr char const* name() { return "DeepSeq"; }
	template <std::size_t N> void print(char (&sz)[N]) const
	{
		std::snprintf(sz, N, "%u#%04X", this->template get<U8>().get(), this->template get<U16>().get());
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

} //end: namespace prn

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

	prn::DEEP_MSG msg;
	med::decoder_context<> ctx;

	ctx.reset(encoded, sizeof(encoded));
#if (MED_EXCEPTIONS)
	decode(med::octet_decoder{ctx}, msg);
#else
	if (!decode(med::octet_decoder{ctx}, msg)) { FAIL() << toString(ctx.error_ctx()); }
#endif

	std::size_t const cont_num[6] = { 21,  1,  9, 15, 19, 21 };
	std::size_t const cust_num[6] = { 10,  0,  2,  4,  6,  8 };

	for (std::size_t level = 0; level < std::extent_v<decltype(cont_num)>; ++level)
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

	prn::DEEP_MSG msg;
	med::decoder_context<> ctx;

	ctx.reset(encoded, sizeof(encoded));
#if (MED_EXCEPTIONS)
	decode(med::octet_decoder{ctx}, msg);
#else
	if (!decode(med::octet_decoder{ctx}, msg)) { FAIL() << toString(ctx.error_ctx()); }
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
	decode(med::octet_decoder{m_ctx}, m_proto);
#else
	if (!decode(med::octet_decoder{m_ctx}, m_proto)) { FAIL() << toString(m_ctx.error_ctx()); }
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
	EXPECT_THROW(decode(med::octet_decoder{m_ctx}, m_proto), med::exception);
#else
	if (decode(med::octet_decoder{m_ctx}, m_proto)) { FAIL() << toString(m_ctx.error_ctx()); }
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

