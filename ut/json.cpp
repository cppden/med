#include "ut.hpp"
#include "json/json.hpp"
#include "json/encoder.hpp"
#include "json/decoder.hpp"

using namespace std::string_view_literals;

namespace js {

using namespace med::literals;

template <class N>
using T = med::json::T<N>;

struct BOOL : med::json::boolean{};
struct INT : med::json::integer{};
struct UINT : med::json::unsignedint{};
struct NUM : med::json::number{};
struct STR : med::json::string{};

struct MSG : med::json::object<
	M< T<decltype("bool_field"_name)>, BOOL >,
	O< T<decltype("int_field"_name)>, INT >,
	O< T<decltype("uint_field"_name)>, UINT >,
	O< T<decltype("double_field"_name)>, NUM >,
	O< T<decltype("array_field"_name)>, STR, med::max<10> >
>
{};


} //end: namespace js

TEST(json, hash)
{
	using namespace med::literals;
	using sample = decltype("Sample"_name);
	constexpr std::string_view sv{sample::data(), sample::size()};
	using hash = med::hash<uint64_t>;
	constexpr uint64_t chash = hash::compute(sv);

	uint64_t hval = hash::init;
	for (char const c : sv) { hval = hash::update(c, hval); }
	ASSERT_EQ(chash, hval);
}

#if 1
TEST(json, encode)
{
	js::MSG msg;
	msg.ref<js::BOOL>().set(true);
	msg.ref<js::INT>().set(-10);
	msg.ref<js::UINT>().set(137);
	msg.ref<js::NUM>().set(3.14159);
	msg.ref<js::STR>().push_back()->set("one");
	msg.ref<js::STR>().push_back()->set("two");

	char buffer[1024] = {};
	med::json::encoder_context ctx{ buffer };

	encode(med::json::encoder{ctx}, msg);
	std::string const got{buffer, ctx.buffer().get_offset()};
	std::string const exp{R"({"bool_field":true,"int_field":-10,"uint_field":137,"double_field":3.14159,"array_field":["one","two"]})"};
	EXPECT_EQ(exp, got);
}
#endif

#if 1
TEST(json, decode)
{
	std::string const encoded{R"(
		{
			"bool_field"   : true,
			"int_field"    : -10,
			"uint_field"   : 137,
			"double_field" : 3.14159,
			"array_field"  : [ "one", "two", "three" ]
		}
	)"};

	med::json::decoder_context ctx{ encoded.data(), encoded.size() };

	js::MSG msg;
	decode(med::json::decoder{ctx}, msg);

	auto const& cmsg = msg;
	EXPECT_EQ(true, cmsg.get<js::BOOL>().get());
	{
		js::INT const* pf = cmsg.field();
		ASSERT_NE(nullptr, pf);
		EXPECT_EQ(-10, pf->get());
	}
	{
		js::UINT const* pf = cmsg.field();
		ASSERT_NE(nullptr, pf);
		EXPECT_EQ(137, pf->get());
	}
	{
		js::NUM const* pf = cmsg.field();
		ASSERT_NE(nullptr, pf);
		EXPECT_EQ(3.14159, pf->get());
	}
	{
		std::string_view const casv[] = {"one"sv,"two"sv,"three"sv};
		ASSERT_EQ(std::size(casv), cmsg.count<js::STR>());
		auto* i = casv;
		for (auto& s : cmsg.get<js::STR>())
		{
			EXPECT_EQ(*i, s.get());
			++i;
		}
	}
}

#endif
