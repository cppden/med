#pragma once
#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wgnu-zero-variadic-macro-arguments"
#endif

#include <cstdio>
#include <gtest/gtest.h>

#include <string_view>
using namespace std::string_view_literals;


#include "med.hpp"
#include "encode.hpp"
#include "decode.hpp"
#include "encoder_context.hpp"
#include "decoder_context.hpp"
#include "octet_encoder.hpp"
#include "octet_decoder.hpp"

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
			char sz[256];
			std::snprintf(sz, sizeof(sz), "mismatch at %zu[0x%zX]: expected=0x%X, actual=0x%X"
					, i, i, expected[i], static_cast<T const*>(actual)[i]);
			return testing::AssertionFailure() << sz;
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

template <class T>
char const* as_string(T const& buffer)
{
	static char sz[1024];
	auto* it = buffer.get_start();
	auto* ite = it + buffer.get_offset();

	char* psz = sz;
	char* end = psz + sizeof(sz);
	*psz = '\0';
	for (; it != ite; ++it)
	{
		psz += std::snprintf(psz, end - psz, "%02X ", *it);
	}

	return sz;
}

template <class FIELD, class MSG, class T>
void check_seqof(MSG const& msg, std::initializer_list<T>&& values)
{
	std::size_t const count = values.size();
	ASSERT_EQ(count, msg.template count<FIELD>());

	auto it = std::begin(values);
	for (auto& v : msg.template get<FIELD>())
	{
		decltype(v.get()) expected = *it++;
		EXPECT_EQ(expected, v.get());
	}
}

template <class FIELD, class MSG>
void check_seqof(MSG const& msg, std::initializer_list<char const*>&& values)
{
	std::size_t const count = values.size();
	ASSERT_EQ(count, msg.template count<FIELD>());

	auto it = std::begin(values);
	for (auto& v : msg.template get<FIELD>())
	{
		char const* expected = *it++;
		std::size_t const len = std::strlen(expected);
		ASSERT_EQ(len, v.get().size());
		ASSERT_TRUE(Matches(expected, v.get().data(), len));
	}
}

template <class T>
inline auto get_string(T const& field) ->
	std::enable_if_t<std::is_class_v<std::remove_reference_t<decltype(field.body())>>, decltype(field.body())>
{
	return field.body();
}

template <class T>
inline auto get_string(T const& field) -> std::enable_if_t<std::is_pointer_v<decltype(field.data())>, T const&>
{
	return field;
}

template <class T>
constexpr std::string_view as_sv(T const& val)
{
	return std::string_view{(char*)val.body().data(), val.body().size()};
}


#define EQ_STRING_O(fld_type, expected) \
{                                                             \
	char const exp[] = expected;                              \
	auto const got = msg->get<fld_type>();                    \
	ASSERT_NE(nullptr, static_cast<fld_type const*>(got));    \
	ASSERT_EQ(sizeof(exp)-1, got->size());                    \
	ASSERT_TRUE(Matches(exp, got->data(), sizeof(exp)-1));    \
}

#define EQ_STRING_O_(index, fld_type, expected) \
{                                                             \
	char const exp[] = expected;                              \
	auto const& range = msg->get<fld_type>();                   \
	auto it = range.begin(); \
	ASSERT_NE(range.end(), it); \
	for (std::size_t i = index; i > 0; --i, ++it) { \
		ASSERT_NE(range.end(), it); \
	} \
	ASSERT_EQ(sizeof(exp)-1, it->size());                    \
	ASSERT_TRUE(Matches(exp, it->data(), sizeof(exp)-1));    \
}

#define EQ_STRING_M(fld_type, expected) \
{                                                         \
	char const exp[] = expected;                          \
	fld_type const& field = msg->get<fld_type>();         \
	auto const& str = get_string(field);                  \
	ASSERT_EQ(sizeof(exp)-1, str.size());                 \
	ASSERT_TRUE(Matches(exp, str.data(), sizeof(exp)-1)); \
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
		else
		{
			CODEC_TRACE("value[%s]=%zu", name, value);
		}
	}

	template <class STR>
	auto on_value(std::size_t depth, char const* name, STR const& value) -> decltype(value.size(), value.data(), void())
	{
		++num_on_value;
		if (m_factor)
		{
			printf("%zu%*c%s :", depth, int(m_factor*depth), ' ', name);

			using iter_t = uint8_t const*;
			for (iter_t it = value.data(), ite = it + value.size(); it != ite; ++it)
			{
				printf(" %02X", *it);
			}

			printf(" (%zu)\n", value.size());
		}
		else
		{
			CODEC_TRACE("value[%s]=%*.*s", name, int(value.size()), int(value.size()), (char const*)value.data());
		}
	}

	void on_container(std::size_t depth, char const* name)
	{
		++num_on_container;
		if (m_factor)
		{
			printf("%zu%*c%s\n", depth, int(m_factor*depth), ' ', name);
		}
		else
		{
			CODEC_TRACE("container[%s]", name);
		}
	}

	void on_custom(std::size_t depth, char const* name, std::string const& s)
	{
		++num_on_custom;
		if (m_factor)
		{
			printf("%zu%*c%s : %s\n", depth, int(m_factor*depth), ' ', name, s.c_str());
		}
		else
		{
			CODEC_TRACE("custom[%s]=%s", name, s.c_str());
		}
	}
};

template <typename ...T>
using M = med::mandatory<T...>;
template <typename ...T>
using O = med::optional<T...>;

using L = med::length_t<med::value<uint8_t>>;
using CNT = med::counter_t<med::value<uint16_t>>;
template <std::size_t TAG>
using T = med::value<med::fixed<TAG, uint8_t>>;
template <std::size_t TAG>
using C = med::value<med::fixed<TAG>>;
