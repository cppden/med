/**
@file
simplest allocator in fixed buffer

@copyright Denis Priyomov 2016-2017
Distributed under the MIT License
(See accompanying file LICENSE or visit https://github.com/cppden/med)
*/

#pragma once

#include <memory>

#include "name.hpp"
#include "exception.hpp"

namespace med {

struct null_allocator
{
	[[nodiscard]]
	void* allocate(std::size_t bytes, std::size_t /*alignment*/) const
	{
		MED_THROW_EXCEPTION(out_of_memory, __FUNCTION__, bytes);
	}
};

template <typename T, class ALLOCATOR, class... ARGs>
T* create(ALLOCATOR& alloc, ARGs&&... args)
{
	void* p = alloc.allocate(sizeof(T), alignof(T));
	if (!p) { MED_THROW_EXCEPTION(out_of_memory, name<T>(), sizeof(T)); }
	return new (p) T{std::forward<ARGs>(args)...};
}

namespace detail {

template <class T>
struct allocator_holder
{
	using allocator_type = T;
	explicit constexpr allocator_holder(allocator_type* p) : m_ref{*p} {}
	constexpr allocator_type& get_allocator() { return m_ref; }

private:
	T& m_ref;
};

template <>
struct allocator_holder<const null_allocator>
{
	static constexpr null_allocator instance{};

	using allocator_type = const null_allocator;
	explicit constexpr allocator_holder(allocator_type*)  {}
	constexpr allocator_type& get_allocator() { return instance; }
};

}

class allocator
{
public:
	allocator(allocator const&) = delete;
	allocator& operator=(allocator const&) = delete;
	constexpr allocator() noexcept = default;
	constexpr allocator(void* data, std::size_t size) { reset(data, size); }
	template <typename T, std::size_t SIZE>
	explicit constexpr allocator(T (&data)[SIZE])     { reset(data); }

	/**
	 * Resets to the current allocation buffer
	 */
	constexpr void release() noexcept                 { m_begin = m_start; m_size = m_total; }

	/**
	 * Resets to new allocation buffer
	 * @param data start of allocation buffer
	 * @param size size of allocation buffer
	 */
	constexpr void reset(void* data, std::size_t size) noexcept
	{
		m_start = data;
		m_total = size;
		release();
	}

	template <typename T, std::size_t SIZE>
	constexpr void reset(T (&data)[SIZE]) noexcept   { reset(data, SIZE * sizeof(T)); }

	/**
	 * Allocates T from the beginning of current free buffer space
	 * @return pointer to instance of T or nullptr/throw when out of space
	 */
	[[nodiscard]]
	void* allocate(std::size_t bytes, std::size_t alignment) noexcept
	{
		void* p = std::align(alignment, bytes, m_begin, m_size);
		//void* p = salign(alignment, bytes, m_begin, m_size);
		if (p)
		{
			m_begin = (uint8_t*)m_begin + bytes;
			m_size -= bytes;
		}
		return p;
	}

private:
#if 0
	static constexpr void* salign(size_t __align, size_t __size, void*& __ptr, size_t& __space) noexcept
	{
		if (__space < __size) return nullptr;
		const auto __intptr = std::bit_cast<uintptr_t>(__ptr);
		const auto __aligned = (__intptr - 1u + __align) & -__align;
		const auto __diff = __aligned - __intptr;
		if (__diff > (__space - __size))
		{
			return nullptr;
		}
		else
		{
			__space -= __diff;
			return __ptr = std::bit_cast<void*>(__aligned);
		}
	}
#endif

	void*       m_begin {};
	std::size_t m_size{};
	void*       m_start {};
	std::size_t m_total {};
};


template <class, class Enable = void >
struct is_allocator : std::false_type { };
template <class T>
struct is_allocator<T,
	std::enable_if_t<
		std::is_same_v<void*, decltype(std::declval<T>().allocate(std::size_t(1), std::size_t(1)))>
	>
> : std::true_type { };
template <class T>
constexpr bool is_allocator_v = is_allocator<T>::value;


template <class T>
constexpr auto get_allocator(T& t) -> std::enable_if_t<is_allocator_v<T>, T&>
{
	return t;
}
template <class T>
constexpr auto get_allocator(T* pt) -> std::enable_if_t<is_allocator_v<T>, T&>
{
	return *pt;
}
template <class T>
constexpr auto get_allocator(T& t) -> std::add_lvalue_reference_t<decltype(t.get_allocator())>
{
	auto& allocator = t.get_allocator();
	static_assert(is_allocator_v<decltype(allocator)>, "IS NOT ALLOCATOR!");
	return allocator;
}
template <class T>
constexpr auto get_allocator(T* pt) -> std::add_lvalue_reference_t<decltype(pt->get_allocator())>
{
	auto& allocator = pt->get_allocator();
	static_assert(is_allocator_v<decltype(allocator)>, "IS NOT ALLOCATOR!");
	return allocator;
}

} //end: namespace med
