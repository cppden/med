/**
@file
context for encoding

@copyright Denis Priyomov 2016-2017
Distributed under the MIT License
(See accompanying file LICENSE or visit https://github.com/cppden/med)
*/

#pragma once


#include "allocator.hpp"
#include "snapshot.hpp"
#include "buffer.hpp"
#include "debug.hpp"

namespace med {

template <
		class ALLOCATOR = null_allocator,
		class BUFFER = buffer<uint8_t*>
		>
class encoder_context : public detail::allocator_holder<ALLOCATOR>
{
public:
	using allocator_type = ALLOCATOR;
	using buffer_type = BUFFER;
	using state_t = typename buffer_type::state_type;

private:
	struct snapshot_s
	{
		SNAPSHOT    snapshot;
		state_t     state;
		snapshot_s* next;
	};

public:
	encoder_context(encoder_context const&) = delete;
	encoder_context& operator=(encoder_context const&) = delete;

	encoder_context(void* data, std::size_t size, allocator_type* alloc = nullptr)
		: detail::allocator_holder<allocator_type>{alloc}
	{
		reset(data, size);
	}

	template <typename T, std::size_t SIZE>
	explicit encoder_context(T (&buf)[SIZE], allocator_type* alloc = nullptr)
		: encoder_context(buf, sizeof(buf), alloc) {}

	buffer_type& buffer()                   { return m_buffer; }

	template <typename T, std::size_t SIZE>
	void reset(T (&data)[SIZE]) noexcept   { reset(data, SIZE * sizeof(T)); }

	void reset(void* data, std::size_t size) noexcept
	{
		m_buffer.reset(static_cast<typename buffer_type::pointer>(data), size);
		m_snapshot = nullptr;
	}

	void reset()
	{
		m_buffer.reset();
		m_snapshot = nullptr;
	}

	/**
	 * Stores the buffer snapshot
	 * @param snap
	 */
	void put_snapshot(SNAPSHOT snap)
	{
		CODEC_TRACE("snapshot %p{%zu}", static_cast<void const*>(snap.id), snap.size);
		snapshot_s* p = create<snapshot_s>(this->get_allocator());
		p->snapshot = snap;
		p->state = m_buffer.get_state();

		p->next = m_snapshot ? m_snapshot->next : nullptr;
		m_snapshot = p;
	}

	class snap_s : public state_t
	{
	public:
		bool validate_length(std::size_t s) const
		{
			CODEC_TRACE("%s(%zu ? %zu)=%d", __FUNCTION__, m_length, s, m_length == s);
			return m_length == s;
		}

	private:
		friend class encoder_context;
		snap_s(state_t const& st, std::size_t len) : state_t{st}, m_length{len} {}
		snap_s() : state_t{}, m_length{} {}

		std::size_t m_length;
	};

	/**
	 * Finds snapshot of the buffer state for the IE
	 * @details Used in encoder to set the buffer state before updating
	 * @param IE to retrieve its snapshot
	 * @return snapshot or empty snapshot if not found
	 */
	template <class IE>
	snap_s get_snapshot(IE const&) const
	{
		static_assert(std::is_base_of<with_snapshot, IE>(), "IE WITH with_snapshot EXPECTED");

		for (snapshot_s const* p = m_snapshot; p != nullptr; p = p->next)
		{
			if (p->snapshot.id == snapshot_id<IE>) { return snap_s{p->state, p->snapshot.size}; }
		}
		return snap_s{};
	}

private:
	using const_iterator = snapshot_s const*;

	buffer_type    m_buffer;
	snapshot_s*    m_snapshot{ nullptr };
};

} //namespace med
