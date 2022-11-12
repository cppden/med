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
#include <limits>

#include "exception.hpp"
#include "name.hpp"

namespace med {

template <typename PTR, std::size_t LEN_DEPTH = 16>
class buffer
{
	using eob_index_t = uint8_t;
	static constexpr eob_index_t max_eob_index = std::numeric_limits<eob_index_t>::max();

public:
	using pointer = PTR;
	using value_type = std::remove_const_t<std::remove_pointer_t<pointer>>;

	constexpr buffer() noexcept = default;

	//captures current state of the buffer
	class state_type
	{
	public:
		constexpr explicit operator bool() const  { return nullptr != cursor; }
		constexpr void reset(pointer p = nullptr) { cursor = p; }
		constexpr operator pointer() const        { return cursor; }

		friend constexpr std::ptrdiff_t operator- (state_type const& rhs, state_type const lhs)
		{
			return rhs.cursor - lhs.cursor;
		}

	private:
		friend class buffer;

		pointer cursor{nullptr};
	};


	//changes the size (end) of the buffer memorizing current end to rollback in dtor
	class size_state
	{
	public:
		constexpr size_state() noexcept = default;
		size_state(size_state const&) = delete;
		size_state& operator= (size_state const&) = delete;

		constexpr size_state(size_state&& rhs) noexcept
			: m_buffer{ rhs.m_buffer }
			, m_index{ rhs.m_index }
			, m_commited{ rhs.m_commited }
		{
			rhs.clear();
		}

		constexpr size_state& operator= (size_state&& rhs) noexcept
		{
			m_buffer = rhs.m_buffer;
			m_index = rhs.m_index;
			m_commited = rhs.m_commited;
			rhs.clear();
			return *this;
		}

		~size_state()                                     { restore_end(); }

		constexpr void restore_end()                      { if (m_buffer) m_buffer->restore_end(*this); }
		constexpr void commit(int delta)                  { m_buffer->commit_end(*this, delta); }

		constexpr std::size_t size() const noexcept       { return m_buffer ? m_buffer->size() : 0; }
		explicit constexpr operator bool() const noexcept { return !empty(); }

	private:
		friend class buffer;
		constexpr size_state(buffer* buf, eob_index_t idx, bool commit_)
			: m_buffer{ buf }
			, m_index{ idx }
			, m_commited{ commit_ }
		{}

		constexpr bool empty() const noexcept             { return m_index == max_eob_index; }
		constexpr void clear() noexcept                   { m_index = max_eob_index; }

		buffer*     m_buffer{ nullptr };
		eob_index_t m_index{ max_eob_index };
		bool        m_commited{ false };
	};

	constexpr auto push_size(std::size_t size, bool commit_)
	{
		if (auto* pend = begin() + size; pend <= end()) //within the current end of buffer
		{
			pointer& ps = m_eob[m_eob_index];
			//TODO: check and throw out of range

			if (commit_)
			{
				ps = end();
				m_end = pend;
				CODEC_TRACE("%u: change by %zu end %p->%p: %s", m_eob_index, size, (void*)ps, (void*)pend, toString());
			}
			else
			{
				ps = pend;
				CODEC_TRACE("%u: change by %zu end PENDING %p->%p: %s", m_eob_index, size, (void*)end(), (void*)pend, toString());
			}

			return size_state{this, m_eob_index++, commit_};
		}
		else
		{
			MED_THROW_EXCEPTION(overflow, "end of buffer", pend - end());
		}
	}

	constexpr void reset()                           { m_state.reset(get_start()); }

	constexpr void reset(pointer data, std::size_t size)
	{
		m_start = data;
		m_state.reset(data);
		end(data + size);
	}

	constexpr state_type get_state() const           { return m_state; }
	constexpr void set_state(state_type const& st)   { m_state = st; }

