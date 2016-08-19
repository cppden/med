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
#include "debug.hpp"

namespace med {

template < class BUFFER = buffer<uint8_t*> >
class encoder_context
{
public:
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

	encoder_context(uint8_t* data, std::size_t size, void* snap_data = nullptr, std::size_t snap_size = 0)
		: m_snapshot{ static_cast<snapshot_s*>(snap_data) }
		, m_max_snapshots{ static_cast<uint16_t>(snap_size/snapshot_size) }
	{
		reset(data, size);
	}

	template <std::size_t SIZE>
	explicit encoder_context(uint8_t (&buff)[SIZE], void* snap_data = nullptr, std::size_t snap_size = 0)
		: encoder_context(buff, SIZE, snap_data, snap_size)
	{
	}

	template <std::size_t SIZE, typename T, std::size_t SNAPSIZE>
	encoder_context(uint8_t (&buff)[SIZE], T (&snap_data)[SNAPSIZE])
		: encoder_context(buff, SIZE, snap_data, sizeof(snap_data))
	{
	}

	encoder_context(encoder_context const&) = delete;
	encoder_context& operator=(encoder_context const&) = delete;

	buffer_type& buffer()                   { return m_buffer; }
	error_context& error_ctx()              { return m_errCtx; }
	explicit operator bool() const          { return static_cast<bool>(m_errCtx); }

	void reset(void* data = nullptr, std::size_t size = 0)
	{
		m_buffer.reset(static_cast<uint8_t*>(data), size);
		m_errCtx.reset();
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
			m_errCtx.set_warning(warning::OVERFLOW);
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
	snap_s snapshot(IE const& ie) const
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

	snapshot_s*    m_snapshot;
	uint16_t const m_max_snapshots;
	uint16_t       m_idx_snapshot{ 0 };
};

} //namespace med
