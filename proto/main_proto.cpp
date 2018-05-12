#include <iostream>
//#include <fstream>

#include <gtest/gtest.h>

#include "med.pb.h"

std::string as_string(void const* p, std::size_t size)
{
	std::string res;

	for (auto it = static_cast<uint8_t const*>(p), ite = it + size; it != ite; ++it)
	{
		char bsz[4];
		std::snprintf(bsz, sizeof(bsz), "%02X ", *it);
		res.append(bsz);
	}

	return res;
}


TEST(encode, first)
{
	plain msg;

	msg.set_int_32(1); //1: 0108; -32:08 e0 ff ff ff ff ff ff ff ff 01 00
	msg.set_int_64(127);
	msg.set_uint_32(128);
	msg.set_uint_64(256);

	//std::stringstream out(std::ios_base::out | std::ios_base::binary);
	uint8_t out[1024];
	ASSERT_TRUE(msg.SerializePartialToArray(out, sizeof(out)));
	std::printf("FIRST:\n%s\n", as_string(out, msg.ByteSize()).c_str());
}

int main(int argc, char **argv)
{
	GOOGLE_PROTOBUF_VERIFY_VERSION;

	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
