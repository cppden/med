#include "ut.hpp"
#include "tag_named.hpp"
#include "json/json.hpp"
#include "json/encoder.hpp"

namespace js {

using namespace med::literals;

template <class N>
using T = med::tag_named<N>;

struct BOOL : med::json::boolean{};
struct INT : med::json::integer{};
struct UINT : med::json::unsignedint{};
struct NUM : med::json::number{};
struct STR : med::json::string{};

struct ARR : med::json::array<
	O< STR, med::max<10> >
>{};

struct MSG : med::json::object<
	M< T<decltype("bool_field"_name)>, BOOL >,
	O< T<decltype("int_field"_name)>, INT >,
	O< T<decltype("uint_field"_name)>, UINT >,
	O< T<decltype("double_field"_name)>, NUM >,
	O< T<decltype("array_field"_name)>, ARR >
>
{};


} //end: namespace js

#if 1

TEST(json, encode)
{
	js::MSG msg;
	msg.ref<js::BOOL>().set(true);
	msg.ref<js::INT>().set(-10);
	msg.ref<js::UINT>().set(137);
	msg.ref<js::NUM>().set(3.14159);
	auto& arr = msg.ref<js::ARR>();
	arr.push_back<js::STR>()->set("one");
	arr.push_back<js::STR>()->set("two");

	char buffer[1024] = {};
	med::json::encoder_context ctx{ buffer };

#if (MED_EXCEPTIONS)
	encode(med::json::encoder{ctx}, msg);
#else
	ASSERT_TRUE(encode(med::json::encoder{ctx}, msg)) << toString(ctx.error_ctx());
#endif
	std::string const got{buffer, ctx.buffer().get_offset()};
	std::string const exp{R"({"bool_field":true,"int_field":-10,"uint_field":137,"double_field":3.14159,"array_field":["one","two"]})"};
	EXPECT_EQ(exp, got);
}
#endif

