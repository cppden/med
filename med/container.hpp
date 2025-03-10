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
#include "length.hpp"
#include "concepts.hpp"
#include "sl/field_copy.hpp"
#include "meta/typelist.hpp"
#include "meta/foreach.hpp"

namespace med {

namespace sl {

struct cont_clear
{
	template <class IE, class SEQ>
	static void apply(SEQ& s)           { static_cast<IE&>(s).clear(); }
};

struct cont_copy
{
	template <class IE, class TO, class FROM, class... ARGS>
	static void apply(TO& to, FROM const& from, ARGS&&... args)
	{
		using field_t = get_field_type_t<IE>;
		auto const& from_field = from.m_ies.template as<field_t>();
		if (from_field.is_set())
		{
			auto& to_field = to.m_ies.template as<field_t>();
			if constexpr (AMultiField<IE>)
			{
				to_field.clear();
				for (auto const& rhs : from_field)
				{
					auto* p = to_field.push_back(std::forward<ARGS>(args)...);
					p->copy(rhs, std::forward<ARGS>(args)...);
				}
			}
			else
			{
				to_field.copy(from_field, std::forward<ARGS>(args)...);
			}
		}
	}
};

template <class TYPE_CTX>
struct cont_len
{
	using EXP_TAG = typename TYPE_CTX::explicit_tag_type;
	using EXP_LEN = typename TYPE_CTX::explicit_length_type;

	static constexpr std::size_t op(std::size_t r1, std::size_t r2) { return r1 + r2; }

	template <class IE, class SEQ, class ENCODER>
	static std::size_t apply(SEQ const& seq, ENCODER& encoder)
	{
		using mi = meta::produce_info_t<ENCODER, IE>;
		using ctx = type_context<typename TYPE_CTX::ie_type, mi, EXP_TAG, EXP_LEN>;
		if constexpr (AMultiField<IE>) //TODO: duplicated in encoder
		{
			IE const& ie = seq;
			std::size_t len = 0;
			for (auto& v : ie) { len += sl::ie_length<ctx>(v, encoder); }
			CODEC_TRACE("cont_len[%s]*%zu len=%zu", name<IE>(), ie.count(), len);
			return len;
		}
		else
		{
			if constexpr (ASameAs<get_field_type_t<IE>, EXP_TAG, EXP_LEN>)
			{
				CODEC_TRACE("cont_len[%s] skip explicit", name<IE>());
				return 0;
			}
			else if constexpr (AMandatory<IE> || AHasSetterType<IE>)
			{
				IE const& ie = seq;
				auto const len = sl::ie_length<ctx>(ie, encoder);
				CODEC_TRACE("cont_len[%s] len=%zu", name<IE>(), len);
				return len;
			}
			else
			{
				IE const& ie = seq;
				auto const len = ie.is_set() ? sl::ie_length<ctx>(ie, encoder) : 0;
				CODEC_TRACE("cont_len[%s] len=%zu", name<IE>(), len);
				return len;
			}
		}
	}

	template <class SEQ, class ENCODER>
	static constexpr std::size_t apply(SEQ const&, ENCODER&)        { return 0; }
};

struct cont_is
{
	static constexpr bool op(bool r1, bool r2)  { return r1 || r2; }

	template <class IE, class SEQ>
	static constexpr bool apply(SEQ const& seq)
	{
		//optional or mandatory field w/ setter => can be set implicitly
		if constexpr (AOptional<IE> || AHasSetterType<IE>)
		{
			CODEC_TRACE("O/S[%s] is_set=%d", name<IE>(), static_cast<IE const&>(seq).is_set());
			return static_cast<IE const&>(seq).is_set();
		}
		else //mandatory field w/o setter => s.b. set explicitly
		{
			CODEC_TRACE("M[%s]%s is_set=%d", name<IE>(), (APredefinedValue<IE> ? "[init/const]":""), static_cast<IE const&>(seq).is_set());
			if constexpr (APredefinedValue<IE>) //don't account predefined IEs like init/const
			{
				return false;
			}
			else
			{
				return static_cast<IE const&>(seq).is_set();
			}
		}
	}

