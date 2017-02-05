/**
@file
context for encoding

@copyright Denis Priyomov 2016-2017
Distributed under the MIT License
(See accompanying file LICENSE or visit https://github.com/cppden/med)
*/

#pragma once


#include "allocator.hpp"
#include "buffer.hpp"
#include "error_context.hpp"
#include "debug.hpp"

namespace med {

template < class BUFFER = buffer<uint8_t*>, class ALLOCATOR = allocator >
class encoder_context
{
public:
	using allocator_type = ALLOCATOR;
	using buffer_type = BUFFER;
	using state_t = typename buffer_type::state_type;

private:
	struct snapshot_s
	{
		SNAPSHOT  snapshot;
		state_t   state;
	};

public:
	enum { snapshot_size = sizeof(snapshot_s) };

	encoder_context(void* data, std::size_t size, std::size_t alloc_size = inf(), void* snap_data = nullptr, std::size_t snap_size = 0)
		: m_allocator{ m_errCtx }
		, m_max_snapshots{ static_cast<uint16_t>(snap_size/sizeof(snapshot_s)) }
	{
		//NOTE: max floored above is corrent since alignment less than sizeof
		if (std::align(alignof(snapshot_s), sizeof(snapshot_s), snap_data, snap_size))
		{
			m_snapshot = static_cast<snapshot_s*>(snap_data);
		}

		reset(data, size, alloc_size);
	}

	template <typename T, std::size_t SIZE>
	explicit encoder_context(T (&buf)[SIZE], std::size_t alloc_size = inf(), void* snap_data = nullptr, std::size_t snap_size = 0)
		: encoder_context(buf, sizeof(buf), alloc_size, snap_data, snap_size)
	{
	}

	template <typename T0, std::size_t SIZE, typename T1, std::size_t SNAPSIZE>
	encoder_context(T0 (&buf)[SIZE], T1 (&snap_data)[SNAPSIZE], std::size_t alloc_size = inf())
		: encoder_context(buf, sizeof(buf), alloc_size, snap_data, sizeof(snap_data))
	{
	}

	encoder_context(encoder_context const&) = delete;
	encoder_context& operator=(encoder_context const&) = delete;

	buffer_type& buffer()                   { return m_buffer; }
	error_context& error_ctx()              { return m_errCtx; }
	explicit operator bool() const          { return static_cast<bool>(error_ctx()); }
	allocator_type& get_allocator()         { return m_allocator; }

	void reset(void* data, std::size_t size, std::size_t alloc_size = inf())
	{
		if (alloc_size)
		{
			if (size > alloc_size)
			{
				size -= alloc_size;
			}
			else //a wild guess option
			{
				size /= 2;
				alloc_size = size;
			}
		}

		m_allocator.reset(static_cast<uint8_t*>(data) + size, alloc_size);
		m_buffer.reset(static_cast<typename buffer_type::pointer>(data), size);

		error_ctx().reset();
		m_idx_snapshot = 0;
	}

	void reset()
	{
		m_allocator.reset();
		m_buffer.reset();

		error_ctx().reset();
		m_idx_snapshot = 0;
	}

	void snapshot(SNAPSHOT const& snap)
	{
		if (m_idx_snapshot < m_max_snapshots)
		{
			CODEC_TRACE("snapshot @%u/%u %p{%zu}", m_idx_snapshot, m_max_snapshots, snap.id, snap.size);
			auto& ss = m_snapshot[m_idx_snapshot++];
			ss.snapshot = snap;
			ss.state = m_buffer.get_state();
		}
		else
		{
			CODEC_TRACE("WARNING: all %u slots used", m_max_snapshots);
			error_ctx().set_warning(warning::OVERFLOW);
		}
	}

	class snap_s : public state_t
	{
	public:
		operator std::size_t() const          { return m_length; }

	private:
		friend class encoder_context;
		snap_s(state_t const& ss, std::size_t len) : state_t{ss}, m_length{len} {}
		snap_s() : state_t{}, m_length{} {}

		std::size_t  m_length;
	};

	template <class IE>
	snap_s snapshot(IE const&) const
	{
		for (snapshot_s const& ss : *this)
		{
			if (ss.snapshot.id == snapshot_id<IE>) { return snap_s{ss.state, ss.snapshot.size}; }
		}
		return snap_s{};
	}

private:
	using const_iterator = snapshot_s const*;
	const_iterator begin() const              { return m_snapshot; }
	const_iterator end() const                { return begin() + m_idx_snapshot; }

	error_context  m_errCtx;
	buffer_type    m_buffer;
	allocator_type m_allocator;

	snapshot_s*    m_snapshot{ nullptr };
	uint16_t const m_max_snapshots;
	uint16_t       m_idx_snapshot{ 0 };
};

} //namespace med
