#pragma once
/*
@file
foreach

@copyright Denis Priyomov 2019
Distributed under the MIT License
(See accompanying file LICENSE or visit https://github.com/cppden/ctstring)
*/
#include <functional>

#include "typelist.hpp"

namespace med::meta {

namespace detail {

template <class... Ts>
struct foreach;

template <class T0, class... Ts>
struct foreach<T0, Ts...>
{
	template <class F, class... Args>
	static constexpr auto exec(F f, Args&&... args)
	{
		f.template apply<T0>(std::forward<Args>(args)...);
		return foreach<Ts...>::template exec(f, std::forward<Args>(args)...);
	}

	template <class PREV, class F, class... Args>
	static constexpr auto exec(F f, Args&&... args)
	{
		f.template apply<PREV, T0>(std::forward<Args>(args)...);
		return foreach<Ts...>::template exec<T0>(f, std::forward<Args>(args)...);
	}
};

template <>
struct foreach<>
{
	template <class F, class... Args>
	static constexpr auto exec(F f, Args&&... args)
	{
		return f.template apply(std::forward<Args>(args)...);
	}

	template <class PREV, class F, class... Args>
	static constexpr auto exec(F f, Args&&... args)
	{
		return f.template apply<PREV>(std::forward<Args>(args)...);
	}
};

template <template<class...> class L, class... Elems, class F, class... Args>
constexpr auto foreach_impl(L<Elems...>&&, F f, Args&&... args)
{
	return foreach<Elems...>::exec(f, std::forward<Args>(args)...);
}

template <template<class...> class L, class... Elems, class F, class... Args>
constexpr auto foreach_impl_prev(L<Elems...>&&, F f, Args&&... args)
{
	return foreach<Elems...>::template exec<void>(f, std::forward<Args>(args)...);
}

} //end: namespace detail

template <class L, class F, class... Args>
constexpr auto foreach(F f, Args&&... args)
{
	return detail::foreach_impl(L{}, f, std::forward<Args>(args)...);
}

template <class L, class F, class... Args>
constexpr auto foreach_prev(F f, Args&&... args)
{
	return detail::foreach_impl_prev(L{}, f, std::forward<Args>(args)...);
}

/////////////////////////////////////////////

namespace detail {

template <class... Ts>
struct fold;

template <class T0, class... Ts>
struct fold<T0, Ts...>
{
	template <class F, class... Args>
	static constexpr auto exec(F f, Args&&... args)
	{
		auto const r1 = f.template apply<T0>(std::forward<Args>(args)...);
		auto const r2 = fold<Ts...>::template exec(f, std::forward<Args>(args)...);
		return f.op(r1, r2);
	}
};

template <>
struct fold<>
{
	template <class F, class... Args>
	static constexpr auto exec(F f, Args&&... args)
	{
		return f.template apply(std::forward<Args>(args)...);
	}
};

template <template<class...> class L, class... Elems, class F, class... Args>
constexpr auto fold_impl(L<Elems...>&&, F f, Args&&... args)
{
	return fold<Elems...>::exec(f, std::forward<Args>(args)...);
}

} //end: namespace detail

template <class L, class F, class... Args>
constexpr auto fold(F f, Args&&... args)
{
	return detail::fold_impl(L{}, f, std::forward<Args>(args)...);
}

/////////////////////////////////////////////

namespace detail {

template <class... Ts>
struct for_if;

template <class T0, class... Ts>
struct for_if<T0, Ts...>
{
	template <class F, class... Args>
	static constexpr auto exec(F f, Args&&... args)
	{
		if (f.template check<T0>(std::forward<Args>(args)...))
		{
			return f.template apply<T0>(std::forward<Args>(args)...);
		}
		else
		{
			return for_if<Ts...>::template exec(f, std::forward<Args>(args)...);
		}
	}

	template <class PREV, class F, class... Args>
	static constexpr auto exec(F f, Args&&... args)
	{
		if (f.template check<PREV, T0>(std::forward<Args>(args)...))
		{
			return f.template apply<PREV, T0>(std::forward<Args>(args)...);
		}
		else
		{
			return for_if<Ts...>::template exec<T0>(f, std::forward<Args>(args)...);
		}
	}
};

template <>
struct for_if<>
{
	template <class F, class... Args>
	static constexpr auto exec(F f, Args&&... args)
	{
		return f.template apply(std::forward<Args>(args)...);
	}

	template <class PREV, class F, class... Args>
	static constexpr auto exec(F f, Args&&... args)
	{
		return f.template apply<PREV>(std::forward<Args>(args)...);
	}
};

template <template<class...> class L, class... Elems, class F, class... Args>
constexpr auto for_if_impl(L<Elems...>&&, F f, Args&&... args)
{
	return for_if<Elems...>::exec(f, std::forward<Args>(args)...);
}

template <template<class...> class L, class... Elems, class F, class... Args>
constexpr auto for_if_impl_prev(L<Elems...>&&, F f, Args&&... args)
{
	return for_if<Elems...>::template exec<void>(f, std::forward<Args>(args)...);
}

} //end: namespace detail

template <class L, class F, class... Args>
constexpr auto for_if(F f, Args&&... args)
{
	return detail::for_if_impl(L{}, f, std::forward<Args>(args)...);
}

template <class L, class F, class... Args>
constexpr auto for_if_prev(F f, Args&&... args)
{
	return detail::for_if_impl_prev(L{}, f, std::forward<Args>(args)...);
}

namespace detail {

template<bool C> struct if_t;
template<> struct if_t<true>
{
	template<typename THEN, typename ELSE> using type = THEN;
};
template<> struct if_t<false>
{
	template<typename THEN, typename ELSE> using type = ELSE;
};

} //end: namespace detail

template<bool C, typename THEN, typename ELSE>
using conditional_t = typename detail::if_t<C>::template type<THEN, ELSE>;


namespace detail {

template <class... Ts>
struct find;

template <class T0, class... Ts>
struct find<T0, Ts...>
{
	template <class CMP>
	using type = conditional_t<CMP::template type<T0>::value, T0, typename find<Ts...>::template type<CMP>>;
};

template <>
struct find<>
{
	template <typename CMP>
	using type = void; //not found
};

template <class L, class CMP>
struct find_impl;

template <template<class...> class L, class... Elems, class CMP>
struct find_impl<L<Elems...>, CMP>
{
	using type = typename find<Elems...>::template type<CMP>;
};

} //end: namespace detail


template <class L, class CMP>
using find = typename detail::find_impl<L, CMP>::type;

} //end: namespace med::meta

