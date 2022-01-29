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
		class BUFFER = buffer<uint8_t const*>
		>
class decoder_context : public detail::allocator_holder<ALLOCATOR>
{
public:
	using allocator_type = ALLOCATOR;
	using buffer_type = BUFFER;
	using state_t = typename buffer_type::state_type;

	decoder_context(void const* data, std::size_t size, allocator_type* alloc = nullptr)
		: detail::allocator_holder<allocator_type>{alloc}
	{
		reset(data, size);
	}

	decoder_context()
		: decoder_context(nullptr, 0)
	{
	}

	template <typename T, std::size_t SIZE>
	decoder_context(T const (&data)[SIZE], allocator_type* alloc = nullptr)
		: decoder_context(data, sizeof(data), alloc)
	{
	}


	buffer_type& buffer()                   { return m_buffer; }

	void reset(void const* data, std::size_t size)
	{
		buffer().reset(static_cast<typename buffer_type::pointer>(data), size);
	}
	template <typename T, std::size_t SIZE>
	void reset(T const (&data)[SIZE])       { reset(data, sizeof(data)); }

	void reset()                            { buffer().reset(); }

private:
	decoder_context(decoder_context const&) = delete;
	decoder_context& operator=(decoder_context const&) = delete;

	buffer_type     m_buffer;
};

} //namespace med
