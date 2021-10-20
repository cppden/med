/**
@file
buffer for encoding and decoding

@copyright Denis Priyomov 2016-2017
Distributed under the MIT License
(See accompanying file LICENSE or visit https://github.com/cppden/med)
*/

#pragma once

#include <cstddef>
#include <cstdint>
#include <algorithm>
#include <iostream>

#include "exception.hpp"
#include "name.hpp"

namespace med {

template <typename PTR>
class buffer
{
public:
	using pointer = PTR;
	using value_type = std::remove_const_t<std::remove_pointer_t<pointer>>;

	//captures current state of the buffer
	class state_type
	{
	public:
		explicit operator bool() const  { return nullptr != cursor; }
		void reset(pointer p = nullptr) { cursor = p; }
		operator pointer() const        { return cursor; }

		friend std::ptrdiff_t operator- (state_type const& rhs, state_type const lhs)
		{
			return rhs.cursor - lhs.cursor;
		}

	private:
		friend class buffer;

		pointer cursor {nullptr};
	};

	//changes the size (end) of the buffer memorizing current end to rollback in dtor
	class size_state
	{
	public:
		size_state() = default;
		size_state(size_state const&) = delete;
		size_state& operator= (size_state const&) = delete;

		size_state(size_state&& rhs) noexcept
			: m_buf{ rhs.m_buf }
			, m_end{ rhs.m_end }
			, m_pending{ rhs.m_pending }
		{
			rhs.m_end = nullptr;
		}

		size_state& operator= (size_state&& rhs) noexcept
		{
			m_buf = rhs.m_buf;
			m_end = rhs.m_end;
			rhs.m_end = nullptr;
			return *this;
		}

		~size_state()                           { restore_end(); }
		void restore_end()
		{
			if (!pending() && m_end)
			{
				CODEC_TRACE("%d: restored end %p->%p: %s", m_instance, (void*)m_buf->m_end, (void*)m_end, m_buf->toString());
				m_buf->m_end = m_end;
				m_end = nullptr;
			}
		}

		void commit(int delta)
		{
			if (pending())
			{
				m_pending = false;
				auto pend = m_end + delta;
				m_end = m_buf->m_end;
				m_buf->m_end = pend;
				CODEC_TRACE("%d: commit adjusted by %d end %p->%p: %s", m_instance, delta, (void*)m_end, (void*)m_buf->m_end, m_buf->toString());
			}
		}

		std::size_t size() const noexcept       { return m_buf ? m_buf->size() : 0; }
		explicit operator bool() const noexcept { return nullptr != m_end; }
		bool pending() const noexcept           { return m_buf && m_end && m_pending; }

	private:
		friend class buffer;

		size_state(buffer* buf, std::size_t size, bool commit_)
			: m_buf{ buf }
			, m_end{ buf->end() }
#ifdef CODEC_TRACE_ENABLE
			, m_instance{ s_instance_count++ }
#endif
		{
			if (auto* pend = buf->begin() + size; pend <= m_end) //within the current end of buffer
			{
				if (commit_)
				{
					buf->m_end = pend;
					CODEC_TRACE("%d: changed by %zu end %p->%p: %s", m_instance, size, (void*)m_end, (void*)buf->end(), buf->toString());
				}
				else
				{
					m_pending = true;
					m_end = pend;
					CODEC_TRACE("%d: pending change by %zu end %p->%p: %s", m_instance, size, (void*)buf->end(), (void*)m_end, buf->toString());
				}
			}
			else
			{
				m_buf = nullptr;
				MED_THROW_EXCEPTION(overflow, "end of buffer", pend - m_end);
//				MED_THROW_EXCEPTION(overflow, name<IE>(), len_value/*, m_decoder.ctx.buffer()*/)
			}
		}


		buffer*     m_buf {nullptr};
		pointer     m_end {nullptr};
		bool        m_pending {false};
#ifdef CODEC_TRACE_ENABLE
		int const   m_instance{-1};
		inline static int  s_instance_count = 0;
#endif
	};

	auto push_size(std::size_t s, bool c)  { return size_state{this, s, c}; }

	void reset()                           { m_state.reset(get_start()); }

	void reset(pointer data, std::size_t size)
	{
		m_start = data;
		m_state.reset(data);
		end(data + size);
	}

	state_type get_state() const           { return m_state; }
	void set_state(state_type const& st)   { m_state = st; }

	bool push_state()
	{
		if (not empty())
		{
			m_store = m_state;
			CODEC_TRACE("push_state: %s", toString());
			return true;
		}
		else
		{
			m_store.reset();
			return false;
		}
	}

	bool pop_state()
	{
		if (m_store)
		{
			m_state = m_store;
			m_store.reset();
			CODEC_TRACE("pop_state: %s", toString());
			return true;
		}
		return false;
	}

