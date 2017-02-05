/**
@file
simplest allocator in fixed buffer

@copyright Denis Priyomov 2016-2017
Distributed under the MIT License
(See accompanying file LICENSE or visit https://github.com/cppden/med)
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

	void reset()                            { m_begin = m_start; }

	void reset(void* data, std::size_t size)
	{
		m_start = static_cast<uint8_t*>(data);
		m_end   = m_start ? m_start + size : m_start;
		reset();
	}

	template <typename T, std::size_t SIZE>
	void reset(T (&data)[SIZE])             { reset(data, SIZE * sizeof(T)); }

	template <class T>
	T* allocate()
	{
		std::size_t sz = size();
		void* p = m_begin;
		if (std::align(alignof(T), sizeof(T), p, sz))
		{
			T* result = new (p) T{};
			m_begin = static_cast<uint8_t*>(p) + sizeof(T);
			return result;
		}
		m_errCtx.allocError(name<T>(), sizeof(T));
		return nullptr;
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


template <class, class Enable = void >
struct is_allocator : std::false_type { };
template <class T>
struct is_allocator<T,
	std::enable_if_t<
		std::is_same<int*, decltype(std::declval<T>().template allocate<int>()) >::value
	>
> : std::true_type { };
template <class T>
constexpr bool is_allocator_v = is_allocator<T>::value;

template <class T>
inline auto get_allocator_ptr(T& t) -> std::enable_if_t<is_allocator_v<T>, T*>
{
	return &t;
}
template <class T>
inline auto get_allocator_ptr(T* pt) -> std::enable_if_t<is_allocator_v<T>, T*>
{
	return pt;
}
template <class T>
inline auto get_allocator_ptr(T& t) -> std::add_pointer_t<decltype(t.get_allocator())>
{
	auto& allocator = t.get_allocator();
	static_assert(is_allocator<decltype(allocator)>::value, "IS NOT ALLOCATOR!");
	return &allocator;
}
template <class T>
inline auto get_allocator_ptr(T* pt) -> std::add_pointer_t<decltype(pt->get_allocator())>
{
	auto& allocator = pt->get_allocator();
	static_assert(is_allocator<decltype(allocator)>::value, "IS NOT ALLOCATOR!");
	return &allocator;
}


} //end: namespace med
