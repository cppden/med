/*!
@file
TODO: define.

@copyright Denis Priyomov 2016
Distributed under the MIT License
(See accompanying file LICENSE or copy at https://opensource.org/licenses/MIT)
*/

#pragma once

#include <memory>
#include "error_context.hpp"

namespace med {

class allocator
{
public:
	explicit allocator(error_context& err)
			: m_errCtx(err)
	{}

	void reset(void* data, std::size_t size)
	{
		if (data && size)
		{
			m_start = static_cast<uint8_t*>(data);
			m_end   = m_start + size;
		}
		m_begin = m_start;
	}

	void reset()
	{
		m_begin = m_start;
	}

	template <class T>
	T* allocate()
	{
		std::size_t sz = size();
		if (void* p = std::align(alignof(T), sizeof(T), p, sz))
		{
			T* result = new (p) T{};
			m_begin = static_cast<uint8_t*>(p) + sizeof(T);
			return result;
		}
		else
		{
			m_errCtx.allocError(name<T>(), sizeof(T));
			return nullptr;
		}
	}

private:
	std::size_t size() const
	{
		return m_end > m_begin ? std::size_t(m_end - m_begin) : 0;
	}

	error_context&  m_errCtx;
	uint8_t*        m_start {nullptr};
	uint8_t*        m_begin {nullptr};
	uint8_t*        m_end {nullptr};
};


} //end: namespace med
