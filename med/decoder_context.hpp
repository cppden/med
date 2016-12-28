/*!
@file
TODO: define.

@copyright Denis Priyomov 2016
Distributed under the MIT License
(See accompanying file LICENSE or copy at https://opensource.org/licenses/MIT)
*/

#pragma once

#include "buffer.hpp"
#include "error_context.hpp"
#include "allocator.hpp"

namespace med {

template < class BUFFER = buffer<uint8_t const*>, class ALLOCATOR = allocator >
class decoder_context
{
public:
	using buffer_type = BUFFER;
	using allocator_type = ALLOCATOR;

	explicit decoder_context(const void* data = nullptr, uint32_t size = 0)
		: m_allocator{m_errCtx}
	{
		reset(data, size);
	}

	template <std::size_t SIZE>
	explicit decoder_context(uint8_t const (&buff)[SIZE])
		: decoder_context(buff, SIZE)
	{
	}

	buffer_type& buffer()                   { return m_buffer; }
	error_context& error_ctx()              { return m_errCtx; }
	explicit operator bool() const          { return static_cast<bool>(error_ctx()); }
	allocator_type& get_allocator()         { return m_allocator; }

	void reset(const void* data = nullptr, uint32_t size = 0)
	{
		buffer().reset(static_cast<uint8_t const*>(data), size);
		error_ctx().reset();
	}

	template <std::size_t SIZE>
	void reset(uint8_t const (&buff)[SIZE]) { reset(buff, SIZE); }

private:
	decoder_context(decoder_context const&) = delete;
	decoder_context& operator=(decoder_context const&) = delete;

	error_context  m_errCtx;
	buffer_type    m_buffer;
	allocator_type m_allocator;
};

} //namespace med
