/**
@file
choice class as union

@copyright Denis Priyomov 2016-2017
Distributed under the MIT License
(See accompanying file LICENSE or visit https://github.com/cppden/med)
*/

#pragma once

#include <new>

#include "exception.hpp"
#include "error.hpp"
#include "debug.hpp"
#include "tag.hpp"
#include "length.hpp"
#include "encode.hpp"
#include "decode.hpp"
#include "util/unique.hpp"

namespace med {

template <class... IEs>
struct cases;

template <class IE, class... IEs>
struct cases<IE, IEs...>
{
	template <typename FIELD>
	using at = std::conditional_t<
		std::is_same_v<typename IE::case_type, FIELD>,
		IE,
		typename cases<IEs...>::template at<FIELD>
	>;
};

template<>
struct cases<>
{
	template <typename FIELD>
	using at = void; //No such field found
};


namespace sl {

template <class... IEs>
struct choice_for;

template <class IE, class... IEs>
struct choice_for<IE, IEs...>
{
	template <typename TAG>
	static constexpr char const* name_tag(TAG const& tag)
	{
		if (IE::tag_type::match(tag))
		{
			using case_t = typename IE::case_type;
			return name<case_t>();
		}
		else
		{
			return choice_for<IEs...>::name_tag(tag);
		}
	}

	template <class ENCODER, class TO>
	static MED_RESULT encode(ENCODER&& encoder, TO const& to)
	{
		if (IE::tag_type::match(get_tag(to.get_header())))
		{
			using case_t = typename IE::case_type;
			CODEC_TRACE("CASE[%s]", name<case_t>());
			void const* store_p = &to.m_storage;

			return med::encode(encoder, to.get_header())
				MED_AND med::encode(encoder, *static_cast<case_t const*>(store_p));
		}
		else
		{
			return choice_for<IEs...>::encode(encoder, to);
		}
	}

	template <class DECODER, class TO, class UNEXP>
	static MED_RESULT decode(DECODER&& decoder, TO& to, UNEXP& unexp)
	{
		using case_t = typename IE::case_type;
		if (IE::tag_type::match( get_tag(to.get_header()) ))
		{
			CODEC_TRACE("->CASE[%s]", name<case_t>());
			case_t* case_p = new (&to.m_storage) case_t{};
			return med::decode(decoder, *case_p, unexp);
		}
		else
		{
			CODEC_TRACE("!CASE[%s]=%zu not a match for %zu", name<case_t>()
						, std::size_t(IE::tag_type::traits::value), std::size_t(get_tag(to.get_header())));
			return choice_for<IEs...>::decode(decoder, to, unexp);
		}
	}

	template <class TO>
	static std::size_t calc_length(TO const& to)
	{
		if (IE::tag_type::match( get_tag(to.get_header()) ))
		{
			using case_t = typename IE::case_type;
			void const* store_p = &to.m_storage;
			return med::get_length(to.get_header())
				+ med::get_length(*static_cast<case_t const*>(store_p));
		}
		else
		{
			return choice_for<IEs...>::calc_length(to);
		}
	}

	template <class TO, class FROM, class... ARGS>
	static inline MED_RESULT copy(TO& to, FROM const& from, ARGS&&... args)
	{
		if (IE::tag_type::match(get_tag(from.get_header())))
		{
			using value_type = typename IE::case_type;
			void const* store_p = &from.m_storage;
			MED_CHECK_FAIL(to.template ref<value_type>().copy(*static_cast<value_type const*>(store_p), std::forward<ARGS>(args)...));
			return to.header().copy(from.get_header());
		}
		else
		{
			return choice_for<IEs...>::copy(to, from, std::forward<ARGS>(args)...);
		}
	}
};

template <>
struct choice_for<>
{
	template <typename TAG>
	static constexpr char const* name_tag(TAG const&)
	{
		return nullptr;
	}

	template <class FUNC, class TO, class UNEXP>
	static MED_RESULT decode(FUNC&& func, TO& to, UNEXP& unexp)
	{
		CODEC_TRACE("unexp CASE[%s] tag=%zu", name<TO>(), std::size_t(get_tag(to.get_header())));
		return unexp(func, to, to.get_header());
	}

	template <class FUNC, class TO>
	static MED_RESULT encode(FUNC&& func, TO const& to)
	{
		CODEC_TRACE("unexp CASE[%s] tag=%zu", name<TO>(), std::size_t(get_tag(to.get_header())));
		return func(error::INCORRECT_TAG, name<TO>(), get_tag(to.get_header()));
	}

