/*!
@file
TODO: define.

@copyright Denis Priyomov 2016
Distributed under the MIT License
(See accompanying file LICENSE or copy at https://opensource.org/licenses/MIT)
*/

#pragma once

#include "allocator.hpp"
#include "buffer.hpp"
#include "error_context.hpp"

namespace med {

template < class BUFFER = buffer<uint8_t const*>, class ALLOCATOR = allocator >
class decoder_context
{
public:
	using allocator_type = ALLOCATOR;
	using buffer_type = BUFFER;

	explicit decoder_context(void const* data = nullptr, std::size_t size = 0, void* alloc_data = nullptr, std::size_t alloc_size = 0)
		: m_allocator{ m_errCtx }
	{
		reset(data, size, alloc_data, alloc_size);
	}

	template <std::size_t SIZE>
	explicit decoder_context(uint8_t const (&buff)[SIZE], void* alloc_data = nullptr, std::size_t alloc_size = 0)
		: decoder_context(buff, SIZE, alloc_data, alloc_size)
	{
	}

	buffer_type& buffer()                   { return m_buffer; }
	error_context& error_ctx()              { return m_errCtx; }
	explicit operator bool() const          { return static_cast<bool>(error_ctx()); }
	allocator_type& get_allocator()         { return m_allocator; }

	void reset(void const* data = nullptr, std::size_t size = 0, void* alloc_data = nullptr, std::size_t alloc_size = 0)
	{
		if (alloc_size)
		{
			m_allocator.reset(alloc_data, alloc_size);
		}
		else
		{
			m_allocator.reset();
		}

		buffer().reset(static_cast<uint8_t const*>(data), size);
		error_ctx().reset();
	}

private:
	decoder_context(decoder_context const&) = delete;
	decoder_context& operator=(decoder_context const&) = delete;

	error_context  m_errCtx;
	buffer_type    m_buffer;
	allocator_type m_allocator;
};

} //namespace med
