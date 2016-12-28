/*!
@file
TODO: define.

@copyright Denis Priyomov 2016
Distributed under the MIT License
(See accompanying file LICENSE or copy at https://opensource.org/licenses/MIT)
*/

#pragma once

#include "ie.hpp"
#include "accessor.hpp"

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
std::enable_if_t<!is_multi_field_v<IE>, std::size_t>
constexpr field_arity()                     { return 1; }
template <class IE>
std::enable_if_t<is_multi_field_v<IE>, std::size_t>
constexpr field_arity()                     { return IE::max; }

//-----------------------------------------------------------------------
template <class IE>
std::enable_if_t<!is_multi_field_v<IE>, bool>
inline is_field_set(IE const& ie)           { return ie.ref_field().is_set(); }
template <class IE>
std::enable_if_t<is_multi_field_v<IE>, bool>
inline is_field_set(IE const& ie)           { return ie.is_set(); }

//NOTE! check stops at 1st mandatory since it's optimal and more flexible
template <class Enable, class... IES>
struct seq_is_set_imp;

template <class... IES>
using seq_is_set = seq_is_set_imp<void, IES...>;

template <class IE, class... IES> //optional or mandatory field w/ setter => can be set implicitly
struct seq_is_set_imp<std::enable_if_t<is_optional_v<IE> || has_setter_type_v<IE>>, IE, IES...>
{
	template <class TO>
	static inline bool is_set(TO const& to) { return is_field_set<IE>(to) || seq_is_set<IES...>::is_set(to); }
};

template <class IE, class... IES> //mandatory field w/o setter => s.b. set explicitly
struct seq_is_set_imp<std::enable_if_t<!is_optional_v<IE> && !has_setter_type_v<IE>>, IE, IES...>
{
	template <class TO>
	static inline bool is_set(TO const& to) { return is_field_set<IE>(to); }
};

template <>
struct seq_is_set_imp<void>
{
	template <class TO>
	static inline bool is_set(TO const&)    { return false; }
};

//-----------------------------------------------------------------------
template <class IE>
std::enable_if_t<!is_multi_field_v<IE>>
inline field_clear(IE& ie)           { ie.ref_field().clear(); }

template <class IE>
std::enable_if_t<is_multi_field_v<IE>>
inline field_clear(IE& ie)
{
	//TODO: clear all, not the 1st only
	ie.template ref_field<0>().clear();
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
template <class Enable, class... IES>
struct container_for_imp;

template <class... IES>
using container_for = container_for_imp<void, IES...>;

template <class IE, class... IES>
struct container_for_imp<std::enable_if_t<!is_multi_field_v<IE>>, IE, IES...>
{
	template <class TO>
	static inline std::size_t calc_length(TO const& to)
	{
		IE const& ie = to;
		return (ie.ref_field().is_set() ? med::get_length<IE>(ie.ref_field()) : 0)
			+ container_for<IES...>::calc_length(to);
	}
};

template <class IE, class... IES>
struct container_for_imp<std::enable_if_t<is_multi_field_v<IE>>, IE, IES...>
{
	template <class TO>
	static inline std::size_t calc_length(TO const& to)
	{
		IE const& ie = to;
		CODEC_TRACE("calc_length[%s]*", name<IE>());
		std::size_t len = 0;
		for (auto& v : ie) { len += med::get_length<IE>(v); }
		return len + container_for<IES...>::calc_length(to);
	}
};

template <>
struct container_for_imp<void>
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
		static_assert(!is_multi_field<IE>(), "MULTI-INSTANCE FIELDS ARE ACCESSED BY INDEX");
		return ie.ref_field();
	}

	template <class FIELD>
	FIELD* ref(std::size_t index)
	{
		auto& ie = m_ies.template as<FIELD>();
		using IE = remove_cref_t<decltype(ie)>;
		static_assert(is_multi_field<IE>(), "SINGLE-INSTANCE FIELDS CAN'T BE ACCESSED BY INDEX");
		return ie.get_field(index);
	}

	template <class FIELD>
	decltype(auto) get() const
	{
		auto& ie = m_ies.template as<FIELD>();
		using IE = remove_cref_t<decltype(ie)>;
		static_assert(!is_multi_field<IE>(), "MULTI-INSTANCE FIELDS ARE ACCESSED BY INDEX");
		return get_field<FIELD>(ie);
	}

	template <class FIELD>
	decltype(auto) get(std::size_t index) const
	{
		auto& ie = m_ies.template as<FIELD>();
		using IE = remove_cref_t<decltype(ie)>;
		static_assert(is_multi_field<IE>(), "SINGLE-INSTANCE FIELDS CAN'T BE ACCESSED BY INDEX");
		return get_field<FIELD>(ie, index);
	}


	template <class FIELD>
	std::size_t count() const               { return field_count(m_ies.template as<FIELD>()); }

	template <class FIELD>
	std::size_t arity() const               { return sl::field_arity(m_ies.template as<FIELD>()); }

	void clear()                            { sl::seq_clear<IES...>::clear(this->m_ies); }
	bool is_set() const                     { return sl::seq_is_set<IES...>::is_set(this->m_ies); }
	std::size_t calc_length() const         { return sl::container_for<IES...>::calc_length(this->m_ies); }

	auto field()                            { return make_accessor(*this); }
	auto field() const                      { return make_accessor(*this); }
	auto cfield() const                     { return make_accessor(*this); }

	auto field(std::size_t index)           { return make_accessor(*this, index); }
	auto field(std::size_t index) const     { return make_accessor(*this, index); }
	auto cfield(std::size_t index) const    { return make_accessor(*this, index); }

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

