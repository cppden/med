/*!
@file
TODO: define.

@copyright Denis Priyomov 2016
Distributed under the MIT License
(See accompanying file LICENSE or copy at https://opensource.org/licenses/MIT)
*/

#pragma once


#include "encode.hpp"
#include "error.hpp"
#include "name.hpp"

/*
struct SinkSample
{
	template <typename T>
	void on_value(std::size_t depth, char const* name, T const& value)
	{
		printf("%*c%s=%u (%Xh)\n", depth, ' ', name, value, value);
	}

	void on_custom(std::size_t depth, char const* name, std::string const& s)
	{
		printf("%*c%s=%s\n", depth, ' ', name, s.c_str());
	}
};
*/

namespace med {

//MAX_LINE - max length of a single output of IE.print
template <class SINK, std::size_t MAX_LINE>
class printer
{
	template <typename T>
	struct has_print
	{
		template <class C, class P>
		static constexpr auto test(P* p) -> decltype(std::declval<C>().print(*p), int{})
		{
			return 2;
		}

		template <class C, class P>
		std::enable_if_t<!std::is_same<void, decltype(std::declval<C>().print())>::value, int>
		static constexpr test(P* p)
		{
			return 1;
		}
		template <class C, class P>
		static constexpr int test(...)
		{
			return 0;
		}
		static const int value = test<T, char[16]>(nullptr);
	};

public:

	class null_encoder
	{
	public:
		template <class IE>
		std::enable_if_t<!has_name<IE>::value, bool>
		constexpr operator()(printer&, IE const&)
		{
			return true;
		}

		template <class IE>
		std::enable_if_t<has_name<IE>::value, bool>
		operator()(printer& me, IE const& ie)
		{
			return me.print(ie);
		}
	};

	class container_encoder
	{
	public:
		//treat un-named container as logical group -> depth not changing
		template <class IE>
		std::enable_if_t<!has_name<IE>::value, bool>
		operator()(printer& me, IE const& ie)
		{
			return ie.encode(me);
		}

		template <class IE>
		std::enable_if_t<has_name<IE>::value, bool>
		operator()(printer& me, IE const& ie)
		{
			CODEC_TRACE("cenc: named [%s]", class_name<IE>());
			return print_named(me, ie);
		}

	private:
		//customized print 2
		template <class IE>
		std::enable_if_t<has_print<IE>::value == 2, bool>
		print_named(printer& me, IE const& ie)
		{
			char sz[MAX_LINE];
			ie.print(sz);
			me.m_sink.on_custom(me.m_depth, name<IE>(), sz);
			return true;
		}

		//customized print 1
		template <class IE>
		std::enable_if_t<has_print<IE>::value == 1, bool>
		print_named(printer& me, IE const& ie)
		{
			me.m_sink.on_custom(me.m_depth, name<IE>(), ie.print());
			return true;
		}

		//no customized print
		template <class IE>
		std::enable_if_t<has_print<IE>::value == 0, bool>
		print_named(printer& me, IE const& ie)
		{
			me.m_sink.on_container(me.m_depth, name<IE>());
			next_depth level{ &me };
			return level && ie.encode(me);
		}

		class next_depth
		{
		public:
			explicit next_depth(printer* p) noexcept
				: m_depth(p->m_depth++)
				, m_that(p)
			{
				CODEC_TRACE("next_depth -> %zu", p->m_depth);
			}

			next_depth(next_depth&& rhs) noexcept
				: m_depth{ rhs.m_depth }
				, m_that{ rhs.m_that }
			{
				rhs.m_that = nullptr;
			}

			next_depth& operator= (next_depth&& rhs) noexcept
			{
				m_that = rhs.m_that;
				m_depth = rhs.m_depth;
				rhs.m_that = nullptr;
				return *this;
				CODEC_TRACE("next_depth >> %zu", m_depth);
			}

			~next_depth()
			{
				if (m_that)
				{
					m_that->m_depth = m_depth;
					CODEC_TRACE("next_depth <- %zu", m_depth);
				}
			}

			explicit operator bool() const
			{
				CODEC_TRACE("next_depth ? %p max=%zu > %zu", m_that, m_that->m_max_depth, m_that->m_depth);
				return 0 == m_that->m_max_depth || m_that->m_max_depth > m_that->m_depth;
			}

		private:
			next_depth(next_depth const&) = delete;
			next_depth& operator= (next_depth const&) = delete;

			std::size_t  m_depth;
			printer*     m_that;
		};
	};

	printer(SINK&& sink, std::size_t max_depth) noexcept
		: m_sink{ std::forward<SINK>(sink) }
		, m_max_depth{ max_depth }
	{
	}

	//state
	constexpr void operator() (med::SNAPSHOT const&) { }
	//errors
	constexpr void operator() (error e, char const*, std::size_t, std::size_t = 0, std::size_t = 0) { }
	//length encoder
	template <int DELTA> constexpr bool operator() (placeholder::_length<DELTA> const&) { return true; }

	//customized print 2
	template <class IE, class IE_TYPE>
	std::enable_if_t<has_print<IE>::value == 2, bool>
	operator() (IE const& ie, IE_TYPE const&)
	{
		char sz[MAX_LINE];
		ie.print(sz);
		m_sink.on_custom(m_depth, name<IE>(), sz);
		return true;
	}

	//customized print 1
	template <class IE, class IE_TYPE>
	std::enable_if_t<has_print<IE>::value == 1, bool>
	operator() (IE const& ie, IE_TYPE const&)
	{
		m_sink.on_custom(m_depth, name<IE>(), ie.print());
		return true;
	}

	//regular print
	template <class IE, class IE_TYPE>
	std::enable_if_t<!has_print<IE>::value && std::is_base_of<PRIMITIVE, IE_TYPE>::value, bool>
	operator() (IE const& ie, IE_TYPE const& ie_type)
	{
		return print(ie);
	}

private:
	friend class container_encoder;
	friend class null_encoder;

	//named PRIMITIVE
	template <class IE>
	std::enable_if_t<has_name<IE>::value, bool>
	print(IE const& ie)
	{
		if (ie.is_set())
		{
			m_sink.on_value(m_depth, IE::name(), ie.get());
			return true;
		}
		return false;
	}

	//un-named PRIMITIVE
	template <class IE>
	std::enable_if_t<!has_name<IE>::value, bool>
	constexpr print(IE const& ie)
	{
		return ie.is_set();
	}

	SINK        m_sink;
	std::size_t m_depth {0};
	std::size_t const m_max_depth;
};

template <class SINK, class IE>
void print(SINK&& sink, IE const& ie, std::size_t max_depth = 0)
{
	encode(printer<SINK, 256>{std::forward<SINK>(sink), max_depth}, ie);
}

} //namespace med