	constexpr bool push_state()
	{
		if (not empty())
		{
			m_store = m_state;
			CODEC_TRACE("%s: %s", __FUNCTION__, toString());
			return true;
		}
		CODEC_TRACE("%s: %s", __FUNCTION__, toString());
		m_store.reset();
		return false;
	}

	constexpr void pop_state()
	{
		CODEC_TRACE("%s: stored=%d", __FUNCTION__, (bool)m_store);
		if (m_store)
		{
			m_state = m_store;
			m_store.reset();
			CODEC_TRACE("%s: %s", __FUNCTION__, toString());
		}
	}

	constexpr pointer get_start() const              { return m_start; }
	constexpr std::size_t get_offset() const         { return begin() - get_start(); }
	constexpr std::size_t size() const               { return end() - begin(); }
	constexpr bool empty() const                     { return begin() >= end(); }
	explicit constexpr operator bool() const         { return !empty(); }

	template <class IE> constexpr void push(value_type v)
	{
		if (not empty()) { *m_state.cursor++ = v; }
		else { MED_THROW_EXCEPTION(overflow, name<IE>(), sizeof(value_type), *this) }
	}

	template <class IE> constexpr value_type pop()
	{
		if (not empty()) { return *m_state.cursor++; }
		else { MED_THROW_EXCEPTION(overflow, name<IE>(), sizeof(value_type), *this) }
	}

	template <class IE, int DELTA> constexpr pointer advance()
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

	template <class IE = void> constexpr pointer advance(int delta)
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
	constexpr void offset(int delta)                 { m_state.cursor += delta; }

	template <class IE> constexpr void fill(std::size_t count, uint8_t value)
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

	constexpr pointer begin() const                  { return m_state.cursor; }
	constexpr pointer end() const                    { return m_end; }

	/**
	 * Adjusts the end of buffer pointer
	 * @details Used by allocator to adjust buffer space after allocation from back
	 * @param p new end of buffer
	 */
	constexpr void end(pointer p)                    { m_end = p > begin() ? p : begin(); }

#if 1
	char const* toString() const
	{
		static char sz[64];
		int n = std::snprintf(sz, sizeof(sz), "%p@#%zu+%zu=", (void*)begin(), size(), get_offset());
		auto from = std::max(-int(get_offset()), -4);
		auto p = begin() + from;
		for (auto const to = std::min(int(size()), from+10); from < to; ++from, ++p)
		{
			n += std::snprintf(sz+n, sizeof(sz)-n, p == begin() ? "[%02X]":"%02X", *p);
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

	constexpr void restore_end(size_state& ss)
	{
		if (ss.m_commited && !ss.empty())
		{
			pointer& ps = m_eob[ss.m_index];
			CODEC_TRACE("%u/%u: restored end %p->%p: %s", ss.m_index, m_eob_index, (void*)end(), (void*)ps, toString());
			m_end = ps;
			--m_eob_index; //TODO: may assert index is the last one: m_eob_index - 1 == ss.m_index?
			ss.clear();
		}
	}

	constexpr void commit_end(size_state& ss, int delta)
	{
		if (!(ss.m_commited || ss.empty()))
		{
			ss.m_commited = true;
			pointer& ps1 = m_eob[ss.m_index];
			ps1 += delta; //end to be set by dependent
			CODEC_TRACE("%u/%u: commit adjusted by %d end %p: %s", ss.m_index, m_eob_index, delta, (void*)ps1, toString());
			if (m_eob_index > ss.m_index + 1) //adjust the deeper level's ends
			{
				//TODO: can we have multiple pending commits?
				pointer& ps2 = m_eob[ss.m_index+1];
				std::swap(ps1, ps2);
			}
			else
			{
				m_end = ps1;
				//MED_THROW_EXCEPTION(overflow, "invalid commit", ss.m_index);
			}
		}
	}

	state_type     m_state{};
	pointer        m_end{};
	pointer        m_start {nullptr};
	state_type     m_store{};
	uint8_t        m_eob_index {0};
	pointer        m_eob[LEN_DEPTH]{};
};

}	//end: namespace med
