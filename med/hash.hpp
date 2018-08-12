#pragma once
/*
@file
string hash computation utils

@copyright Denis Priyomov 2018
Distributed under the MIT License
(See accompanying file LICENSE or visit https://github.com/cppden/ctstring)
*/

#include <cstdint>
#include <string_view>

namespace med {

template <typename VALUE = uint64_t>
class hash
{
public:
	using value_type = VALUE;
	static constexpr value_type init = 5381;

	static constexpr value_type compute(std::string_view const& sv)
	{
		return const_hash_impl(sv.data() + sv.size() - 1, sv.size());
	}

	static constexpr value_type update(char const c, value_type hval)
	{
		return ((hval << 5) + hval) + std::size_t(c); //33*hash + c
	}

	//Fowler–Noll–Vo : http://isthe.com/chongo/tech/comp/fnv/
	// constexpr unsigned hash(int n=0, unsigned h=2166136261)
	// {
	// 	return n == size ? h : hash(n+1,(h * 16777619) ^ (sv[n]));
	// }

private:
	static constexpr value_type const_hash_impl(char const* end, std::size_t count)
	{
		return count > 0
				? value_type(end[0]) + 33 * const_hash_impl(end - (count > 1 ? 1 : 0), count - 1)
				: init;
	}


};

} //end: namespace med

