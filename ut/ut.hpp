#pragma once

/*
 * EXPECT_TRUE(Matches(buff, out_buff, size));
 */
template<typename T>
inline testing::AssertionResult Matches(T const* expected, void const* actual, std::size_t size)
{
	for (std::size_t i = 0; i < size; ++i)
	{
		if (expected[i] != static_cast<T const*>(actual)[i])
		{
			return testing::AssertionFailure() << "mismatch at " << i
				<< ": expected=" << (std::size_t)expected[i]
				<< ", actual=" << (std::size_t)static_cast<T const*>(actual)[i];
		}
	}

	return ::testing::AssertionSuccess();
}

/*
 * EXPECT_TRUE(Matches(buff, out_buff));
 */
template<typename T, std::size_t size>
inline testing::AssertionResult Matches(T const(&expected)[size], T const* actual)
{
	return Matches(expected, actual, size);
}


#define EQ_STRING_O(fld_type, expected) \
{                                                             \
	char const exp[] = expected;                              \
	auto const got = msg->get<fld_type>();                    \
	ASSERT_NE(nullptr, static_cast<fld_type const*>(got));    \
	ASSERT_EQ(sizeof(exp)-1, got->size());                    \
	ASSERT_TRUE(Matches(exp, got->data(), sizeof(exp)-1));    \
}

#define EQ_STRING_O_(INDEX, fld_type, expected) \
{                                                             \
	char const exp[] = expected;                              \
	auto const got = msg->get<fld_type, INDEX>();             \
	ASSERT_NE(nullptr, static_cast<fld_type const*>(got));    \
	ASSERT_EQ(sizeof(exp)-1, got->size());                    \
	ASSERT_TRUE(Matches(exp, got->data(), sizeof(exp)-1));    \
}

#define EQ_STRING_M(fld_type, expected) \
{                                                             \
	char const exp[] = expected;                              \
	fld_type const& got = msg->field();                       \
	ASSERT_EQ(sizeof(exp)-1, got->size());                    \
	ASSERT_TRUE(Matches(exp, got->data(), sizeof(exp)-1));    \
}

struct dummy_sink
{
	explicit dummy_sink(std::size_t factor)
		: m_factor{ factor }
	{
	}

	std::size_t m_factor;
	std::size_t num_on_value{ 0 };
	std::size_t num_on_container{ 0 };
	std::size_t num_on_custom{ 0 };

	void on_value(std::size_t depth, char const* name, std::size_t value)
	{
		++num_on_value;
		if (m_factor)
		{
			printf("%zu%*c%s : %zu (%zXh)\n", depth, int(m_factor*depth), ' ', name, value, value);
		}
	}

	template <class STR>
	auto on_value(std::size_t depth, char const* name, STR const& value) -> decltype(value.size(), value.data(), void())
	{
		++num_on_value;
		if (m_factor)
		{
			printf("%zu%*c%s : %*.*s (%zu)\n", depth, int(m_factor*depth), ' ', name, int(value.size()), int(value.size()), (char const*)value.data(), value.size());
		}
	}

	void on_container(std::size_t depth, char const* name)
	{
		++num_on_container;
		if (m_factor)
		{
			printf("%zu%*c%s\n", depth, int(m_factor*depth), ' ', name);
		}
	}

	void on_custom(std::size_t depth, char const* name, std::string const& s)
	{
		++num_on_custom;
		if (m_factor)
		{
			printf("%zu%*c%s : %s\n", depth, int(m_factor*depth), ' ', name, s.c_str());
		}
	}
};

