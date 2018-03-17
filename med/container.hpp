/**
@file
base container class used in sequence and set

@copyright Denis Priyomov 2016-2017
Distributed under the MIT License
(See accompanying file LICENSE or visit https://github.com/cppden/med)
*/

#pragma once

#include "debug.hpp"
#include "accessor.hpp"
#include "multi_field.hpp"

namespace med {

template <class... IES>
struct ies;

template <class IE, class... IES>
struct ies<IE, IES...>
{
	template <typename FIELD>
	using at = std::conditional_t<
		std::is_same<get_field_type_t<IE>, FIELD>::value,
		IE,
		typename ies<IES...>::template at<FIELD>
	>;
};

template<>
struct ies<>
{
	template <typename FIELD>
	using at = void; //not found
};

namespace sl {

//-----------------------------------------------------------------------
template <class IE>
constexpr std::size_t field_arity()
{
	if constexpr (is_multi_field_v<IE>)
	{
		return IE::max;
	}
	else
	{
		return 1;
	}
}

//-----------------------------------------------------------------------
template <class IE>
inline bool is_field_set(IE const& ie)
{
	if constexpr (is_multi_field_v<IE>)
	{
		return ie.is_set();
	}
	else
	{
		return ie.ref_field().is_set();
	}
}

//NOTE! check stops at 1st mandatory since it's optimal and more flexible
template <class... IES>
struct seq_is_set;

template <class IE, class... IES>
struct seq_is_set<IE, IES...>
{
	template <class TO>
	static constexpr bool is_set(TO const& to)
	{
		//optional or mandatory field w/ setter => can be set implicitly
		if constexpr (is_optional_v<IE> || has_setter_type_v<IE>)
		{
			return is_field_set<IE>(to) || seq_is_set<IES...>::is_set(to);
		}
		//mandatory field w/o setter => s.b. set explicitly
		else //if constexpr (!is_optional_v<IE> && !has_setter_type_v<IE>)
		{
			return is_field_set<IE>(to);
		}
	}
};

template <>
struct seq_is_set<void>
{
	template <class TO>
	static constexpr bool is_set(TO const&)    { return false; }
};

//-----------------------------------------------------------------------
template <class IE>
inline void field_clear(IE& ie)
{
	if constexpr (is_multi_field_v<IE>)
	{
		ie.clear();
	}
	else
	{
		ie.ref_field().clear();
	}
}

template <class... IES>
struct seq_clear;

template <class IE, class... IES>
struct seq_clear<IE, IES...>
{
	template <class TO>
	static inline void clear(TO& to) { field_clear<IE>(to); seq_clear<IES...>::clear(to); }
};

template <>
struct seq_clear<>
{
	template <class TO>
	static constexpr void clear(TO&)    { }
};

//-----------------------------------------------------------------------
template <class IE, class ...ARGS>
inline MED_RESULT field_copy(IE& to, IE const& from, ARGS&&... args)
{
	if constexpr (is_multi_field_v<IE>)
	{
		to.clear();
		for (auto const& rhs : from)
		{
			if (auto* p = to.push_back(std::forward<ARGS>(args)...))
			{
				MED_CHECK_FAIL(p->copy(rhs, std::forward<ARGS>(args)...));
			}
			else
			{
				MED_RETURN_FAILURE;
			}
		}
		MED_RETURN_SUCCESS;
	}
	else
	{
		return to.ref_field().copy(from.ref_field(), std::forward<ARGS>(args)...);
	}
}

template <class... IES>
struct seq_copy;

template <class IE, class... IES>
struct seq_copy<IE, IES...>
{
	template <class FIELDS, class... ARGS>
	static inline MED_RESULT copy(FIELDS& to, FIELDS const& from, ARGS&&... args)
	{
		return field_copy<IE>(to, from, std::forward<ARGS>(args)...)
			MED_AND seq_copy<IES...>::copy(to, from, std::forward<ARGS>(args)...);
	}
};

template <>
struct seq_copy<>
{
	template <class FIELDS, class... ARGS>
	static constexpr MED_RESULT copy(FIELDS&, FIELDS const&, ARGS&&...)
	{
		MED_RETURN_SUCCESS;
	}
};

//-----------------------------------------------------------------------
template <class... IES>
struct container_for;

template <class IE, class... IES>
struct container_for<IE, IES...>
{
	template <class TO>
	static constexpr std::size_t calc_length(TO const& to)
	{
		if constexpr (is_multi_field_v<IE>)
		{
			IE const& ie = to;
			CODEC_TRACE("calc_length[%s]*", name<IE>());
			std::size_t len = 0;
			for (auto& v : ie) { len += get_length<IE>(v); }
			return len + container_for<IES...>::calc_length(to);
		}
		else
		{
			IE const& ie = to;
			return (has_setter_type_v<IE> || ie.ref_field().is_set() ? get_length<IE>(ie.ref_field()) : 0)
				+ container_for<IES...>::calc_length(to);
		}
	}
};

template <>
struct container_for<>
{
	template <class TO>
	static constexpr std::size_t calc_length(TO const&)
	{
		return 0;
	}
};

}	//end: namespace sl

template <class ...IES>
class container : public IE<CONTAINER>
{
public:
	using container_t = container<IES...>;

