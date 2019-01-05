/**
@file
special case of encoder to print IEs via user-provided sink

@copyright Denis Priyomov 2016-2017
Distributed under the MIT License
(See accompanying file LICENSE or visit https://github.com/cppden/med)
*/

#pragma once


#include "encode.hpp"
#include "exception.hpp"
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

	void on_error(char const* err)
	{
		printf("%s\n", err);
	}
};
*/

namespace med {

//MAX_LINE - max length of a single output of IE.print
template <class SINK, std::size_t MAX_LINE>
class printer
{
	template <int N>
	using int_t = std::integral_constant<int, N>;

	template <typename T>
	struct has_print
	{
		//check for: void IE::print(char (&sz)[N])
		template <class C, class P>
		static auto test(P* p) -> decltype(std::declval<C>().print(*p), int_t<2>{});

		//check for: some_type IE::print()
		template <class C, class P>
		static std::enable_if_t<!std::is_void_v<decltype(std::declval<C>().print())>, int_t<1>>
		test(P* p);

		template <class C, class P>
		static int_t<0> test(...);

		using type = decltype(test<T, char[MAX_LINE]>(nullptr));
	};

public:

	struct null_encoder
	{
		template <class IE>
		constexpr void operator()(printer& me, IE const& ie)
		{
			if constexpr (has_name_v<IE>)
			{
				me.print_named(ie, typename has_print<IE>::type{});
			}
		}
	};

	class container_encoder
	{
	public:
		template <class IE>
		void operator()(printer& me, IE const& ie)
		{
			if constexpr (has_name_v<IE>)
			{
				return print_named(me, ie, typename has_print<IE>::type{});
			}
			else
			{
				//treat un-named container as logical group -> depth not changing
				return ie.encode(me);
			}
		}

	private:
		//customized prints
		template <class IE>
		void print_named(printer& me, IE const& ie, int_t<2> pt)
		{
			me.print_named(ie, pt);
		}

		template <class IE>
		void print_named(printer& me, IE const& ie, int_t<1> pt)
		{
			me.print_named(ie, pt);
		}

		//no customized print, change depth level
		template <class IE>
		void print_named(printer& me, IE const& ie, int_t<0>)
		{
			me.m_sink.on_container(me.m_depth, name<IE>());
			auto const depth = me.m_depth++;
			CODEC_TRACE("depth -> %zu < max=%zu", me.m_depth, me.m_max_depth);
			if (0 == me.m_max_depth || me.m_max_depth > me.m_depth) { ie.encode(me); }
			me.m_depth = depth;
			CODEC_TRACE("depth <- %zu", depth);
		}
	};


	printer(SINK&& sink, std::size_t max_depth) noexcept
		: m_sink{ std::forward<SINK>(sink) }
		, m_max_depth{ max_depth }
	{
	}

	//primitive
	template <class IE, class IE_TYPE>
	constexpr void operator() (IE const& ie, IE_TYPE const&)
	{
		if constexpr (has_name_v<IE>)
		{
			print_named(ie, typename has_print<IE>::type{});
		}
	}

	//state
	constexpr void operator() (SNAPSHOT const&) { }
	//length encoder
	template <int DELTA> constexpr void operator() (placeholder::_length<DELTA> const&) { }

private:
	friend class container_encoder;
	friend struct null_encoder;

	//customized print 2
	template <class IE>
	void print_named(IE const& ie, int_t<2>)
	{
		char sz[MAX_LINE];
		ie.print(sz);
		m_sink.on_custom(m_depth, name<IE>(), sz);
	}

	//customized print 1
	template <class IE>
	void print_named(IE const& ie, int_t<1>)
	{
		m_sink.on_custom(m_depth, name<IE>(), ie.print());
	}

	//regular print
	template <class IE>
	void print_named(IE const& ie, int_t<0>)
	{
		m_sink.on_value(m_depth, IE::name(), ie.get());
	}

	SINK        m_sink;
	std::size_t m_depth {0};
	std::size_t const m_max_depth;
};

//print named IEs only up to given depth (not including, i.e. < max_depth)
//or full depth when max_depth=0
template <class SINK, class IE, std::size_t MAX_LINE = 128>
void print(SINK&& sink, IE const& ie, std::size_t max_depth = 0)
{
	try
	{
		encode(printer<SINK, MAX_LINE>{std::forward<SINK>(sink), max_depth}, ie);
	}
	catch (exception const& ex)
	{
		sink.on_error(ex.what());
	}
}



template <class SINK, std::size_t MAX_LINE>
class dumper
{
public:

	struct null_encoder
	{
		template <class IE>
		void operator()(dumper& me, IE const& ie)
		{
			me.dump(ie);
		}
	};

	struct container_encoder
	{
		template <class IE>
		void operator()(dumper& me, IE const& ie)
		{
			me.m_sink.on_container(me.m_depth, name<IE>());
			auto const depth = me.m_depth++;
			ie.encode(me);
			me.m_depth = depth;
		}
	};

	explicit dumper(SINK&& sink) noexcept
		: m_sink{ std::forward<SINK>(sink) }
	{
	}

	//primitives
	template <class IE, class IE_TYPE>
	void operator() (IE const& ie, IE_TYPE const&)
	{
		dump(ie);
	}

	//state
	constexpr void operator() (SNAPSHOT const&) { }
	//length encoder
	template <int DELTA> constexpr void operator() (placeholder::_length<DELTA> const&) { }

	template <class IE>
	void dump(IE const& ie)                     { m_sink.on_value(m_depth, name<IE>(), ie.get()); }

	friend struct container_encoder;

	SINK        m_sink;
	std::size_t m_depth {0};
};

//print all (named and not) IEs in full depth
template <class SINK, class IE, std::size_t MAX_LINE = 128>
void print_all(SINK&& sink, IE const& ie)
{
	dumper<SINK, MAX_LINE> d{std::forward<SINK>(sink)};
	try
	{
		encode(d, ie);
	}
	catch (exception const& ex)
	{
		d.m_sink.on_error(ex.what());
	}
}

} //namespace med
