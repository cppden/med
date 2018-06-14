#include "ut.hpp"

#include "encode.hpp"
#include "decode.hpp"
#include "encoder_context.hpp"
#include "decoder_context.hpp"

#include "protobuf/protobuf.hpp"
#include "protobuf/encoder.hpp"
#include "protobuf/decoder.hpp"

using namespace med::protobuf;

namespace pb {

/*
message plain {
	int32    int_32  = 1;
	int64    int_64  = 2;
	uint32   uint_32 = 3;
	uint64   uint_64 = 4;
}
*/

template <uint32_t FIELD_NUM, med::protobuf::wire_type TYPE>
using T = med::value<med::fixed<med::protobuf::field_tag(FIELD_NUM, TYPE), med::protobuf::field_type>>;

struct plain : med::sequence<
	O< T<1, wire_type::VARINT>, int32 >,
	O< T<2, wire_type::VARINT>, int64 >,
	O< T<3, wire_type::VARINT>, uint32 >,
	O< T<4, wire_type::VARINT>, uint64 >
>{};

uint8_t const plain_encoded[] = {
	0x08, 0x01, //(T{1}<<3)|Varint{0}, value{1}
	0x10, 0x7f, //(T{2}<<3)|Varint{0}, value{127}
	0x18, 0x80, 0x01, //(T{3}<<3)|Varint{0}, value{128}
	0x20, 0x80, 0x02, //(T{4}<<3)|Varint{0}, value{256}
};

} //end: namespace pb

#define OPT_CHECK(MSG, FIELD, VALUE) \
	{ FIELD const* p = MSG.field();  \
	ASSERT_NE(nullptr, p);           \
	EXPECT_EQ(VALUE, p->get());}    \
/* OPT_CHECK */

TEST(protobuf, encode_plain)
{
	pb::plain msg;
	msg.ref<int32>().set(1);
	msg.ref<int64>().set(127);
	msg.ref<uint32>().set(128);
	msg.ref<uint64>().set(256);

	uint8_t buffer[128] = {};
	med::encoder_context<> ctx{ buffer };

#if (MED_EXCEPTIONS)
	encode(med::protobuf::encoder{ctx}, msg);
#else
	ASSERT_TRUE(encode(med::protobuf::encoder{ctx}, msg)) << toString(ctx.error_ctx());
#endif
	EXPECT_EQ(sizeof(pb::plain_encoded), ctx.buffer().get_offset());
	EXPECT_TRUE(Matches(pb::plain_encoded, buffer));
}

TEST(protobuf, decode_plain)
{
	med::decoder_context<> ctx{ pb::plain_encoded };

	pb::plain msg;

#if (MED_EXCEPTIONS)
	decode(med::protobuf::decoder{ctx}, msg);
#else
	ASSERT_TRUE(decode(med::protobuf::decoder{ctx}, msg)) << toString(ctx.error_ctx());
#endif

	pb::plain const& cmsg = msg;

	OPT_CHECK(cmsg, int32,  1);
	OPT_CHECK(cmsg, int64,  127);
	OPT_CHECK(cmsg, uint32, 128);
	OPT_CHECK(cmsg, uint64, 256);
}

