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
#include "sl/field_copy.hpp"

namespace med {

template <class... IES>
struct ies;

template <class IE, class... IES>
struct ies<IE, IES...>
{
	template <typename FIELD>
	using at = std::conditional_t<
		std::is_same_v<get_field_type_t<IE>, FIELD>,
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
template <class... IES>
struct container_for;

template <class IE, class... IES>
struct container_for<IE, IES...>
{
	template <class TO>
	static inline void clear(TO& to)
	{
		static_cast<IE&>(to).clear();
		container_for<IES...>::clear(to);
	}

	template <class TO>
	static constexpr bool is_set(TO const& to)
	{
		//optional or mandatory field w/ setter => can be set implicitly
		if constexpr (is_optional_v<IE> || has_setter_type_v<IE>)
		{
			return static_cast<IE const&>(to).is_set() || container_for<IES...>::is_set(to);
		}
		else //mandatory field w/o setter => s.b. set explicitly
		{
			return static_cast<IE const&>(to).is_set();
		}
	}

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

	template <class TO, class FROM, class... ARGS>
	static inline MED_RESULT copy(TO& to, FROM const& from, ARGS&&... args)
	{
		using field_type = get_field_type_t<IE>;
		auto const& from_field = from.m_ies.template as<field_type>();
		if (from_field.is_set())
		{
			auto& to_field = to.m_ies.template as<field_type>();
			if constexpr (is_multi_field_v<IE>)
			{
				to_field.clear();
				for (auto const& rhs : from_field)
				{
					if (auto* p = to_field.push_back(std::forward<ARGS>(args)...))
					{
						MED_CHECK_FAIL(p->copy(rhs, std::forward<ARGS>(args)...));
					}
					else
					{
						MED_RETURN_FAILURE;
					}
				}
			}
			else
			{
				MED_CHECK_FAIL(to_field.ref_field().copy(from_field.ref_field(), std::forward<ARGS>(args)...));
			}
		}
		//MED_CHECK_FAIL( field_copy<IE>(to, from, std::forward<ARGS>(args)...) );
		return container_for<IES...>::copy(to, from, std::forward<ARGS>(args)...);
	}
};

template <>
struct container_for<>
{
	template <class TO>
	static constexpr void clear(TO&)                                { }

	template <class TO>
	static constexpr bool is_set(TO const&)                         { return false; }

	template <class TO>
	static constexpr std::size_t calc_length(TO const&)             { return 0; }

	template <class TO, class FROM, class... ARGS>
	static constexpr MED_RESULT copy(TO&, FROM const&, ARGS&&...)   { MED_RETURN_SUCCESS; }
};

}	//end: namespace sl

template <class... IES>
class container : public IE<CONTAINER>
{
public:
	using container_t = container<IES...>;

	template <class FIELD>
	static constexpr bool has()
	{
		using IE = typename ies<IES...>::template at<FIELD>;
		return not std::is_same_v<void, IE>;
	}

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
		static_assert(is_multi_field<IE>(), "SINGLE-INSTANCE FIELDS ARE WRITTEN VIA ref");
		return ie.push_back(std::forward<ARGS>(args)...);
	}

	//TODO: would be nice to recover space from allocator if possible
	template <class FIELD>
	void pop_back()
	{
		auto& ie = m_ies.template as<FIELD>();
		using IE = remove_cref_t<decltype(ie)>;
		static_assert(is_multi_field<IE>(), "SINGLE-INSTANCE FIELDS ARE REMOVED VIA clear");
		ie.pop_back();
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
	void clear()                            { sl::container_for<IES...>::clear(this->m_ies); }
	//NOTE! check stops at 1st mandatory since it's optimal and more flexible
	bool is_set() const                     { return sl::container_for<IES...>::is_set(this->m_ies); }
	std::size_t calc_length() const         { return sl::container_for<IES...>::calc_length(this->m_ies); }

	template <class FROM, class... ARGS>
	MED_RESULT copy(FROM const& from, ARGS&&... args)
	{
		return sl::container_for<IES...>::copy(*this, from, std::forward<ARGS>(args)...);
	}

	template <class TO, class... ARGS>
	MED_RESULT copy_to(TO& to, ARGS&&... args) const
	{
		return sl::container_for<IES...>::copy(to, *this, std::forward<ARGS>(args)...);
	}

protected:
	template <class...>
	friend struct sl::container_for;

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