	template <class FIELD>
	FIELD& ref()
	{
		static_assert(!std::is_const<FIELD>(), "ATTEMPT TO COPY FROM CONST REF");
		auto& ie = m_ies.template as<FIELD>();
		using IE = remove_cref_t<decltype(ie)>;
		static_assert(!is_multi_field<IE>(), "MULTI-INSTANCE FIELDS ARE WRITTEN VIA PUSH_BACK");
		return ie.ref_field();
	}

	template <class FIELD>
	decltype(auto) get() const
	{
		auto& ie = m_ies.template as<FIELD>();
		//using IE = remove_cref_t<decltype(ie)>;
		return get_field<FIELD>(ie);
	}

	template <class FIELD>
	[[deprecated("inefficient, may be removed")]]
	FIELD const* get(std::size_t index) const
	{
		auto& ie = m_ies.template as<FIELD>();
		using IE = remove_cref_t<decltype(ie)>;
		static_assert(is_multi_field<IE>(), "SINGLE-INSTANCE FIELDS ARE READ W/O INDEX");
		return ie.at(index);
	}

	template <class FIELD, class... ARGS>
	FIELD* push_back(ARGS&&... args)
	{
		auto& ie = m_ies.template as<FIELD>();
		using IE = remove_cref_t<decltype(ie)>;
		static_assert(is_multi_field<IE>(), "SINGLE-INSTANCE FIELDS ARE WRITTEN VIA REF");
		return ie.push_back(std::forward<ARGS>(args)...);
	}

	auto field()                            { return make_accessor(*this); }
	auto field() const                      { return make_accessor(*this); }
	auto cfield() const                     { return make_accessor(*this); }
	[[deprecated("inefficient, will be removed")]]
	auto field(std::size_t index) const     { return make_accessor(*this, index); }
	[[deprecated("inefficient, will be removed")]]
	auto cfield(std::size_t index) const    { return make_accessor(*this, index); }

	template <class FIELD>
	std::size_t count() const               { return field_count(m_ies.template as<FIELD>()); }

	template <class FIELD>
	static constexpr std::size_t arity()
	{
		using IE = typename ies<IES...>::template at<FIELD>;
		return sl::field_arity<IE>();
	}

	template <class FIELD>
	void clear()                            { m_ies.template as<FIELD>().clear(); }
	void clear()                            { sl::seq_clear<IES...>::clear(this->m_ies); }
	bool is_set() const                     { return sl::seq_is_set<IES...>::is_set(this->m_ies); }
	std::size_t calc_length() const         { return sl::container_for<IES...>::calc_length(this->m_ies); }

	template <class... ARGS>
	MED_RESULT copy(container_t const& from, ARGS&&... args)
	{
		return sl::seq_copy<IES...>::copy(this->m_ies, from.m_ies, std::forward<ARGS>(args)...);
	}

	template <class... ARGS>
	MED_RESULT copy_to(container_t& to, ARGS&&... args) const
	{
		return sl::seq_copy<IES...>::copy(to.m_ies, this->m_ies, std::forward<ARGS>(args)...);
	}

protected:
	struct ies_t : IES...
	{
		template <class FIELD>
		decltype(auto) as() const
		{
			using IE = typename ies<IES...>::template at<FIELD>;
			static_assert(!std::is_same<void, IE>(), "NO SUCH FIELD");
			return static_cast<IE const&>(*this);
		}

		template <class FIELD>
		decltype(auto) as()
		{
			using IE = typename ies<IES...>::template at<FIELD>;
			static_assert(!std::is_same<void, IE>(), "NO SUCH FIELD");
			return static_cast<IE&>(*this);
		}
	};

	ies_t m_ies;
};


}