	template <class TO>
	static constexpr std::size_t calc_length(TO const&)
	{
		return 0;
	}

	template <class TO, class FROM, class... ARGS>
	static constexpr MED_RESULT copy(TO&, FROM const&, ARGS&&...)
	{
		MED_RETURN_FAILURE;
	}
};

}	//end: namespace sl

template <class HEADER, class ...CASES>
class choice : public IE<CONTAINER>
{
private:
	static_assert(util::are_unique(CASES::tag_type::get()...), "TAGS ARE NOT UNIQUE");

	template <class T>
	struct selector
	{
		explicit selector(T* that) noexcept
			: m_this(that)
		{}

		template <class U> operator U&()
		{
			static_assert(!std::is_const<T>(), "CONST CHOICE RETURNS A POINTER, NOT REFERENCE!");
			return m_this->template ref<U>();
		}

		template <class U> operator U const* ()
		{
			return m_this->template get<U>();
		}

		T* m_this;
	};
	template <class T>
	static auto make_selector(T* that) { return selector<T>{that}; }

public:
	using header_type = HEADER;

	template <typename TAG>
	static constexpr char const* name_tag(TAG const& tag)
	{
		return sl::choice_for<CASES...>::name_tag(tag);
	}

	header_type const& header() const       { return m_header; }
	header_type const& get_header() const   { return m_header; }
	header_type& header()                   { return m_header; }

	bool is_set() const                     { return header().is_set(); }
	std::size_t calc_length() const         { return sl::choice_for<CASES...>::calc_length(*this); }

	auto select()                           { return make_selector(this); }
	auto select() const                     { return make_selector(this); }
	auto cselect() const                    { return make_selector(this); }

	template <class CASE>
	CASE& ref()
	{
		//TODO: how to prevent a copy when callee-side re-uses reference by mistake?
		static_assert(!std::is_const<CASE>(), "REFERENCE IS NOT FOR ACCESSING AS CONST");
		using IE = typename cases<CASES...>::template at<CASE>;
		static_assert(!std::is_same<void, IE>(), "NO SUCH CASE IN CHOICE");
		void* store_p = &m_storage;
		return *(set_tag(header(), IE::tag_type::get_encoded())
			? new (store_p) CASE{}
			: static_cast<CASE*>(store_p));
	}

	template <class CASE>
	CASE const* get() const
	{
		using IE = typename cases<CASES...>::template at<CASE>;
		static_assert(!std::is_same<void, IE>(), "NO SUCH CASE IN CHOICE");
		void const* store_p = &m_storage;
		return IE::tag_type::match( get_tag(header()) ) ? static_cast<CASE const*>(store_p) : nullptr;
	}

	template <class CASE>
	CASE& clear()
	{
		using IE = typename cases<CASES...>::template at<CASE>;
		static_assert(!std::is_same<void, IE>(), "NO SUCH CASE IN CHOICE");
		set_tag(header(), IE::tag_type::get());
		return *(new (&m_storage) CASE{});
	}

	template <class FROM, class... ARGS>
	MED_RESULT copy(FROM const& from, ARGS&&... args)
	{
		if (from.is_set())
		{
			return sl::choice_for<CASES...>::copy(*this, from, std::forward<ARGS>(args)...);
		}
		MED_RETURN_SUCCESS;
	}

	template <class DST, class... ARGS>
	MED_RESULT copy_to(DST& to, ARGS&&... args) const
	{
		if (this->is_set())
		{
			return sl::choice_for<CASES...>::copy(to, *this, std::forward<ARGS>(args)...);
		}
		MED_RETURN_SUCCESS;
	}


	template <class ENCODER>
	MED_RESULT encode(ENCODER&& encoder) const
	{
		return sl::choice_for<CASES...>::encode(encoder, *this);
	}

	template <class DECODER, class UNEXP>
	MED_RESULT decode(DECODER&& decoder, UNEXP& unexp)
	{
		return med::decode(decoder, header(), unexp)
			MED_AND sl::choice_for<CASES...>::decode(decoder, *this, unexp);
	}

private:
	template <class... IEs>
	friend struct sl::choice_for;

	using storage_type = std::aligned_union_t<0, typename CASES::case_type...>;

	header_type  m_header;
	storage_type m_storage;
};

} //end: namespace med
