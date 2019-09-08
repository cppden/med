#include <algorithm>
#include <vector>
#include <random>

#include "ut.hpp"
#include "ut_proto.hpp"

#include "meta/typelist.hpp"
#include "meta/unique.hpp"
#include "sl/octet_info.hpp"


TEST(unique, typed)
{
	//checking tag uniqueness
	using tags = med::meta::typelist<
		med::minfo< med::mi<med::mik::TAG, C<0x02>> >,
		med::minfo< med::mi<med::mik::TAG, C<0x03>> >,
		med::minfo< med::mi<med::mik::TAG, C<0x04>> >
	>;
	static_assert(std::is_void_v<med::meta::unique_t<med::tag_getter<med::sl::octet_info>, tags>>);

//	using dup_tags = med::meta::typelist<
//		med::minfo< med::mi<med::mik::TAG, C<0x02>>,
//		med::minfo< med::mi<med::mik::TAG, C<0x03>>,
//		med::minfo< med::mi<med::mik::TAG, C<0x02>>
//	>;
//	static_assert(not std::is_void_v<med::meta::unique_t<med::tag_getter<med::sl::octet_info>, dup_tags>>);
}

TEST(unique, static_odd)
{
	using namespace med::meta;
	static_assert(are_unique(nullptr, 1u, 2, 3, 4, 5));
	static_assert(are_unique(1u, 2, 3, nullptr, 4, 5));
	static_assert(are_unique(1u, 2, 3, 4, 5, nullptr));
	static_assert(not are_unique(5, 2, 3, 4, 5));
	static_assert(not are_unique(1, 5, 3, 4, 5));
	static_assert(not are_unique(1, 2, 5, 4, 5));
	static_assert(not are_unique(1, 2, 3, 5, 5));
}

TEST(unique, static_even)
{
	using namespace med::meta;
	static_assert(are_unique(1, 2, 3, 4));
	static_assert(not are_unique(4, 2, 3, 4));
	static_assert(not are_unique(1, 4, 3, 4));
	static_assert(not are_unique(1, 2, 4, 4));
}

TEST(unique, dynamics)
{
	std::random_device rdev;
	std::mt19937 rgen{rdev()};

	std::vector<int> v{1, 2, 3, 4, 5};
	for (auto i = 0; i < 5; ++i)
	{
		std::shuffle(v.begin(), v.end(), rgen);
		EXPECT_TRUE(med::meta::are_unique(v[0], v[1], v[2], v[3], v[4]));
	}

	v = {1, 2, 3, 4, 1};
	for (auto i = 0; i < 5; ++i)
	{
		std::shuffle(v.begin(), v.end(), rgen);
		EXPECT_FALSE(med::meta::are_unique(v[0], v[1], v[2], v[3], v[4]));
	}
}
