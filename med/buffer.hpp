/*!
@file
TODO: define.

@copyright Denis Priyomov 2016
Distributed under the MIT License
(See accompanying file LICENSE or copy at https://opensource.org/licenses/MIT)
*/

#pragma once

#include <cstddef>
#include <cstdint>

#include "debug.hpp"


namespace med {

template <typename PTR>
class buffer
{
public:
	using pointer = PTR;

	class state_type
	{
	public:
		explicit operator bool() const  { return nullptr != cursor; }
		void reset(PTR p = nullptr)     { cursor = p; }
		operator PTR() const            { return cursor; }

		friend std::ptrdiff_t operator- (state_type const& rhs, state_type const lhs)
		{
			return rhs.cursor - lhs.cursor;
		}

	private:
		friend class buffer;

		PTR cursor; //TODO: use offset? compare performance vs ptr
	};

	class size_state
	{
	public:
		size_state() noexcept
			: m_buf{ nullptr }
			, m_end{ nullptr }
		{
		}

		size_state(size_state&& rhs) noexcept
			: m_buf{ rhs.m_buf }
			, m_end{ rhs.m_end }
		{
			rhs.m_end = nullptr;
		}

		size_state& operator= (size_state&& rhs)
		{
			m_buf = rhs.m_buf;
			m_end = rhs.m_end;
			rhs.m_end = nullptr;
			return *this;
		}

		~size_state()
		{
			if (m_end)
			{
				m_buf->m_end = m_end;
				CODEC_TRACE("restored end to %p: %s", m_end, m_buf->toString());
			}
		}

		std::size_t size() const                { return m_buf->size(); }
		explicit operator bool() const          { return nullptr != m_end; }

	private:
		friend class buffer;

		size_state(buffer* buf, std::size_t size) noexcept
			: m_buf{ buf }
			, m_end{ buf->end() }
		{
			m_buf->m_end = m_buf->begin() + size;
			if (m_buf->end() > m_end)
			{
				CODEC_TRACE("INVALID END BEOYOND CURRENT BY %zd", m_buf->end() - m_end);
				m_buf->m_end = m_end;
				m_end = nullptr;
			}
			else
			{
				CODEC_TRACE("changed size to %zu ends at %p (was %p): %s", size, m_buf->end(), m_end, m_buf->toString());
			}
		}

		size_state(size_state const&) = delete;
		size_state(size_state&) = delete;
		size_state& operator= (size_state const&) = delete;
		size_state& operator= (size_state&) = delete;

		buffer* m_buf;
		PTR     m_end;
	};


	void reset(pointer data, std::size_t size)
	{
		if (data && size)
		{
			m_start = data;
			m_end   = m_start + size;
		}
		m_state.reset(m_start);
	}

	state_type get_state() const            { return m_state; }
	void set_state(state_type const& st)    { m_state = st; }

	bool push_state()
	{
		if (size() > 0)
		{
			m_store = m_state;
			CODEC_TRACE("PUSH_STATE: %s", toString());
			return true;
		}
		else
		{
			m_store.reset();
			return false;
		}
	}

	void pop_state()
	{
		if (m_store)
		{
			m_state = m_store;
			m_store.reset();
			CODEC_TRACE("POP_STATE: %s", toString());
		}
	}

	std::size_t get_offset() const          { return begin() - m_start; }
	std::size_t size() const                { return end() - begin(); }

	pointer advance(int delta)
	{
		pointer p = nullptr;
		if (delta >= 0 && size() >= std::size_t(delta))
		{
			p = begin();
			m_state.cursor += delta;
		}
		else if (delta < 0 && (begin() + delta) >= m_start)
		{
			m_state.cursor += delta;
			p = begin();
		}
		return p;
	}

	bool fill(std::size_t count, uint8_t value)
	{
		CODEC_TRACE("padding %zu bytes=%u", count, value);
		if (size() >= count)
		{
			while (count--) *m_state.cursor++ = value;
			return true;
		}
		return false;
	}

	pointer begin() const                   { return m_state.cursor; }
	pointer end() const                     { return m_end; }

	auto push_size(std::size_t size)        { return size_state{this, size}; }

#ifdef CODEC_TRACE_ENABLE
	char const* toString() const
	{
		static char sz[64];
		uint8_t const* p = begin();
		snprintf(sz, sizeof(sz), "%p@%zu+%zu: %02x %02x [%02x] %02x %02x", p, get_offset(), size()
			, p[-2], p[-1], p[0], p[1], p[2]);
		return sz;
	}
#endif

private:
	friend class size_state;

	state_type     m_state;
	pointer        m_end;
	pointer        m_start;
	state_type     m_store;
};

}	//end: namespace med