	pointer get_start() const              { return m_start; }
	std::size_t get_offset() const         { return begin() - get_start(); }
	std::size_t size() const               { return end() - begin(); }
	bool empty() const                     { return begin() >= end(); }
	explicit operator bool() const         { return !empty(); }

	template <class IE> void push(value_type v)
	{
		if (not empty()) { *m_state.cursor++ = v; }
		else { MED_THROW_EXCEPTION(overflow, name<IE>(), sizeof(value_type), *this) }
	}

	template <class IE> value_type pop()
	{
		if (not empty()) { return *m_state.cursor++; }
		else { MED_THROW_EXCEPTION(overflow, name<IE>(), sizeof(value_type), *this) }
	}

	template <class IE, int DELTA> pointer advance()
	{
		pointer p = nullptr;
		if constexpr (DELTA > 0)
		{
			if (size() >= std::size_t(DELTA))
			{
				p = begin();
				m_state.cursor += DELTA;
			}
			else
			{
				MED_THROW_EXCEPTION(overflow, name<IE>(), DELTA, *this)
			}
		}
		else if constexpr (DELTA < 0)
		{
			if (begin() + DELTA >= get_start())
			{
				m_state.cursor += DELTA;
				p = begin();
			}
			else
			{
				MED_THROW_EXCEPTION(overflow, name<IE>(), DELTA, *this)
			}
		}
		return p;
	}

	template <class IE = void> pointer advance(int delta)
	{
		pointer p = nullptr;
		if (delta >= 0 && size() >= std::size_t(delta))
		{
			p = begin();
			m_state.cursor += delta;
		}
		else if (delta < 0 && (begin() + delta) >= get_start())
		{
			m_state.cursor += delta;
			p = begin();
		}
		else if constexpr (not std::is_void_v<IE>)
		{
			MED_THROW_EXCEPTION(overflow, name<IE>(), delta, *this)
		}
		return p;
	}

	/// similar to advance but no bounds check
	void offset(int delta)                 { m_state.cursor += delta; }

	template <class IE> void fill(std::size_t count, uint8_t value)
	{
		//CODEC_TRACE("padding %zu bytes=%u", count, value);
		if (size() >= count)
		{
			while (count--) *m_state.cursor++ = value;
		}
		else
		{
			MED_THROW_EXCEPTION(overflow, name<IE>(), count, *this)
		}
	}

	pointer begin() const                  { return m_state.cursor; }
	pointer end() const                    { return m_end; }

	/**
	 * Adjusts the end of buffer pointer
	 * @details Used by allocator to adjust buffer space after allocation from back
	 * @param p new end of buffer
	 */
	void end(pointer p)                    { m_end = p > begin() ? p : begin(); }

#if 1
	char const* toString() const
	{
		static char sz[64];
		if constexpr (std::is_same_v<char, value_type>)
		{
			auto p = begin();
			auto const len = int(std::min(size(), std::size_t(16)));
			std::snprintf(sz, sizeof(sz), "#%zu+%zu=%.*s", size(), get_offset(), len, p);
		}
		else
		{
			int n = std::snprintf(sz, sizeof(sz), "#%zu+%zu=", size(), get_offset());
			auto from = std::max(-int(get_offset()), -4);
			auto p = begin() + from;
			for (auto const to = std::min(int(size()), from+10); from < to; ++from, ++p)
			{
				n += std::snprintf(sz+n, sizeof(sz)-n, p == begin() ? "[%02X]":"%02X", *p);
			}
		}
		return sz;
	}

	friend std::ostream& operator << (std::ostream& out, buffer const& buf)
	{
		return out << buf.toString();
	}
#else
	//seems everyone likes C++ streams but for some reason I find them way too ugly...
	//NOTE: left here only for comparison with handy printf
	friend std::ostream& operator << (std::ostream& out, buffer const& buf)
	{
		auto const save_flags = out.flags();

		out
			<< '[' << buf.size() << ']'
			<< '+' << buf.get_offset() << '=';
		if constexpr (std::is_same_v<char, value_type>)
		{
			auto const size = std::min(buf.size(), std::size_t(32));
			out << '=' << std::string_view{buf.begin(), size};
		}
		else
		{
			auto* p = buf.begin() - 4;
			for (int i = -4; i < 0; ++i)
			{
				out << std::hex << std::setfill('0') << std::setw(2) << +p[i];
			}
			out << '[' << std::hex << std::setfill('0') << std::setw(2) << +p[0] << ']';
			for (int i = 1; i < 5; ++i)
			{
				std::cout << std::hex << std::setfill('0') << std::setw(2) << +p[i];
			}
		}

		out.flags(save_flags);
		return out;
	}
#endif

private:
	friend class size_state;

	state_type     m_state;
	pointer        m_end;
	pointer        m_start {nullptr};
	state_type     m_store;
};

}	//end: namespace med


