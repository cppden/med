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
//#include <iomanip>
//#include <string_view>

#include "exception.hpp"
#include "name.hpp"

namespace med {

template <typename PTR>
class buffer
{
public:
	using pointer = PTR;
	using value_type = std::remove_const_t<std::remove_pointer_t<pointer>>;
	//static constexpr bool is_const = std::is_const_v<std::remove_pointer_t<pointer>>;

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

		~size_state()                           { restore_end(); }
		void restore_end()
		{
			if (m_end)
			{
				CODEC_TRACE("restored end %p->%p: %s", (void*)m_buf->m_end, (void*)m_end, m_buf->toString());
				m_buf->m_end = m_end;
				m_end = nullptr;
			}
		}

		std::size_t size() const                { return m_buf ? m_buf->size() : 0; }
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
				CODEC_TRACE("INVALID END ABOVE CURRENT BY %zd", m_buf->end() - m_end);
				m_buf->m_end = m_end;
				m_end = nullptr;
			}
			else
			{
				CODEC_TRACE("changed size to %zu ends at %p (was %p): %s", size, (void*)m_buf->end(), (void*)m_end, m_buf->toString());
			}
		}

		size_state(size_state const&) = delete;
		size_state& operator= (size_state const&) = delete;

		buffer* m_buf;
		pointer m_end;
	};


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

	template <class IE>
	void push(value_type v)
	{
		if (not empty()) { *m_state.cursor++ = v; }
		else { MED_THROW_EXCEPTION(overflow, name<IE>(), sizeof(value_type), *this) }
	}

	template <class IE>
	value_type pop()
	{
		if (not empty()) { return *m_state.cursor++; }
		else { MED_THROW_EXCEPTION(overflow, name<IE>(), sizeof(value_type), *this) }
	}

	template <class IE, int DELTA>
	pointer advance()
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

	template <class IE = void>
	pointer advance(int delta)
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

	template <class IE>
	void fill(std::size_t count, uint8_t value)
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

	auto push_size(std::size_t size)       { return size_state{this, size}; }

#if 1
	char const* toString() const
	{
		static char sz[64];
		auto p = begin();
		if constexpr (std::is_same_v<char, value_type>)
		{
			auto const len = int(std::min(size(), std::size_t(16)));
			std::snprintf(sz, sizeof(sz), "[%zu]+%zu=%.*s", size(), get_offset(), len, p);
		}
		else
		{
			std::snprintf(sz, sizeof(sz)
					, "[%zu]+%zu=%02x%02x%02x%02x[%02x]%02x%02x%02x%02x"
					, size(), get_offset(), p[-4],p[-3],p[-2],p[-1], p[0], p[1],p[2],p[3],p[4]);
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


