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
#include "printer.hpp"

template <typename... T>
using M = med::mandatory<T...>;
template <typename... T>
using O = med::optional<T...>;

using L = med::length_t<med::value<uint8_t>>;
using CNT = med::counter_t<med::value<uint16_t>>;
template <std::size_t TAG>
using T = med::value<med::fixed<TAG, uint8_t>>;
template <std::size_t TAG>
using T16 = med::value<med::fixed<TAG, uint16_t>>;
template <std::size_t TAG>
using C = med::value<med::fixed<TAG, uint8_t>>;

/*
 * EXPECT_TRUE(Matches(buff, out_buff, size));
 */
template<typename T>
inline testing::AssertionResult Matches(T const* expected, void const* actual, std::size_t size)
{
	auto* p = static_cast<T const*>(actual);
	for (std::size_t i = 0; i < size; ++i)
	{
		if (expected[i] != p[i])
		{
			char sz[4*1024];
			int n = std::snprintf(sz, sizeof(sz), "\nMismatch at offsets:\noffset: expected |   actual\n");
			while (i < size)
			{
				if (expected[i] != p[i])
				{
					n += std::snprintf(sz+n, sizeof(sz)-n, "%6zu: %8X | %8X\n", i, expected[i], p[i]);
				}
				++i;
			}
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

template <std::integral U, std::size_t SIZE>
char const *as_string(U (&buffer)[SIZE])
{
	static char sz[64 * 1024];

	auto psz = sz, end = psz + sizeof(sz);
	for (auto b : buffer)
	{
		psz += std::snprintf(psz, end - psz, "%02X ", b);
	}

	return sz;
}

template <class T>
char const* as_string(T const& buffer)
{
	static char sz[64*1024];

	auto psz = sz, end = psz + sizeof(sz);
	for (auto it = buffer.get_start(), ite = it + buffer.get_offset(); it != ite; ++it)
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
	std::size_t num_on_error{ 0 };

	void on_value(std::size_t depth, char const* name, std::size_t value)
	{
		++num_on_value;
		if (m_factor)
		{
			std::printf("%zu%*c%s : %zu (%zXh)\n", depth, int(m_factor*depth), ' ', name, value, value);
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
			std::printf("%zu%*c%s :", depth, int(m_factor*depth), ' ', name);

			for (auto it = value.data(), ite = it + value.size(); it != ite; ++it)
			{
				std::printf(" %02X", *it);
			}

			std::printf(" (%zu)\n", value.size());
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
			std::printf("%zu%*c%s\n", depth, int(m_factor*depth), ' ', name);
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
			std::printf("%zu%*c%s : %s\n", depth, int(m_factor*depth), ' ', name, s.c_str());
		}
		else
		{
			CODEC_TRACE("custom[%s]=%s", name, s.c_str());
		}
	}

	void on_error(char const* err)
	{
		++num_on_error;
		if (m_factor)
		{
			std::printf("ERROR: %s\n", err);
		}
		else
		{
			CODEC_TRACE("ERROR: %s", err);
		}
	}
};

struct string_sink
{
	static constexpr int m_factor = 2;
	char sz[16*1024];
	char* psz = sz;
	//sink() { clear(); }

	std::size_t space() const { return sizeof(sz) - (psz - sz);}
	//void clear() { psz = sz; *psz = '\0';}

	void on_value(std::size_t depth, char const* name, std::size_t value)
	{
		std::snprintf(psz, space(), "%*c%s : %zu (%zXh)\n", int(m_factor*depth), ' ', name, value, value);
	}

	template <class STR>
	auto on_value(std::size_t depth, char const* name, STR const& value) -> decltype(value.size(), value.data(), void())
	{
		std::snprintf(psz, space(), "%*c%s :", int(m_factor*depth), ' ', name);
		for (auto it = value.data(), ite = it + value.size(); it != ite; ++it)
		{
			std::snprintf(psz, space(), " %02X", *it);
		}
		std::snprintf(psz, space(), " (%zu)\n", value.size());
	}

	void on_container(std::size_t depth, char const* name)
	{
		std::snprintf(psz, space(), "%*c%s\n", int(m_factor*depth), ' ', name);
	}

	void on_custom(std::size_t depth, char const* name, std::string const& s)
	{
		std::snprintf(psz, space(), "%*c%s : %s\n", int(m_factor*depth), ' ', name, s.c_str());
	}

	void on_error(char const* err)
	{
		std::snprintf(psz, space(), "ERROR: %s\n", err);
	}
};

std::string get_printed(auto const& msg)
{
	string_sink s;
	med::print_all(s, msg);
	return s.sz;
}

template <class MSG, class RANGE>
void check_decode(MSG const& msg, RANGE const& binary)
{
	MSG decoded_msg;
	auto const size = binary.get_offset();
	ASSERT_GT(size, 0);
	auto const* data = binary.get_start();
	med::decoder_context ctx{data, size};
	CODEC_TRACE("--->>> DECODE <<<--- : %s", med::name<MSG>());
	decode(med::octet_decoder{ctx}, decoded_msg);
#if 1 //re-encode to check binary buffer
	uint8_t buffer[1024];
	ASSERT_LE(size, sizeof(buffer));
	med::encoder_context ectx{buffer, size};
	CODEC_TRACE("--->>> ENCODE <<<--- : %s", med::name<MSG>());
	encode(med::octet_encoder{ectx}, decoded_msg);
	ASSERT_STREQ(as_string(binary), as_string(ectx.buffer()));
#endif
#if 0 //TODO: fails strange for GTPC...
	ASSERT_TRUE(msg == decoded_msg)
		<< "\nEncoded:\n" << get_printed(msg)
		<< "\nDecoded:\n" << get_printed(decoded_msg);
#else
	(void)msg;
#endif
};
