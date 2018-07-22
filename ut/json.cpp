#include "ut.hpp"
#include "tag_named.hpp"
#include "json/json.hpp"
#include "json/encoder.hpp"

namespace js {

using namespace med::literals;

template <class N>
using T = med::tag_named<N>;

struct FBOOL : med::json::boolean {};
struct FINT : med::json::integer {};
//struct FNUM : med::json::number{};

struct MSG : med::set< med::value<std::size_t>,
	M< T<decltype("bool_field"_name)>, FBOOL>,
	O< T<decltype("integer_field"_name)>, FINT>
>
{
//	static constexpr char const* name() { return "message"; }
};


} //end: namespace js

#if 1

TEST(json, encode)
{
	js::MSG msg;
	msg.ref<js::FBOOL>().set(true);
	msg.ref<js::FINT>().set(-10);

	uint8_t buffer[1024] = {};
	med::encoder_context<> ctx{ buffer };

#if (MED_EXCEPTIONS)
	encode(med::json::encoder{ctx}, msg);
#else
	ASSERT_TRUE(encode(med::json::encoder{ctx}, msg)) << toString(ctx.error_ctx());
#endif
	std::string const got{(char*)buffer, ctx.buffer().get_offset()};
	std::string const exp{R"({"bool_field":true,"integer_field":-10})"};
	EXPECT_EQ(exp, got);
}
#endif

