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
		class ALLOCATOR = const null_allocator,
		class BUFFER = buffer<uint8_t>
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

	constexpr encoder_context(void* p, size_t s, allocator_type* a = nullptr) noexcept
		: detail::allocator_holder<allocator_type>{a} { reset(p, s); }

	template <typename T, size_t SIZE>
	explicit constexpr encoder_context(T (&p)[SIZE], allocator_type* a = nullptr) noexcept
		: encoder_context(p, sizeof(p), a) {}

	constexpr buffer_type& buffer() noexcept            { return m_buffer; }
	constexpr buffer_type const& buffer()const noexcept { return m_buffer; }

	template <typename... Ts>
	constexpr void reset(Ts... args) noexcept
	{
		buffer().reset(args...);
		m_snapshot = nullptr;
	}

	/**
	 * Stores the buffer snapshot
	 * @param snap
	 */
	constexpr void put_snapshot(SNAPSHOT snap)
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
		constexpr bool validate_length(size_t s) const
		{
			CODEC_TRACE("%s(%zu ? %zu)=%d", __FUNCTION__, m_length, s, m_length == s);
			return m_length == s;
		}

	private:
		friend class encoder_context;
		constexpr snap_s(state_t const& st, size_t len) : state_t{st}, m_length{len} {}
		constexpr snap_s() = default;

		size_t m_length{};
	};

	/**
	 * Finds snapshot of the buffer state for the IE
	 * @details Used in encoder to set the buffer state before updating
	 * @param IE to retrieve its snapshot
	 * @return snapshot or empty snapshot if not found
	 */
	template <class IE>
	constexpr snap_s get_snapshot(IE const&) const
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
