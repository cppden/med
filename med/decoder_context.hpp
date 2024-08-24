/**
@file
context for decoding

@copyright Denis Priyomov 2016-2017
Distributed under the MIT License
(See accompanying file LICENSE or visit https://github.com/cppden/med)
*/

#pragma once

#include "allocator.hpp"
#include "buffer.hpp"
#include "snapshot.hpp"

namespace med {

template <
		class ALLOCATOR = const null_allocator,
		class BUFFER = buffer<uint8_t const>
		>
class decoder_context : public detail::allocator_holder<ALLOCATOR>
{
public:
	using allocator_type = ALLOCATOR;
	using buffer_type = BUFFER;
	using state_t = typename buffer_type::state_type;

	decoder_context() noexcept : decoder_context(nullptr, 0){}
	decoder_context(void const* p, size_t s, allocator_type* a = nullptr) noexcept
		: detail::allocator_holder<allocator_type>{a} { reset(p, s); }
	template <typename T, size_t SIZE>
	decoder_context(T const (&p)[SIZE], allocator_type* a = nullptr) noexcept
		: decoder_context(p, sizeof(p), a) {}
	template <typename T>
	decoder_context(std::span<T> p, allocator_type* a = nullptr) noexcept
		: decoder_context(p.data(), p.size_bytes(), a) {}

	buffer_type& buffer() noexcept          { return m_buffer; }

	template <typename... Ts>
	constexpr void reset(Ts&&... args)noexcept{ buffer().reset(std::forward<Ts>(args)...); }

private:
	decoder_context(decoder_context const&) = delete;
	decoder_context& operator=(decoder_context const&) = delete;

	buffer_type     m_buffer;
};

} //namespace med
