/*!
@file
TODO: define.

@copyright Denis Priyomov 2016
Distributed under the MIT License
(See accompanying file LICENSE or copy at https://opensource.org/licenses/MIT)
*/

#pragma once

#include <boost/noncopyable.hpp>

#include "buffer.hpp"
#include "error_context.hpp"

namespace med {


template < class BUFFER = buffer<uint8_t const*> >
class decoder_context
{
public:
	using buffer_type = BUFFER;

	explicit decoder_context(const void* data = nullptr, uint32_t size = 0)
	{
		reset(data, size);
	}

	template <std::size_t SIZE>
	explicit decoder_context(uint8_t const (&buff)[SIZE])
		: decoder_context(buff, SIZE)
	{
	}

	decoder_context(decoder_context const&) = delete;
	decoder_context& operator=(decoder_context const&) = delete;

	buffer_type& buffer()                   { return m_buffer; }
	error_context& error_ctx()              { return m_errCtx; }
	explicit operator bool() const          { return static_cast<bool>(m_errCtx); }

	void reset(const void* data = nullptr, uint32_t size = 0)
	{
		m_buffer.reset(static_cast<uint8_t const*>(data), size);
		m_errCtx.reset();
	}

	template <std::size_t SIZE>
	void reset(uint8_t const (&buff)[SIZE]) { reset(buff, SIZE); }

private:
	error_context  m_errCtx;
	buffer_type    m_buffer;
};

} //namespace med
