#include <limits>
#include <cstdint>
#include <cstddef>

#include "ut.hpp"
#include "buffer.hpp"

TEST(buffer, push_size_rejects_overflow_without_changing_state)
{
	for (bool commit : {false, true})
	{
		for (std::size_t requested : {
			std::size_t{7},
			std::numeric_limits<std::size_t>::max()
		})
		{
			SCOPED_TRACE(testing::Message()
				<< "commit=" << commit << ", requested=" << requested);

			uint8_t storage[8]{};
			med::buffer<uint8_t> buffer;
			buffer.reset(storage);
			buffer.advance(2); // Six bytes remain.

			auto const begin = buffer.begin();
			auto const end = buffer.end();

			EXPECT_THROW(buffer.push_size(requested, commit), med::overflow);
			EXPECT_EQ(buffer.begin(), begin);
			EXPECT_EQ(buffer.end(), end);
			EXPECT_EQ(buffer.size(), 6u);
		}
	}
}

TEST(buffer, push_size_accepts_boundaries_and_restores_end)
	{
	uint8_t storage[8]{};
	med::buffer<uint8_t> buffer;
	buffer.reset(storage);
	buffer.advance(2);

	for (std::size_t requested : {std::size_t{0}, std::size_t{6}})
	{
		{
			auto state = buffer.push_size(requested, true);
			EXPECT_EQ(buffer.size(), requested);
		}
		EXPECT_EQ(buffer.size(), 6u);
		EXPECT_EQ(buffer.get_offset(), 2u);
	}
}