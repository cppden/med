#include <gtest/gtest.h>
#include <optional>

#include "meta/typelist.hpp"


TEST(meta, index)
{
	using list_t = med::meta::typelist<int, char, double>;
	static_assert(med::meta::list_size_v<list_t> == 3);
	static_assert(med::meta::list_index_of_v<int, list_t> == 0);
	static_assert(med::meta::list_index_of_v<char, list_t> == 1);
	static_assert(med::meta::list_index_of_v<double, list_t> == 2);
	static_assert(med::meta::list_index_of_v<std::nullopt_t, list_t> == med::meta::list_size_v<list_t>);
}

template <class T> struct err;


TEST(meta, append)
{
	using list_t = med::meta::typelist<int, char>;
	static_assert(std::is_same_v<
			med::meta::typelist<int, char, double, float, void>,
			med::meta::append_t<list_t, med::meta::typelist<double, float>, med::meta::typelist<void>>
	>);
}

TEST(meta, transform)
{
	using list_t = med::meta::typelist<int, char>;
	static_assert(
		std::is_same_v<
			med::meta::typelist<int*, char*>,
			med::meta::transform_t<list_t, std::add_pointer>
		>);
}


template <class V, bool constructed>
struct tag_of
{
	static constexpr auto c = constructed;
	V value;
};

template <class T, class I>
struct make_t
{
	static constexpr auto constructed = I::value;
	using type = tag_of<T, constructed>;
};


template <std::size_t I, std::size_t N>
struct not_last
{
	using type = std::bool_constant<I < (N-1)>;
};


TEST(meta, interleave)
{
	static_assert(
		std::is_same_v<
			med::meta::typelist<tag_of<char, true>, tag_of<double, true>, tag_of<int, false>>,
			med::meta::transform_indexed_t<med::meta::typelist<char, double, int>, make_t, not_last>
		>);

	static_assert(
		std::is_same_v<
			med::meta::typelist<tag_of<float, false>>,
			med::meta::transform_indexed_t<med::meta::typelist<float>, make_t, not_last>
		>);

	using list1 = med::meta::typelist<char, int>;
	using list2 = med::meta::typelist<float, double>;
	static_assert(
		std::is_same_v<
			med::meta::typelist<char, float, int, double>,
			med::meta::interleave_t<list1, list2>
		>);
	static_assert(
		std::is_same_v<
			med::meta::typelist<char, float, int, float>,
			med::meta::interleave_t<list1, float>
		>);
}
