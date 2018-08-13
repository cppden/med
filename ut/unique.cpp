#include <algorithm>
#include <vector>
#include <random>

#include "ut.hpp"

#include "util/unique.hpp"

TEST(unique, static_odd)
{
	using namespace med::util;
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
	using namespace med::util;
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
		EXPECT_TRUE(med::util::are_unique(v[0], v[1], v[2], v[3], v[4]));
	}

	v = {1, 2, 3, 4, 1};
	for (auto i = 0; i < 5; ++i)
	{
		std::shuffle(v.begin(), v.end(), rgen);
		EXPECT_FALSE(med::util::are_unique(v[0], v[1], v[2], v[3], v[4]));
	}
}