	template <class SEQ>
	static constexpr bool apply(SEQ const&)     { return false; }
};

struct cont_eq
{
	static constexpr bool op(bool r1, bool r2)  { return r1 && r2; }

	template <class IE, class SEQ>
	static constexpr bool apply(SEQ const& lhs, SEQ const& rhs)
	{
		return static_cast<IE const&>(lhs) == static_cast<IE const&>(rhs);
	}

	template <class SEQ>
	static constexpr bool apply(SEQ const&, SEQ const&)     { return true; }
};

//-----------------------------------------------------------------------
template <class IE>
constexpr std::size_t field_arity()
{
	if constexpr (AMultiField<IE>)
	{
		return IE::max;
	}
	else
	{
		return 1;
	}
}

}	//end: namespace sl

template <class IE_TYPE, class... IES>
class container : public IE<IE_TYPE>
{
public:
	using container_t = container<IE_TYPE, IES...>;
	using ies_types = meta::typelist<IES...>;

	template <class T>
	static constexpr bool has()             { return not std::is_void_v<meta::find_t<ies_types, sl::field_at<T>>>; }

	template <class FIELD>
	decltype(auto) ref()
	{
		static_assert(!std::is_const_v<FIELD>, "ATTEMPT TO COPY FROM CONST REF");
		auto& ie = m_ies.template as<FIELD>();
		using IE = std::remove_cvref_t<decltype(ie)>;
		if constexpr (AMultiField<IE>)
		{
			return static_cast<IE&>(ie);
		}
		else
		{
			return static_cast<FIELD&>(ie);
		}
	}

	template <class FIELD>
	decltype(auto) get() const
	{
		auto& ie = m_ies.template as<FIELD>();
		return get_field<FIELD>(ie);
	}

	template <class FIELD>
	std::size_t count() const               { return field_count(m_ies.template as<FIELD>()); }

	template <class FIELD>
	static constexpr std::size_t arity()    { return sl::field_arity<meta::find_t<ies_types, sl::field_at<FIELD>>>(); }

	template <class FIELD>
	void clear()                            { m_ies.template as<FIELD>().clear(); }
	void clear()                            { meta::foreach<ies_types>(sl::cont_clear{}, this->m_ies); }
	bool is_set() const                     { return meta::fold<ies_types>(sl::cont_is{}, this->m_ies); }
	template <class IE_LIST, class TYPE_CTX>
	std::size_t calc_length(auto& enc) const { return meta::fold<IE_LIST>(sl::cont_len<TYPE_CTX>{}, this->m_ies, enc); }
	template <class TYPE_CTX = type_context<IE_TYPE>>
	std::size_t calc_length(auto& enc) const { return calc_length<ies_types, TYPE_CTX>(enc); }

	template <class FROM, class... ARGS>
	void copy(FROM const& from, ARGS&&... args)
	{ meta::foreach<ies_types>(sl::cont_copy{}, *this, from, std::forward<ARGS>(args)...); }

	template <class TO, class... ARGS>
	void copy_to(TO& to, ARGS&&... args) const
	{ meta::foreach<ies_types>(sl::cont_copy{}, to, *this, std::forward<ARGS>(args)...); }

	bool operator==(container const& rhs) const { return meta::fold<ies_types>(sl::cont_eq{}, this->m_ies, rhs.m_ies); }

protected:
	friend struct sl::cont_copy;

	struct ies_t : IES...
	{
		template <class FIELD>
		decltype(auto) as() const
		{
			using IE = meta::find_t<ies_types, sl::field_at<FIELD>>;
			static_assert(!std::is_void<IE>(), "NO SUCH FIELD");
			return static_cast<IE const&>(*this);
		}

		template <class FIELD>
		decltype(auto) as()
		{
			using IE = meta::find_t<ies_types, sl::field_at<FIELD>>;
			static_assert(!std::is_void<IE>(), "NO SUCH FIELD");
			return static_cast<IE&>(*this);
		}
	};

	ies_t m_ies;
};

} //end: namespace med
